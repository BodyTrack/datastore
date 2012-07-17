#ifndef INCLUDE_IMPORT_JSON_H
#define INCLUDE_IMPORT_JSON_H

// C++
#include <string>

// Local includes
#include "KVS.h"
#include "Parse.h"

void parse_json_file(const std::string &infile,
		     std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > > &numeric_data,
		     std::map<std::string, simple_shared_ptr<std::vector<DataSample<std::string> > > > &string_data,
		     std::vector<ParseError> &errors,
		     ParseInfo &info);

#endif
