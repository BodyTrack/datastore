#ifndef INCLUDE_PARSE_H
#define INCLUDE_PARSE_H

// C++
#include <string>

// JSON
#include <json/json.h>

class ParseError : public std::exception {
  std::string m_what;
public:
  ParseError(const char *fmt, ...);
  virtual const char* what() const throw();
  virtual ~ParseError() throw();
};

struct ParseInfo {
  double min_time;
  double max_time;
  int good_records;
  int bad_records;
  Json::Value channel_specs;
};

#endif
