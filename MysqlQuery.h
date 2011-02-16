#ifndef MYSQL_QUERY_INCLUDE_H
#define MYSQL_QUERY_INCLUDE_H

#include <string>
#include <map>
#include <vector>

#include <mysql.h>

class MysqlRow : public std::map<std::string, std::string> {
 public:
  void get(std::string &ret, const std::string &key) { ret = (*this)[key]; }
  void get(int &ret, const std::string &key) { ret = atoi((*this)[key].c_str()); }
  void get(long long &ret, const std::string &key) { ret = atoll((*this)[key].c_str()); }
  void get(double &ret, const std::string &key) { ret = atof((*this)[key].c_str()); }
  long long get_long_long(const std::string &key) { return atoll((*this)[key].c_str()); }
};

class MysqlQuery {
 public:
  typedef std::map<std::string, std::string> row;
  MysqlQuery(std::string fmt, ...);
  ~MysqlQuery();
  MysqlRow *fetch_row();
  static bool s_debug;
 private:
  MYSQL_RES *m_result;
  std::vector<std::string> m_fieldnames;
  MysqlRow m_current_row;

  int m_rowcount;
};  

#endif
