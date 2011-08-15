#ifndef INCLUDE_PARSE_H
#define INCLUDE_PARSE_H

// C++
#include <string>

// JSON
#include <json/json.h>

// Local
#include "DataSample.h"

class ParseError : public std::exception {
  std::string m_what;
public:
  ParseError(const char *fmt, ...);
  virtual const char* what() const throw();
  virtual ~ParseError() throw();
};

struct ParseInfo {
  DataRanges imported_data;
  DataRanges channel;
  int good_records;
  int bad_records;
  Json::Value channel_specs;
};

#endif
