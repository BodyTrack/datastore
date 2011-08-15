// C++
#include <map>
#include <vector>
#include <fstream>
#include <limits>

// C
#include <stdio.h>

#include <boost/shared_ptr.hpp>

// Local
#include "Channel.h"
#include "DataSample.h"

// Self
#include "ImportJson.h"

void parse_json_file(const std::string &infile,
		     std::map<std::string, boost::shared_ptr<std::vector<DataSample<double> > > > &numeric_data,
		     std::map<std::string, boost::shared_ptr<std::vector<DataSample<std::string> > > > &string_data,
		     std::vector<ParseError> &errors,
		     ParseInfo &info)
{
  info.good_records = 0;
  info.bad_records = 0;

  Json::Reader reader;
  std::ifstream instream(infile.c_str());
  Json::Value json;
  if (!reader.parse(instream, json)) {
    throw ParseError("Failed to parse JSON file");
  }
  //fprintf(stderr, "%s\n", rtrim(Json::FastWriter().write(json)).c_str());
  
  Json::Value channel_names = json["channel_names"];
  //fprintf(stderr, "%s\n", rtrim(Json::FastWriter().write(channel_names)).c_str());
  Json::Value numeric_channel_names = json["numeric_ch_names"];
  //fprintf(stderr, "%s\n", rtrim(Json::FastWriter().write(numeric_channel_names)).c_str());
  
  // Create sample vectors
  std::vector<boost::shared_ptr<std::vector<DataSample<double> > > > vector_numeric_data;
  std::vector<boost::shared_ptr<std::vector<DataSample<std::string> > > > vector_string_data;
  
  for (unsigned i = 0; i < channel_names.size(); i++) {
    vector_numeric_data.push_back(boost::shared_ptr<std::vector<DataSample<double> > >(new std::vector<DataSample<double> >()));
    vector_string_data.push_back(boost::shared_ptr<std::vector<DataSample<std::string> > >(new std::vector<DataSample<std::string> >()));
  }
  
  Json::Value datajson = json["data"];
  //fprintf(stderr, "%s\n", rtrim(Json::FastWriter().write(datajson)).c_str());

  for (unsigned i = 0; i < datajson.size(); i++) {
    Json::Value row = datajson[i];
    double timestamp = row[(unsigned int)0].asDouble();
    //fprintf(stderr, "%.6f %s\n", timestamp, rtrim(Json::FastWriter().write(row)).c_str());
    for (unsigned j = 1; j < row.size(); j++) {
      if (row[j].type() == Json::stringValue) {
        vector_string_data[j-1]->push_back(DataSample<std::string>(timestamp, row[j].asString()));
      } else {
        vector_numeric_data[j-1]->push_back(DataSample<double>(timestamp, row[j].asDouble()));
      }
    }
  }

  for (unsigned i = 0; i < channel_names.size(); i++) {
    if (vector_numeric_data[i]->size()) numeric_data[channel_names[i].asString()] = vector_numeric_data[i];
    if (vector_string_data[i]->size()) string_data[channel_names[i].asString()] = vector_string_data[i];
  }
  
  info.good_records = 1;
}

