#include "MysqlQuery.h"

#include <cfloat>
#include <assert.h>
#include <iostream>

#include "channel.h"

bool Channel::s_debug = true;

Channel::Channel(int user_id, std::string dev_nickname, std::string ch_name)
  : m_user_id(user_id), m_dev_nickname(dev_nickname), m_ch_name(ch_name), m_seek_time(-1)
{
}

void Channel::seek(double seek_time)
{
  // Find all logrecs overlapping time

  m_seek_time = m_current_time = seek_time;
  m_logrecs.clear();
  MysqlQuery overlap_q("select * from Logrecs where user_id = %d and dev_nickname = '%s' and "
                       "begin_d <= %lf and end_d >= %lf and "
                       "dat_fields LIKE '%%- %s%%' OR dat_fields LIKE '%%name: %s%%'",
                       m_user_id, m_dev_nickname.c_str(),
                       m_seek_time, m_seek_time,
                       m_ch_name.c_str(), m_ch_name.c_str());

  fetch_logrecs_and_seek(m_logrecs, overlap_q, seek_time);
  m_begin_valid = m_end_valid = m_seek_time;
  m_next_fetch_seq = 0;
}
  
//  MysqlQuery prev_q("select * from Logrecs where user_id = %d and dev_nickname = '%s' and "
//                    "end_d < %lf and "
//                    "dat_fields LIKE '%%- %s%%' OR dat_fields LIKE '%%name: %s%%' "
//                    "order by end_d desc limit 10",
//                    m_user_id, m_dev_nickname.c_str(),
//                    seek_time,
//                    ch_name.c_str(), ch_name.c_str());
//  
//  Logrec::logrecs_from_query(m_prev_logrecs, prev_q);


bool Channel::fetch_next(double &t, double &val)
{
  assert(m_seek_time >= 0);
  double best_time = DBL_MAX;
  Logrec *best_logrec = NULL;

  while (1) {
    for (unsigned i = 0; i < m_logrecs.size(); i++) {
      double test_time;
      if (m_logrecs[i]->next_time(test_time) && test_time < best_time) {
        best_logrec = m_logrecs[i];
        best_time = test_time;
      }
    }
    
    if (best_time >= m_end_valid) {
      fetch_next_logrecs();
      continue;
    }

    if (best_logrec) {
      //std::cerr << "best logrec id is " << best_logrec->id << std::endl;
      bool success = best_logrec->fetch_next(t, val);
      m_current_time = t;
      assert(success);
      return true;
    } else {
      return false;
    }
  }
}

// TODO: limit record ID to make sure new records don't mess us up
void Channel::fetch_next_logrecs() {
  int max_fetch = 10;

  if (s_debug) std::cerr << "Fetch next logrecs " << m_next_fetch_seq << " to " << m_next_fetch_seq + max_fetch - 1 << ":  m_end_valid was " << m_end_valid << std::endl;
  
  MysqlQuery next_q("select * from Logrecs where user_id = %d and dev_nickname = '%s' and "
                    "begin_d > %lf and "
                    "dat_fields LIKE '%%- %s%%' OR dat_fields LIKE '%%name: %s%%' "
                    "order by begin_d limit %d,%d",
                    m_user_id, m_dev_nickname.c_str(),
                    m_seek_time,
                    m_ch_name.c_str(), m_ch_name.c_str(),
                    m_next_fetch_seq, max_fetch);
  
  int count = fetch_logrecs_and_seek(m_logrecs, next_q, m_current_time);
  m_next_fetch_seq += count;
  if (s_debug) std::cerr << "Fetch next logrecs:  got " << count << " more logrecs\n";
  if (count < max_fetch) {
    // We have them all;  valid through end of time
    m_end_valid = DBL_MAX;
  } else {
    for (unsigned i = 0; i < m_logrecs.size(); i++) {
      m_end_valid = std::max(m_end_valid, m_logrecs[i]->begin_d);
    }
  }
  if (s_debug) fprintf(stderr, "Fetch next logrecs:  m_end_valid now %f\n", m_end_valid);
}
    
int Channel::fetch_logrecs_and_seek(std::vector<Logrec*> &logrecs, MysqlQuery &query, double seek) {
  MysqlRow *row;
  int count = 0;
  while ((row = query.fetch_row())) {
    Logrec *lr = new Logrec(row, m_ch_name);
    lr->seek(seek);
    logrecs.push_back(lr);
    count++;
  }
  if (s_debug) std::cerr << "fetch_logrecs returns " << count << " logrecs\n";
  return count;
}


  
  
