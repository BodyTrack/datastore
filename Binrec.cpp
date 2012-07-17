// C
#include <assert.h>
#include <stdio.h>
#include <string.h>

// C++
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// System
#include <sys/stat.h>
#include <sys/mman.h>


// Local
#include "crc32.h"
#include "DataSample.h"
#include "KVS.h"
#include "Log.h"
#include "simple_shared_ptr.h"
#include "utils.h"

// Self
#include "Binrec.h"

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
  unsigned long long ret =
    ((unsigned long long)ptr[0]<< 0) + ((unsigned long long) ptr[1]<< 8) +
    ((unsigned long long)ptr[2]<<16) + ((unsigned long long) ptr[3]<<24) +
    ((unsigned long long)ptr[4]<<32) + ((unsigned long long) ptr[5]<<40);
  ptr += 6;
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

bool verbose;

ParseError::ParseError(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  m_what = string_vprintf(fmt, args);
  va_end(args);
}

const char* ParseError::what() const throw() { return m_what.c_str(); }
ParseError::~ParseError() throw() {}

TimeRecord::TimeRecord() : type(0) {}

TimeRecord::TimeRecord(const Source &source, const unsigned char *&ptr) {
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

StartOfFileRecord::StartOfFileRecord() : protocol_version(0), tick_period(0) {}
StartOfFileRecord::StartOfFileRecord(const Source &source, const unsigned char *payload, int payload_len) {
  try {
    const unsigned char *ptr=payload;
    protocol_version = read_u16(ptr);
    time = TimeRecord(source, ptr);
    tick_period = read_u48(ptr);
    if (verbose) log_f("parse_bt_file: Parsing SOFR:  tick_period is %llu", tick_period);
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
      if (verbose) log_f("parse_bt_file:   '%s'='%s'", key.c_str(), value.c_str());
      device_params[key] = value;
    }
    
    if (!device_params.count("channel_specs")) {
      throw ParseError("No channel_specs field in DEVICE_PARAMS in START_OF_FILE record");
    }
    Json::Reader reader;
    if (!reader.parse(device_params["channel_specs"], channel_specs)) {
      throw ParseError("Failed to parse channel_specs JSON");
    }
    Json::Value::Members channel_names = channel_specs.getMemberNames();
    for (unsigned i = 0; i < channel_names.size(); i++) {
      std::string units = get_channel_units(channel_names[i]);
      double scale = get_channel_scale(channel_names[i]);
      if (verbose)
        log_f("parse_bt_file:   channel %d: '%s', units '%s', scale %g",
                i, channel_names[i].c_str(), units.c_str(), scale);
    }
  }
  catch (ParseError &e) {
    throw ParseError("In RTYPE_START_OF_FILE payload starting at byte %d: %s", source.pos(payload), e.what());
  }
}

std::string StartOfFileRecord::get_channel_units(const std::string &channel_name) const {
  return channel_specs[channel_name]["units"].asString();
}

double StartOfFileRecord::get_channel_scale(const std::string &channel_name) const {
  return channel_specs[channel_name]["scale"].asDouble();
}

double RtcRecord::seconds_since_1970() const {
  return nanoseconds_since_1970 / 1e9;
}

RtcRecord::RtcRecord(const Source &source, const unsigned char *payload, int payload_len) {
  if (payload_len != 13) {
    throw ParseError("In RTYPE_RTC payload starting at byte %d, incorrect length (should be 13, is %d)",
                     source.pos(payload), payload_len);
  }
  const unsigned char *ptr=payload;
  
  tick_count = read_u32(ptr);
  long long seconds_since_1970 = read_s40(ptr);
  unsigned int nanoseconds = read_u32(ptr);
  nanoseconds_since_1970 = seconds_since_1970 * 1000000000LL + nanoseconds;
}

std::string RtcRecord::to_string() const {
  assert(nanoseconds_since_1970 >= 0); // TODO: fix this for negative times and take out this assertion
  return string_printf("[RTC Record ticks=0x%x, seconds since 1970=%lld.%09d]",
                       tick_count,
                       nanoseconds_since_1970 / 1000000000LL,
                       (int)(nanoseconds_since_1970 % 1000000000));
}

