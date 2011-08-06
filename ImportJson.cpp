// C++
#include <map>
#include <vector>
#include <fstream>

// C
#include <stdio.h>

#include <boost/shared_ptr.hpp>

// Local
#include "Channel.h"
#include "DataSample.h"

// Self
#include "ImportJson.h"

void parse_json_file(const std::string &infile,
		     std::map<std::string, boost::shared_ptr<std::vector<DataSample<double> > > > &data,
		     std::vector<ParseError> &errors,
		     ParseInfo &info)
{
  info.min_time = 1.0/0.0;
  info.max_time = -1.0/0.0;
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
  for (unsigned i = 0; i < numeric_channel_names.size(); i++) {
    data[numeric_channel_names[i].asString()] = 
      boost::shared_ptr<std::vector<DataSample<double> > >(new std::vector<DataSample<double> >());
  }
  
  Json::Value datajson = json["data"];
  //fprintf(stderr, "%s\n", rtrim(Json::FastWriter().write(datajson)).c_str());

  for (unsigned i = 0; i < datajson.size(); i++) {
    Json::Value row = datajson[i];
    double timestamp = row[(unsigned int)0].asDouble();
    info.min_time = std::min(info.min_time, timestamp);
    info.max_time = std::max(info.max_time, timestamp);
    //fprintf(stderr, "%.6f %s\n", timestamp, rtrim(Json::FastWriter().write(row)).c_str());
    for (unsigned j = 1; j < row.size(); j++) {
      std::string channel_name = channel_names[j-1].asString();
      if (data.find(channel_name) != data.end()) {
	double value = row[j].asDouble();
	data[channel_name]->push_back(DataSample<double>(timestamp, value));
      }
    }
  }
  info.good_records = 1;
}



void import_json_file(KVS &store, const std::string &bt_file, int uid, const std::string &dev_nickname, ParseInfo &info)
{
  bool write_partial_on_errors = true;
  std::map<std::string, boost::shared_ptr<std::vector<DataSample<double> > > > data;
  std::vector<ParseError> errors;

  parse_json_file(bt_file, data, errors, info);
  if (errors.size()) {
    fprintf(stderr, "Parse errors:\n");
    for (unsigned i = 0; i < errors.size(); i++) {
      fprintf(stderr, "%s\n", errors[i].what());
    }
    if (!data.size()) {
      fprintf(stderr, "No data returned\n");
    } else if (!write_partial_on_errors) {
      fprintf(stderr, "Partial data returned, but not adding to store\n");
    } else {
      fprintf(stderr, "Partial data returned;  adding to store\n");
    }
  }

  for (std::map<std::string, boost::shared_ptr<std::vector<DataSample<double> > > >::iterator i =
         data.begin(); i != data.end(); ++i) {

    std::string channel_name = i->first;
    boost::shared_ptr<std::vector<DataSample<double > > > samples = i->second;

    fprintf(stderr, "%.6f: %s %zd samples\n", (*samples)[0].time, channel_name.c_str(), samples->size());
    
    Channel ch(store, uid, dev_nickname + "." + channel_name);
    ch.add_data(*samples);
  }  
}

