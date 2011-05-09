// System
#include <sys/stat.h>
#include <sys/mman.h>

// C
#include <assert.h>

// C++
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// Local
#include "crc32.h"


std::string string_vprintf(const char *fmt, va_list args)
{
  std::string ret;
  int size= 200;
  while (1) {
    ret.resize(size);
#if defined(_WIN32)
    int nwritten= _vsnprintf(&ret[0], size-1, fmt, args);
#else
    int nwritten= vsnprintf(&ret[0], size-1, fmt, args);
#endif
    // Some c libraries return -1 for overflow, some return
    // a number larger than size-1
    if (nwritten >= 0 && nwritten < size-2) {
      if (ret[nwritten-1] == 0) nwritten--;
      ret.resize(nwritten);
      return ret;
    }
    size *= 2;
  }
}

std::string string_printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  std::string ret= string_vprintf(fmt, args);
  va_end(args);
  return ret;
}

class ParseError : public std::exception {
  std::string m_what;
public:
  ParseError(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    m_what = string_vprintf(fmt, args);
    va_end(args);
  }
  virtual const char* what() const throw() { return m_what.c_str(); }
  virtual ~ParseError() throw() {}
};

double read_float64(const unsigned char *&ptr)
{
  double ret;
  unsigned int little_endian_test = 1;
  if (((unsigned char*)&little_endian_test)[0]) {
    ret = *(double*)ptr;
  } else {
    ((unsigned char*)&ret)[0] = ptr[7];
    ((unsigned char*)&ret)[1] = ptr[6];
    ((unsigned char*)&ret)[2] = ptr[5];
    ((unsigned char*)&ret)[3] = ptr[4];
    ((unsigned char*)&ret)[4] = ptr[3];
    ((unsigned char*)&ret)[5] = ptr[2];
    ((unsigned char*)&ret)[6] = ptr[1];
    ((unsigned char*)&ret)[7] = ptr[0];
  }
  ptr += 8;
  return ret;
}

