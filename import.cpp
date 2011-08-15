// C++
#include <iostream>
#include <string>
#include <vector>

// C
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

// Local
#include "Binrec.h"
#include "Channel.h"
#include "DataSample.h"
#include "FilesystemKVS.h"
#include "ImportBT.h"
#include "ImportJson.h"
#include "utils.h"

void usage()
{
  std::cerr << "Usage:\n";
  std::cerr << "import store.kvs uid device-nickname file1.bt ... fileN.bt\n";
  std::cerr << "Exiting...\n";
  exit(1);
}

int main(int argc, char **argv)
{
  char **argptr = argv+1;

  if (!*argptr) usage();
  std::string storename = *argptr++;

  if (!*argptr) usage();
  int uid = atoi(*argptr++);
  if (uid <= 0) usage();
  
  if (!*argptr) usage();
  std::string dev_nickname = *argptr++;

  std::vector<std::string> files(argptr, argv+argc);
  if (!files.size()) usage();

  for (unsigned i = 0; i < files.size(); i++) {
    if (!filename_exists(files[i])) {
      fprintf(stderr, "File %s doesn't exist\n", files[i].c_str());
      usage();
    }
  }

  fprintf(stderr, "Opening store %s\n", storename.c_str());
  FilesystemKVS store(storename.c_str());


  bool write_partial_on_errors = true;
  
  for (unsigned i = 0; i < files.size(); i++) {
    std::string filename = files[i];
    fprintf(stderr, "Importing %s into UID %d\n", filename.c_str(), uid);
    
    ParseInfo info;
    std::map<std::string, boost::shared_ptr<std::vector<DataSample<double> > > > numeric_data;
    std::map<std::string, boost::shared_ptr<std::vector<DataSample<std::string> > > > string_data;
    std::vector<ParseError> errors;
    
    //import_json_file(store, files[i], uid, dev_nickname, info);

    if (!strcasecmp(filename_suffix(filename).c_str(), "bt")) {
      parse_bt_file(filename, numeric_data, errors, info);
    } else if (!strcasecmp(filename_suffix(filename).c_str(), "json")) {
      parse_json_file(filename, numeric_data, string_data, errors, info);
    } else {
      printf("{\"failure\":\"Unrecognized filename suffix %s\"}\n", filename_suffix(filename).c_str());
      continue;
    }

    if (errors.size()) {
      fprintf(stderr, "Parse errors:\n");
      for (unsigned i = 0; i < errors.size(); i++) {
        fprintf(stderr, "%s\n", errors[i].what());
      }
      if (!numeric_data.size() && !string_data.size()) {
        fprintf(stderr, "No data returned\n");
      } else if (!write_partial_on_errors) {
        fprintf(stderr, "Partial data returned, but not adding to store\n");
      } else {
        fprintf(stderr, "Partial data returned;  adding to store\n");
      }
    }

    std::map<std::string, DataRanges> import_ranges;
    std::map<std::string, DataRanges> channel_ranges;
    Range import_time_range;
    
    for (std::map<std::string, boost::shared_ptr<std::vector<DataSample<double> > > >::iterator i =
           numeric_data.begin(); i != numeric_data.end(); ++i) {
      
      std::string channel_name = i->first;
      boost::shared_ptr<std::vector<DataSample<double> > > samples = i->second;
      
      fprintf(stderr, "%.6f: %s %zd numeric samples\n", (*samples)[0].time, channel_name.c_str(), samples->size());
      
      Channel ch(store, uid, dev_nickname + "." + channel_name);

      {
        DataRanges cr;
        ch.add_data(*samples, &cr);
        if (!cr.times.empty()) channel_ranges[channel_name].add(cr);
      }

      {
        DataRanges ir;
        for (unsigned i = 0; i < samples->size(); i++) {
          DataSample<double> &sample = (*samples)[i];
          ir.times.add(sample.time);
          ir.double_samples.add(sample.value);
        }
        import_ranges[channel_name].add(ir);
      }
    }  
    
    for (std::map<std::string, boost::shared_ptr<std::vector<DataSample<std::string> > > >::iterator i =
           string_data.begin(); i != string_data.end(); ++i) {
      
      std::string channel_name = i->first;
      boost::shared_ptr<std::vector<DataSample<std::string> > > samples = i->second;
      
      fprintf(stderr, "%.6f: %s %zd textual samples\n", (*samples)[0].time, channel_name.c_str(), samples->size());
      
      Channel ch(store, uid, dev_nickname + "." + channel_name);

      {
        DataRanges cr;
        ch.add_data(*samples, &cr);
        if (!cr.times.empty()) channel_ranges[channel_name].add(cr);
      }

      {
        DataRanges ir;
        for (unsigned i = 0; i < samples->size(); i++) {
          DataSample<std::string> &sample = (*samples)[i];
          ir.times.add(sample.time);
        }
        import_ranges[channel_name].add(ir);
      }
    }

    for (std::map<std::string, DataRanges>::iterator i = import_ranges.begin(); i != import_ranges.end(); ++i) {
      info.channel_specs[i->first]["imported_bounds"] = i->second.to_json();
      import_time_range.add(i->second.times);
    }

    for (std::map<std::string, DataRanges>::iterator i = channel_ranges.begin(); i != channel_ranges.end(); ++i) {
      info.channel_specs[i->first]["channel_bounds"] = i->second.to_json();
    }

    Json::Value result(Json::objectValue);
    result["successful_records"] = Json::Value(info.good_records);
    result["failed_records"] = Json::Value(info.bad_records);
    if (!import_time_range.empty()) {
      result["min_time"] = Json::Value(import_time_range.min());
      result["max_time"] = Json::Value(import_time_range.max());
    }
    result["channel_specs"] = info.channel_specs;
    printf("%s", rtrim(Json::FastWriter().write(result)).c_str());
  }
  fprintf(stderr, "Done\n");
  return 0;
}
