#ifndef LOGREC_INCLUDE_H
#define LOGREC_INCLUDE_H

#include <vector>

#include "MysqlQuery.h"

class Logrec {
public:
  long long id;
  std::string type;
  int user_id;
  std::string dev_id;
  std::string device_id;
  std::string dev_nickname;
  std::string device_class;
  std::string firmware_version;
  double begin_d;
  double end_d;
  std::string dat_fields;
  int num_datapoints;
  std::string comment;
  std::vector<std::string> fieldnames;
  
  Logrec(MysqlRow *row, const std::string &ch_name);
  static bool s_debug;
  static std::string s_csv_root;
  void seek(double t);
  bool next_time(double &t, bool force_read=false);
  bool fetch_next(double &t, double &val);
  std::string csv_filename();
  std::string summary();
private:
  // Fetch state

  bool m_csv_valid;
  
  // for now a vector we fill from parsing the CSV file; someday let's mmap a binary file here
  std::vector<double> m_csv_data; 
  
  int m_csv_row;
  int m_csv_rows;
  int m_csv_cols;
  void read_csv();
  double m_current_time;

  std::string m_fetch_ch_name;
  int m_fetch_index;
};
  
#endif