unsigned long long read_u48(const unsigned char *&ptr)
{
  fprintf(stderr, "read_u48 %02x %02x %02x %02x %02x %02x\n",
          ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  unsigned long long ret =
    ((unsigned long long)ptr[0]<< 0) + ((unsigned long long) ptr[1]<< 8) +
    ((unsigned long long)ptr[2]<<16) + ((unsigned long long) ptr[3]<<24) +
    ((unsigned long long)ptr[4]<<32) + ((unsigned long long) ptr[5]<<40);
  ptr += 6;
  fprintf(stderr, "result is %llu\n", ret);
  return ret;
}


long long read_s40(const unsigned char *&ptr)
{
  long long ret =
    ((long long) ptr[0]<< 0) +
    ((long long) ptr[1]<< 8) +
    ((long long) ptr[2]<<16) +
    ((long long) ptr[3]<<24) +
    ((long long)((char*)ptr)[4]<<32);
  ptr += 5;
  return ret;
}

unsigned int read_u32(const unsigned char *&ptr)
{
  unsigned int ret = ptr[0] + (ptr[1]<<8) + (ptr[2]<<16) + (ptr[3]<<24);
  ptr += 4;
  return ret;
}

unsigned int read_u16(const unsigned char *&ptr)
{
  unsigned int ret = ptr[0] + (ptr[1]<<8);
  ptr += 2;
  return ret;
}

unsigned int read_u8(const unsigned char *&ptr)
{
  unsigned int ret = ptr[0];
  ptr += 1;
  return ret;
}

enum {
  RTYPE_START_OF_FILE = 1,
  RTYPE_RTC = 2,
  RTYPE_PERIODIC_DATA = 3
};

enum {
  TIME_DOUBLE = 1,
  TIME_TICKS32 = 2
};
  
bool verbose;

struct Source {
  const unsigned char *begin;
  Source(const unsigned char *b) : begin(b) {}
  int pos(const unsigned char *p) const { return p-begin; }
  int pos(const char *p) const { return (unsigned char*)p-begin; }
};

struct TimeRecord {
  unsigned char type;
  double time_double;
  unsigned int time_ticks32;
  TimeRecord() : type(0) {}
  TimeRecord(const Source &source, const unsigned char *&ptr) {
    type = *(ptr++);
    switch (type) {
    case TIME_DOUBLE:
      time_double = read_float64(ptr);
      break;
    case TIME_TICKS32:
      time_ticks32 = read_u32(ptr);
      break;
    default:
      throw ParseError("At byte %d: Unknown TimeRecord type 0x%02x", source.pos(ptr-1), type);
    }
  }
};

struct StartOfFileRecord {
  unsigned int protocol_version;
  TimeRecord time;
  long long tick_period; // in picoseconds
  std::map<std::string, std::string> device_params;
  StartOfFileRecord() : protocol_version(0), tick_period(0) {}
  StartOfFileRecord(const Source &source, const unsigned char *payload, int payload_len) {
    try {
      const unsigned char *ptr=payload;
      protocol_version = read_u16(ptr);
      time = TimeRecord(source, ptr);
      tick_period = read_u48(ptr);
      fprintf(stderr, "Parsing SOFR:  tick_period is %llu\n", tick_period);
      char *cptr = (char*) ptr, *end= (char*) payload+payload_len;
      if (end[-1] != 0) throw ParseError("At byte %d: DEVICE_PARAMS don't end with NUL", source.pos(end-1));
      while (cptr < end-1) {
        char *keyptr = cptr;
        cptr = strchr(cptr, '\t');
        if (!cptr) throw ParseError("At byte %d: DEVICE_PARAMS missing tab", source.pos(keyptr));
        std::string key(keyptr, cptr);
        cptr++; // skip tab
        char *valueptr = cptr;
        cptr = strchr(cptr, '\n');
        if (!cptr) throw ParseError("At byte %d: DEVICE_PARAMS missing newline", source.pos(valueptr));
        std::string value(valueptr, cptr);
        cptr++; // skip
        fprintf(stderr, "  '%s'='%s'\n", key.c_str(), value.c_str());
        device_params[key] = value;
      }
    }
    catch (ParseError &e) {
      throw ParseError("In RTYPE_START_OF_FILE payload starting at byte %d:\n%s", source.pos(payload), e.what());
    }
  }
};

struct RtcRecord {
  unsigned int tick_count;
  long long nanoseconds_since_1970; // UTC
  double seconds_since_1970() const {
    return nanoseconds_since_1970 / 1e9;
  }
  RtcRecord(const Source &source, const unsigned char *payload, int payload_len) {
    if (payload_len != 13) {
      throw ParseError("In RTYPE_RTC payload starting at byte %d, incorrect length (should be 13, is %d",
                       source.pos(payload), payload_len);
    }
    const unsigned char *ptr=payload;
    
    tick_count = read_u32(ptr);
    long long seconds_since_1970 = read_s40(ptr);
    unsigned int nanoseconds = read_u32(ptr);
    nanoseconds_since_1970 = seconds_since_1970 * 1000000000LL + nanoseconds;
  }
  std::string to_string() const {
    // I think this is wrong for negative times
    return string_printf("[RTC Record ticks=0x%x, seconds since 1970=%lld.%09d]",
                         tick_count,
                         nanoseconds_since_1970 / 1000000000LL,
                         (int)(nanoseconds_since_1970 % 1000000000));
  }
};

class TickToTime;

struct PeriodicDataRecord {
  unsigned int first_sample_short_tick;
  unsigned long long first_sample_long_tick;
  double first_sample_time;
  double last_sample_plus_one_time;
  unsigned int sample_period;  // in ticks
  unsigned int number_of_samples;
  std::string channel_signature;
  std::vector<std::pair<std::string, int> > channel_definitions;
  std::vector<unsigned char> data;

  unsigned int n_channels() const {
    return channel_definitions.size();
  }


  void get_samples(int ch, std::vector<double> &samples) const {
    samples.resize(number_of_samples);
    int total_bits = 0;
    int sample_start_bit = 0;
    for (unsigned i = 0; i < channel_definitions.size(); i++)
      total_bits += channel_definitions[i].second;
    for (int i = 0; i < ch; i++)
      sample_start_bit += channel_definitions[i].second;
    int sample_bits = channel_definitions[ch].second;
    assert(total_bits % 8 == 0);
    assert(sample_start_bit % 8 == 0);
    assert(sample_bits % 8 == 0);
    int offset = sample_start_bit / 8;
    int stride = total_bits / 8;
    unsigned int (*read)(const unsigned char *&ptr)=NULL;
    switch (sample_bits) {
    case 8:
      read = read_u8;
      break;
    case 16:
      read = read_u16;
      break;
    case 32:
      read = read_u32;
      break;
    default:
      throw ParseError("Don't know how to read channel with %d bits", sample_bits);
    }
    for (unsigned i = 0; i < number_of_samples; i++) {
      const unsigned char *ptr = &data[offset + i * stride];
      samples[i] = (*read)(ptr);
    }
  }

  const std::string &channel_name(int i) const {
    return channel_definitions[i].first;
  }
  
  unsigned int last_sample_tick() const {
    return first_sample_short_tick + sample_period * number_of_samples;
  }

  void set_time(const TickToTime &ttt);
  
  PeriodicDataRecord(const Source &source, const unsigned char *payload, int payload_len) {
    try {
      const unsigned char *ptr=payload, *end=payload+payload_len;
      first_sample_short_tick = read_u32(ptr);
      sample_period = read_u32(ptr);
      number_of_samples = read_u32(ptr);

      char *cdef_begin = (char*)ptr;
      char *cdef_end;
      for (cdef_end = cdef_begin; cdef_end < (char*)end && *cdef_end; cdef_end++) {}
      if (cdef_end >= (char*)end) throw ParseError("At byte %d: DEVICE_PARAMS don't end with NUL", source.pos(end-1));

      char *cptr = (char*) cdef_begin;
      channel_signature = std::string(cdef_begin, cdef_end-1);
      
      while (cptr < cdef_end) {
        char *keyptr = cptr;
        cptr = strchr(cptr, '\t');
        if (!cptr) throw ParseError("At byte %d: DEVICE_PARAMS missing tab", source.pos(keyptr));
        std::string key(keyptr, cptr);
        cptr++; // skip tab
        char *valueptr = cptr;
        cptr = strchr(cptr, '\n');
        if (!cptr) throw ParseError("At byte %d: DEVICE_PARAMS missing newline", source.pos(valueptr));
        std::string value(valueptr, cptr);
        cptr++; // skip
        int nbits = atoi(value.c_str());
        //fprintf(stderr, " '%s'=%d; ", key.c_str(), nbits);
        channel_definitions.push_back(std::pair<std::string, int>(key, nbits));
      }
      //fprintf(stderr, "\n");

      ptr = (unsigned char*)cdef_end+1;
      data = std::vector<unsigned char>(ptr, end);
    }
    catch (ParseError &e) {
      throw ParseError("In RTYPE_START_OF_FILE payload starting at byte %d:\n%s", source.pos(payload), e.what());
    }
  }

};
  
class TickToTime {
private:
  unsigned long long m_tick_period;  // in nanoseconds
  unsigned long long m_current_tick;
  double m_zero_tick_time; // time of the zeroth tick, in seconds since 1970 (UTC)
public:
  TickToTime() : m_tick_period(0), m_current_tick(0), m_zero_tick_time(0) {}
  unsigned long long current_tick() { return m_current_tick; }
  void receive_binrec(const StartOfFileRecord &sofr) {
    m_tick_period = sofr.tick_period;
    fprintf(stderr, "SOFR:  tick_period is %llu picoseconds\n", m_tick_period);
    fprintf(stderr, "SOFR:  tick_period is %g milliseconds\n", m_tick_period/1e9);
  }
  void receive_binrec(const RtcRecord &rtcr) {
    receive_short_ticks(rtcr.tick_count);
    fprintf(stderr, "\n");
    fprintf(stderr, "Current tick = %llu, tick period = %llu\n", m_current_tick, m_tick_period);
    fprintf(stderr, "Current tick = %llu, seconds = %g\n", m_current_tick, (m_current_tick * m_tick_period) / 1e12);
    double new_zero_tick_time = rtcr.seconds_since_1970() - (m_current_tick * m_tick_period) / 1e12;
    if (m_zero_tick_time == 0) {
      fprintf(stderr, "Setting zero_tick_time to %g\n", new_zero_tick_time);
    } else {
      fprintf(stderr, "Won't change zero_tick_time by %.9f from %.9f to %.9f\n",
              new_zero_tick_time - m_zero_tick_time, m_zero_tick_time, new_zero_tick_time);
      return;
    }
    m_zero_tick_time = new_zero_tick_time;
  }
  void receive_binrec(const PeriodicDataRecord &pdr) {
    receive_short_ticks(pdr.last_sample_tick());
  }
  void receive_short_ticks(unsigned int short_ticks) {
    if (m_current_tick == 0) {
      m_current_tick = short_ticks + 0x100000000;
    } else {
      // Find closest current_tick such that (current_tick % 2^32) == tick_count
      m_current_tick = short_ticks_to_long_ticks(short_ticks);
    }
  }
  unsigned long long short_ticks_to_long_ticks(unsigned int short_ticks) const {
    unsigned long long ret = (m_current_tick & 0xffffffff00000000LL) | short_ticks;
    long long err = ((m_current_tick - ret) + 0x80000000LL) / 0x100000000LL;
    return ret + err * 0x100000000LL;
  }
  double long_ticks_to_time(unsigned long long long_ticks) const {
    return m_zero_tick_time + (long_ticks * m_tick_period) / 1e12;
  }
};

void PeriodicDataRecord::set_time(const TickToTime &ttt) {
  first_sample_long_tick = ttt.short_ticks_to_long_ticks(first_sample_short_tick);
  first_sample_time = ttt.long_ticks_to_time(first_sample_long_tick);
  last_sample_plus_one_time = ttt.long_ticks_to_time(first_sample_long_tick + number_of_samples * sample_period);
}

void parse_bt_file(const std::string &infile)
{
  // Memory-map file
  FILE *in = fopen(infile.c_str(), "r");
  if (!in) throw std::runtime_error("fopen");
  struct stat statbuf;
  if (-1 == fstat(fileno(in), &statbuf)) throw std::runtime_error("fopen");
  long long len = statbuf.st_size;
  const unsigned char *in_mem = (unsigned char*) mmap(NULL, len, PROT_READ, MAP_SHARED/*|MAP_POPULATE*/, fileno(in), 0);
  const unsigned char *end = in_mem + len;
  if (in_mem == (unsigned char*)-1) throw std::runtime_error("mmap");
  fprintf(stderr, "Mapped %s (%lld KB)\n", infile.c_str(), len/1024);
  Source source(in_mem);
  const unsigned char *ptr = in_mem;

  int nrecords[256];
  memset(nrecords, 0, sizeof(nrecords));
  
  StartOfFileRecord sofr;
  TickToTime ttt;
  std::map<std::string, unsigned long long> last_tick;
  while (ptr < end) {
    const unsigned char *beginning_of_record = ptr;
    try {
      unsigned int magic = read_u32(ptr);
      //fprintf(stderr, "magic=0x%x\n", magic);
      if (magic != 0xb0de744c) throw ParseError("Incorrect magic # at byte %d", source.pos(ptr - 4));
      unsigned int record_size = read_u32(ptr);
      //fprintf(stderr, "record_size=%d\n", record_size);
      if (record_size + beginning_of_record > end) throw ParseError("Record size too long at byte %d (size=%d, but only %d bytes left in file)", source.pos(ptr - 4), record_size, end-beginning_of_record);
      int record_type = read_u16(ptr);
      if (record_type != RTYPE_START_OF_FILE && record_type != RTYPE_RTC && record_type != RTYPE_PERIODIC_DATA) {
        throw ParseError("Unknown record type 0x%x at byte %d", record_type, source.pos(ptr - 2));
      }
      const unsigned char *payload = ptr;
      unsigned int payload_len = record_size-14;
      ptr += payload_len;
      //fprintf(stderr, "Got record type %d, payload len %d\n", record_type, payload_len);
      unsigned int crc = read_u32(ptr);
      unsigned int calculated_crc = crc32(beginning_of_record, record_size - 4, 0);
      if (crc != calculated_crc) {
        throw ParseError("Incorrect CRC32 byte %d.  read 0x%x != calculated 0x%x\n",
                         source.pos(ptr - 4), crc, calculated_crc);
      }
      
      switch (record_type) {
      case RTYPE_START_OF_FILE:
        sofr = StartOfFileRecord(source, payload, payload_len);
        ttt.receive_binrec(sofr);
        break;
      case RTYPE_RTC:
      {
        RtcRecord rtcr(source, payload, payload_len);
        ttt.receive_binrec(rtcr);
        fprintf(stderr, "%s\n", rtcr.to_string().c_str());
      }
      break;
      case RTYPE_PERIODIC_DATA:
      {
        PeriodicDataRecord pdr(source, payload, payload_len);
        ttt.receive_binrec(pdr);
        pdr.set_time(ttt);
        for (unsigned i = 0; i < pdr.n_channels(); i++) {
          const std::string &channel_name = pdr.channel_name(i);
          fprintf(stderr,
                  "PDR channel %s nsamples %d nsamples_since_last %g start time %.9f end time(+1) %.9f:",
                  channel_name.c_str(), pdr.number_of_samples,
                  (double) (pdr.first_sample_long_tick - last_tick[channel_name]) / pdr.sample_period,
                  pdr.first_sample_time, pdr.last_sample_plus_one_time);
          std::vector<double> samples;
          pdr.get_samples(i, samples);
          for (unsigned i = 0; i < pdr.number_of_samples; i++) {
            fprintf(stderr, " %g", samples[i]);
          }
          fprintf(stderr, "\n");
          
          last_tick[channel_name] = pdr.first_sample_long_tick;
        }
      }
      break;
      default:
        assert(0);
      }
      nrecords[record_type]++;
    }
    catch (ParseError &e) {
      throw ParseError("In record starting at byte %d in file %s:\n%s",
                       source.pos(beginning_of_record), infile.c_str(), e.what());
    }
  }
    
  if (-1 == munmap((void*)in_mem, len)) { perror("munmap"); exit(1); }
  fclose(in);
  fprintf(stderr, "Finished parsing\n");
  fprintf(stderr, "%d RTYPE_START_OF_FILE records\n", nrecords[RTYPE_START_OF_FILE]);
  fprintf(stderr, "%d RTYPE_RTC records\n", nrecords[RTYPE_RTC]);
  fprintf(stderr, "%d RTYPE_PERIODIC_DATA records\n", nrecords[RTYPE_PERIODIC_DATA]);
}

void test_tick_to_time()
{
  TickToTime ttt;
  ttt.receive_short_ticks(0);
  assert(ttt.current_tick() == 0x100000000);
  ttt.receive_short_ticks(1);
  assert(ttt.current_tick() == 0x100000001);
  ttt.receive_short_ticks(0xffffffff);
  assert(ttt.current_tick() == 0x0FFFFFFFF);
  ttt.receive_short_ticks(0);
  assert(ttt.current_tick() == 0x100000000);
  ttt.receive_short_ticks(0x7fffffff);
  assert(ttt.current_tick() == 0x17fffffffLL);
  ttt.receive_short_ticks(0);
  assert(ttt.current_tick() == 0x100000000);
  ttt.receive_short_ticks(0x7fffffff);
  assert(ttt.current_tick() == 0x17fffffffLL);
  ttt.receive_short_ticks(0x80000000);
  assert(ttt.current_tick() == 0x180000000LL);
  ttt.receive_short_ticks(0);
  assert(ttt.current_tick() == 0x200000000LL);
}

int main(int argc, char **argv)
{
  test_tick_to_time();
  parse_bt_file("front-porch-test/4dbc7356.bt");
  return(0);
}