PeriodicDataRecord::PeriodicDataRecord(const Source &source, const unsigned char *payload,
                                       int payload_len, const StartOfFileRecord *sofr)
  : start_of_file_record(sofr)
{
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
      if (verbose) log_f("parse_bt_file:  '%s'=%d", key.c_str(), nbits);
      channel_definitions.push_back(std::pair<std::string, int>(key, nbits));
    }
    
    ptr = (unsigned char*)cdef_end+1;
    data = std::vector<unsigned char>(ptr, end);
  }
  catch (ParseError &e) {
    throw ParseError("In RTYPE_START_OF_FILE payload starting at byte %d: %s", source.pos(payload), e.what());
  }
}

unsigned int PeriodicDataRecord::n_channels() const {
  return channel_definitions.size();
}

void PeriodicDataRecord::get_samples(int ch, std::vector<double> &samples) const {
  double scale = start_of_file_record->get_channel_scale(channel_definitions[ch].first);
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
    samples[i] = (*read)(ptr) * scale;
  }
}

void PeriodicDataRecord::get_data_samples(int ch, std::vector<DataSample<double> > &data_samples) const
{
  std::vector<double> samples;
  get_samples(ch, samples);
  data_samples.resize(samples.size());
  for (unsigned i = 0; i < samples.size(); i++) {
    double frac = (double)i / samples.size();
    data_samples[i].time = (1-frac)*first_sample_time + frac*last_sample_plus_one_time;
    data_samples[i].value = samples[i];
  }
}

const std::string &PeriodicDataRecord::channel_name(int i) const {
  return channel_definitions[i].first;
}

unsigned int PeriodicDataRecord::last_sample_tick() const {
  return first_sample_short_tick + sample_period * number_of_samples;
}

TickToTime::TickToTime() : m_tick_period(0), m_current_tick(0), m_zero_tick_time(0) {}

unsigned long long TickToTime::current_tick() { return m_current_tick; }

void TickToTime::receive_binrec(const StartOfFileRecord &sofr) {
  m_tick_period = sofr.tick_period;
  if (verbose)
    log_f("parse_bt_file: SOFR:  tick_period is %llu picoseconds (%g microseconds, %g MHz)",
            m_tick_period, m_tick_period/1e6, 1e6/m_tick_period);
}

void TickToTime::receive_binrec(const RtcRecord &rtcr) {
  receive_short_ticks(rtcr.tick_count);
  if (verbose) {
    log_f("parse_bt_file: Current tick = %llu, tick period = %llu", m_current_tick, m_tick_period);
    log_f("parse_bt_file: Current tick = %llu, seconds = %g", m_current_tick, (m_current_tick * m_tick_period) / 1e12);
  }
  double new_zero_tick_time = rtcr.seconds_since_1970() - (m_current_tick * m_tick_period) / 1e12;
  if (m_zero_tick_time == 0) {
    if (verbose)
      log_f("parse_bt_file: RTC record: Setting zero_tick_time to %.9f", new_zero_tick_time);
  } else {
    if (verbose) {
      log_f("parse_bt_file: Won't change zero_tick_time by %.9f from %.9f to %.9f",
              new_zero_tick_time - m_zero_tick_time, m_zero_tick_time, new_zero_tick_time);
    }
    return;
  }
  m_zero_tick_time = new_zero_tick_time;
}

void TickToTime::receive_binrec(const PeriodicDataRecord &pdr) {
  receive_short_ticks(pdr.last_sample_tick());
}

void TickToTime::receive_short_ticks(unsigned int short_ticks) {
  if (m_current_tick == 0) {
    m_current_tick = short_ticks + 0x100000000LL;
  } else {
    // Find closest current_tick such that (current_tick % 2^32) == tick_count
    m_current_tick = short_ticks_to_long_ticks(short_ticks);
  }
}

unsigned long long TickToTime::short_ticks_to_long_ticks(unsigned int short_ticks) const {
  unsigned long long ret = (m_current_tick & 0xffffffff00000000LL) | short_ticks;
  long long err = ((m_current_tick - ret) + 0x80000000LL) / 0x100000000LL;
  return ret + err * 0x100000000LL;
}

