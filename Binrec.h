#ifndef INCLUDE_BINREC_H
#define INCLUDE_BINREC_H

// C++
#include <map>
#include <stdexcept>
#include <vector>

// JSON
#include <json/json.h>

// Local
#include "DataSample.h"
#include "Parse.h"
#include "simple_shared_ptr.h"

struct Source {
  const unsigned char *begin;
  Source(const unsigned char *b) : begin(b) {}
  int pos(const unsigned char *p) const { return p-begin; }
  int pos(const char *p) const { return (unsigned char*)p-begin; }
};

enum {
  RTYPE_START_OF_FILE = 1,
  RTYPE_RTC = 2,
  RTYPE_PERIODIC_DATA = 3
};

enum {
  TIME_DOUBLE = 1,
  TIME_TICKS32 = 2
};
  
struct TimeRecord {
  unsigned char type;
  double time_double;
  unsigned int time_ticks32;
  TimeRecord();
  TimeRecord(const Source &source, const unsigned char *&ptr);
};

struct StartOfFileRecord {
  unsigned int protocol_version;
  TimeRecord time;
  long long tick_period; // in picoseconds
  std::map<std::string, std::string> device_params;
  Json::Value channel_specs;
  StartOfFileRecord();
  StartOfFileRecord(const Source &source, const unsigned char *payload, int payload_len);
  std::string get_channel_units(const std::string &channel_name) const;
  double get_channel_scale(const std::string &channel_name) const;
};

struct RtcRecord {
  unsigned int tick_count;
  long long nanoseconds_since_1970; // UTC
  RtcRecord(const Source &source, const unsigned char *payload, int payload_len);
  double seconds_since_1970() const;
  std::string to_string() const;
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
  const StartOfFileRecord *start_of_file_record;

  PeriodicDataRecord(const Source &source, const unsigned char *payload, int payload_len,
                     const StartOfFileRecord *sofr);
  unsigned int n_channels() const;

  void get_samples(int ch, std::vector<double> &samples) const;
  void get_data_samples(int ch, std::vector<DataSample<double> > &data_samples) const;

    
  const std::string &channel_name(int i) const;
  unsigned int last_sample_tick() const;
  void set_time(const TickToTime &ttt);
};

class TickToTime {
private:
  unsigned long long m_tick_period;  // in nanoseconds
  unsigned long long m_current_tick;
  double m_zero_tick_time; // time of the zeroth tick, in seconds since 1970 (UTC)
public:
  TickToTime();
  unsigned long long current_tick();
  void receive_binrec(const StartOfFileRecord &sofr);
  void receive_binrec(const RtcRecord &rtcr);
  void receive_binrec(const PeriodicDataRecord &pdr);
  void receive_short_ticks(unsigned int short_ticks);
  unsigned long long short_ticks_to_long_ticks(unsigned int short_ticks) const;
  double long_ticks_to_time(unsigned long long long_ticks) const;
};

class BtParserReceiver {
public:
  virtual void receive_data_samples(const std::string &channel_name,
                                    const std::vector<DataSample<double> > &samples) = 0;
};

void parse_bt_file(const std::string &infile,
                   std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > > &data,
                   std::vector<ParseError> &errors,
		   ParseInfo &info);

extern bool verbose;

#endif
