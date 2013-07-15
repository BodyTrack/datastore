#ifndef MAP_SERIALIZER_INCLUDE_H
#define MAP_SERIALIZER_INCLUDE_H

// C
#include <cassert>

// C++
#include <map>
#include <sstream>
#include <string>

// TODO: Template this to support maps other than string -> string
// TODO: Use some scheme like URL encoding to allow the caller
// to pass keys and values containing the elem separator and kv separator
class MapSerializer {
public:
  MapSerializer() : elem_sep("&"), kv_sep("=") { }
  MapSerializer(const std::string elem_sep, const std::string kv_sep)
      : elem_sep(elem_sep), kv_sep(kv_sep) {
    assert (!elem_sep.empty());
    assert (!kv_sep.empty());
  }

  std::string serialize(const std::map<std::string, std::string> m) const {
    typedef std::map<std::string, std::string>::const_iterator map_it;

    std::stringstream ss;
    bool first_elem = true;
    for (map_it it = m.begin(); it != m.end(); it++) {
      std::string key = it->first;
      std::string value = it->second;
      check_separators("Key", key);
      check_separators("Value", value);

      if (first_elem)
        first_elem = false;
      else
        ss << elem_sep;
      ss << key << kv_sep << value;
    }
    return ss.str();
  }

  std::map<std::string, std::string> deserialize(const std::string serialized) const {
    std::vector<std::string> elems;
    split(serialized, elem_sep, elems);

    std::map<std::string, std::string> m;
    for (unsigned idx = 0; idx < elems.size(); idx++) {
      std::vector<std::string> kv;
      split(elems[idx], kv_sep, kv);
      if (kv.size() != 2)
        throw std::runtime_error("Key-value pair '" + elems[idx]
            + "' does not contain exactly one key-value separator "
            + kv_sep);
      m.insert(std::pair<std::string, std::string>(kv[0], kv[1]));
    }

    return m;
  }

private:
  std::string elem_sep, kv_sep;

  void check_separators(const std::string item_name,
      const std::string item) const {
    if (item.find(elem_sep) != std::string::npos)
      throw std::runtime_error(item_name + " '" + item
          + "' contains element separator " + elem_sep);
    if (item.find(kv_sep) != std::string::npos)
      throw std::runtime_error(item_name + " '" + item
          + "' contains key-value separator " + kv_sep);
  }

  void split(const std::string s,
      const std::string delim,
      std::vector<std::string> &results) const {
    assert (!delim.empty());
    size_t delim_length = delim.size();

    size_t curr_pos = 0;
    while (curr_pos < s.size()) {
      size_t delim_pos = s.find(delim, curr_pos);
      if (delim_pos != curr_pos) { // Skip empty substrings
        if (delim_pos == std::string::npos) {
          results.push_back(s.substr(curr_pos));
          return;
        }
        results.push_back(s.substr(curr_pos, delim_pos - curr_pos));
      }
      curr_pos = delim_pos + delim_length;
    }
  }
};

#endif /* MAP_SERIALIZER_INCLUDE_H */