double TickToTime::long_ticks_to_time(unsigned long long long_ticks) const {
  return m_zero_tick_time + (long_ticks * m_tick_period) / 1e12;
}

void PeriodicDataRecord::set_time(const TickToTime &ttt) {
  first_sample_long_tick = ttt.short_ticks_to_long_ticks(first_sample_short_tick);
  first_sample_time = ttt.long_ticks_to_time(first_sample_long_tick);
  last_sample_plus_one_time = ttt.long_ticks_to_time(first_sample_long_tick + number_of_samples * sample_period);
  if (verbose)
    log_f("parse_bt_file: PDR::set_time %.9f-%.9f", first_sample_time, last_sample_plus_one_time);
}


void parse_bt_file(const std::string &infile,
                   std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > > &data,
                   std::vector<ParseError> &errors,
		   ParseInfo &info)
{
  info.good_records = 0;
  info.bad_records = 0;
  double begintime = doubletime();
  // Memory-map file
  FILE *in = fopen(infile.c_str(), "rb");
  if (!in) throw std::runtime_error("fopen");
  struct stat statbuf;
  if (-1 == fstat(fileno(in), &statbuf)) throw std::runtime_error("fopen");
  long long len = statbuf.st_size;
  const unsigned char *in_mem = (unsigned char*) mmap(NULL, len, PROT_READ, MAP_SHARED/*|MAP_POPULATE*/, fileno(in), 0);
  const unsigned char *end = in_mem + len;
  if (in_mem == (unsigned char*)-1) throw std::runtime_error("mmap");
  if (verbose) log_f("parse_bt_file: Mapped %s (%lld KB)", infile.c_str(), len/1024);
  Source source(in_mem);
  const unsigned char *ptr = in_mem;

  int nrecords[256];
  memset(nrecords, 0, sizeof(nrecords));
  long long nvalues=0;
  
  StartOfFileRecord sofr;
  TickToTime ttt;

  std::map<std::string, unsigned long long> last_tick;
  bool out_of_order = false;
  
  while (ptr < end) {
    const unsigned char *beginning_of_record = ptr;
    try {
      unsigned int magic = read_u32(ptr);
      if (magic != 0xb0de744c) throw ParseError("Incorrect magic # at byte %d", source.pos(ptr - 4));
      unsigned int record_size = read_u32(ptr);
      if (verbose) {
        log_f("parse_bt_file:  At location %d, magic=0x%x, record size %d",
              (int)(beginning_of_record - in_mem), magic, record_size);
      }
      if (record_size + beginning_of_record > end) throw ParseError("Record size too long at byte %d (size=%d, but only %d bytes left in file)", source.pos(ptr - 4), record_size, end-beginning_of_record);
      int record_type = read_u16(ptr);
      if (record_type != RTYPE_START_OF_FILE && record_type != RTYPE_RTC && record_type != RTYPE_PERIODIC_DATA) {
        throw ParseError("Unknown record type 0x%x at byte %d", record_type, source.pos(ptr - 2));
      }
      const unsigned char *payload = ptr;
      unsigned int payload_len = record_size-14;
      ptr += payload_len;
      if (verbose) log_f("parse_bt_file: Got record type %d, payload len %d", record_type, payload_len);
      unsigned int crc = read_u32(ptr);
      unsigned int calculated_crc = crc32(beginning_of_record, record_size - 4, 0);
      if (crc != calculated_crc) {
        // Recoverable error;  add to errors and try to continue
        errors.push_back(ParseError("Incorrect CRC32 byte %d.  read 0x%x != calculated 0x%x",
                                    source.pos(ptr - 4), crc, calculated_crc));
	if (record_type == RTYPE_PERIODIC_DATA) info.bad_records++;
        continue;
      }

      switch (record_type) {
      case RTYPE_START_OF_FILE:
        sofr = StartOfFileRecord(source, payload, payload_len);
        ttt.receive_binrec(sofr);
	info.channel_specs = sofr.channel_specs;
        break;
      case RTYPE_RTC:
      {
        RtcRecord rtcr(source, payload, payload_len);
        ttt.receive_binrec(rtcr);
        if (verbose) log_f("parse_bt_file: %s", rtcr.to_string().c_str());
      }
      break;
      case RTYPE_PERIODIC_DATA:
      {
        PeriodicDataRecord pdr(source, payload, payload_len, &sofr);
        ttt.receive_binrec(pdr);
        pdr.set_time(ttt);
        for (unsigned i = 0; i < pdr.n_channels(); i++) {
          std::string channel_name = pdr.channel_name(i);
          nvalues += pdr.number_of_samples;
          log_f("parse_pt_file: %d samples, start tick 0x%x (%u)",
                pdr.number_of_samples, pdr.first_sample_short_tick, pdr.first_sample_short_tick);
          std::vector<DataSample<double> > data_samples;
          pdr.get_data_samples(i, data_samples);

          if (data_samples.size()) {
            if (data.find(channel_name) == data.end()) {
              data[channel_name].reset(new std::vector<DataSample<double> >());
            } else {
              if (data[channel_name]->back().time > data_samples.front().time) {
                if (verbose) log_f("Warning: sample times in channel %s are out-of-order (%f > %f)",
                                   channel_name.c_str(),
                                   data[channel_name]->back().time, data_samples.front().time);
                out_of_order = true;
              }
            }
            
            data[channel_name]->insert(data[channel_name]->end(), data_samples.begin(), data_samples.end());
	    info.good_records++;
          }
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
      errors.push_back(ParseError("In record starting at byte %d in file %s: %s",
                                  source.pos(beginning_of_record), infile.c_str(), e.what()));
      info.bad_records++;
    }
  }
  
  if (-1 == munmap((void*)in_mem, len)) { perror("munmap"); exit(1); }
  fclose(in);
  
  if (out_of_order) {
    for (std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > >::iterator i =
           data.begin(); i != data.end(); ++i) {
      
      simple_shared_ptr<std::vector<DataSample<double > > > samples = i->second;
      std::sort(samples->begin(), samples->end(), DataSample<double>::time_lessthan);
    }
  }

  // Check samples are in order
  for (std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > >::iterator i =
         data.begin(); i != data.end(); ++i) {
    
    simple_shared_ptr<std::vector<DataSample<double > > > samples = i->second;
    for (unsigned i = 0; i < samples->size()-1; i++) {
      assert((*samples)[i].time <= ((*samples)[i+1].time));
    }
  }

  double duration = doubletime() - begintime;
  log_f("parse_bt_file: Parsed %lld bytes in %g seconds (%dK/sec)", len, duration, (int)(len / duration / 1024));
  if (verbose) {
    log_f("parse_bt_file: %d RTYPE_START_OF_FILE records", nrecords[RTYPE_START_OF_FILE]);
    log_f("parse_bt_file: %d RTYPE_RTC records", nrecords[RTYPE_RTC]);
    log_f("parse_bt_file: %d RTYPE_PERIODIC_DATA records", nrecords[RTYPE_PERIODIC_DATA]);
    log_f("parse_bt_file:    %lld values", nvalues);
  }
}

// void test_tick_to_time()
// {
//   TickToTime ttt;
//   ttt.receive_short_ticks(0);
//   assert(ttt.current_tick() == 0x100000000);
//   ttt.receive_short_ticks(1);
//   assert(ttt.current_tick() == 0x100000001);
//   ttt.receive_short_ticks(0xffffffff);
//   assert(ttt.current_tick() == 0x0FFFFFFFF);
//   ttt.receive_short_ticks(0);
//   assert(ttt.current_tick() == 0x100000000);
//   ttt.receive_short_ticks(0x7fffffff);
//   assert(ttt.current_tick() == 0x17fffffffLL);
//   ttt.receive_short_ticks(0);
//   assert(ttt.current_tick() == 0x100000000);
//   ttt.receive_short_ticks(0x7fffffff);
//   assert(ttt.current_tick() == 0x17fffffffLL);
//   ttt.receive_short_ticks(0x80000000);
//   assert(ttt.current_tick() == 0x180000000LL);
//   ttt.receive_short_ticks(0);
//   assert(ttt.current_tick() == 0x200000000LL);
// }
// 
