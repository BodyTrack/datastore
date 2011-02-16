#ifndef CHANNEL_INCLUDE_H
#define CHANNEL_INCLUDE_H

#include <vector>
#include <string>

#include "Logrec.h"

class Channel {
public:
  Channel(int user_id, std::string dev_nickname, std::string ch_name);
  void seek(double t);
  bool fetch_next(double &t, double &val);
  static bool s_debug;
private:
  int m_user_id;
  std::string m_dev_nickname;
  std::string m_ch_name;
  
  double m_current_time;  // current time for fetch_next/prev
  double m_seek_time;     // time of last seek.  NOT necessarily current time
  int m_next_fetch_seq;
  
  // TODO: make this a shared ptr;  currently we leak instead
  std::vector<Logrec*> m_logrecs;
  double m_begin_valid, m_end_valid;

  void fetch_next_logrecs();
  int fetch_logrecs_and_seek(std::vector<Logrec*> &logrecs, MysqlQuery &query, double seek);

};

#endif
