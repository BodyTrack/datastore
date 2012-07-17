// C++
#include <map>
#include <vector>
#include <fstream>
#include <limits>

// C
#include <stdio.h>

#include "simple_shared_ptr.h"

// Local
#include "Channel.h"
#include "DataSample.h"

// Self
#include "ImportJson.h"


template <typename A, typename B>
void erase_empty_data(std::map<A, B> &m) {

  for (typename std::map<A, B>::iterator i = m.begin(); i != m.end();) {
    if (!i->second->size()) {
      m.erase(i++);
    } else {
      i++;
    }
  }
}

void parse_json_single(Json::Value json,
		       std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > > &numeric_data,
		       std::map<std::string, simple_shared_ptr<std::vector<DataSample<std::string> > > > &string_data,
		       std::vector<ParseError> &errors,
		       ParseInfo &info)
{
  Json::Value channel_names = json["channel_names"];
  Json::Value numeric_channel_names = json["numeric_ch_names"];

  std::vector<simple_shared_ptr<std::vector<DataSample<double> > > > vector_numeric_data;
  std::vector<simple_shared_ptr<std::vector<DataSample<std::string> > > > vector_string_data;

  // Create channels as needed, and record pointers to them in vector_{numeric|string}_data for fast loading
  for (unsigned i = 0; i < channel_names.size(); i++) {
    std::string channel_name = channel_names[i].asString();
    if (numeric_data.find(channel_name) == numeric_data.end()) {
      numeric_data[channel_name] = simple_shared_ptr<std::vector<DataSample<double> > >(new std::vector<DataSample<double> >());
    }
    vector_numeric_data.push_back(numeric_data[channel_name]);
    if (string_data.find(channel_name) == string_data.end()) {
      string_data[channel_name] = simple_shared_ptr<std::vector<DataSample<std::string> > >(new std::vector<DataSample<std::string> >());
    }
    vector_string_data.push_back(string_data[channel_name]);
  }
  
  Json::Value datajson = json["data"];

  Json::FastWriter foo;

  for (unsigned i = 0; i < datajson.size(); i++) {
    Json::Value row = datajson[i];
    double timestamp;
    try {
      timestamp = row[(unsigned int)0].asDouble();
    } catch (std::exception &e) {
      try {
	// HACK!  parse
	timestamp = atof(row[(unsigned int)0].asString().c_str());
	if (timestamp == 0) throw ParseError("zero timestamp");
      } catch (std::exception &e) {
	std::string msg =
	  string_printf("In row %d, cannot parse timestamp (first col) of %s as double-precision",
			i, rtrim(Json::FastWriter().write(row)).c_str());
	throw ParseError(msg.c_str());
      }
    }

    for (unsigned j = 1; j < row.size(); j++) {
      if (row[j].type() == Json::nullValue) {
	// skip
      } else if (row[j].type() == Json::stringValue) {
        vector_string_data[j-1]->push_back(DataSample<std::string>(timestamp, row[j].asString()));
      } else {
        vector_numeric_data[j-1]->push_back(DataSample<double>(timestamp, row[j].asDouble()));
      }
    }
  }

  // Remove vectors of size 0
  
  erase_empty_data(numeric_data);
  erase_empty_data(string_data);
  
  info.good_records++;
}

void parse_json(Json::Value json,
		std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > > &numeric_data,
		std::map<std::string, simple_shared_ptr<std::vector<DataSample<std::string> > > > &string_data,
		std::vector<ParseError> &errors,
		ParseInfo &info)
{  
  if (json.isArray()) {
    for (unsigned i = 0; i < json.size(); i++) {
      parse_json_single(json[i], numeric_data, string_data, errors, info);
    }
  } else {
    parse_json_single(json, numeric_data, string_data, errors, info);
  }
}

void parse_json_file(const std::string &infile,
		     std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > > &numeric_data,
		     std::map<std::string, simple_shared_ptr<std::vector<DataSample<std::string> > > > &string_data,
		     std::vector<ParseError> &errors,
		     ParseInfo &info)
{
  info.good_records = 0;
  info.bad_records = 0;

  Json::Reader reader;
  std::ifstream instream(infile.c_str(), std::ios::binary);
  Json::Value json;
  if (!reader.parse(instream, json)) {
    throw ParseError("Failed to parse JSON file");
  }
  
  parse_json(json, numeric_data, string_data, errors, info);
}
