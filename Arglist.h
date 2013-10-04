#ifndef ARGLIST_H
#define ARGLIST_H

#include <list>
#include <string>

void usage(const char *fmt, ...);

class Arglist : public std::list<std::string> {
public:
  Arglist(char **begin, char **end) {
    for (char **arg = begin; arg < end; arg++) push_back(std::string(*arg));
  }
  std::string shift() {
    if (empty()) usage("Missing argument");
    std::string ret = front();
    pop_front();
    return ret;
  }
  double shift_double() {
    std::string arg = shift();
    char *end;
    double ret = strtod(arg.c_str(), &end);
    if (end != arg.c_str() + arg.length()) {
      usage("Can't parse '%s' as floating point value", arg.c_str());
    }
    return ret;
  }
  int shift_int() {
    return parse_int(shift());
  }
  std::string to_string() const {
    std::string ret;
    for (std::list<std::string>::const_iterator i = begin(); i != end(); ++i) {
      if (ret.length()) ret += " ";
      ret += "'" + *i + "'";
    }
    return ret;
  }
  static bool is_flag(const std::string &arg) {
    return arg.length() > 0 && arg[0] == '-';
  }
  static int parse_int(const std::string &arg) {
    char *end;
    int ret = strtol(arg.c_str(), &end, 0);
    if (end != arg.c_str() + arg.length()) {
      usage("Can't parse '%s' as integer", arg.c_str());
    }
    return ret;
  }
};

#endif
