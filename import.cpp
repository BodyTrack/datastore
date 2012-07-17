// C++
#include <iostream>
#include <string>
#include <vector>

// C
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// Local
#include "Binrec.h"
#include "Channel.h"
#include "DataSample.h"
#include "FilesystemKVS.h"
#include "ImportBT.h"
#include "ImportJson.h"
#include "Log.h"
#include "utils.h"

void usage()
{
  std::cerr << "Usage:\n";
  std::cerr << "import store.kvs uid device-nickname [--format format] file1.bt ... fileN.bt\n";
  std::cerr << "allows formats: bt json\n";
  throw std::runtime_error("Bad arguments");
}

void emit_json(Json::Value &json) {
  std::string response = rtrim(Json::FastWriter().write(json));
  printf("%s\n", response.c_str());
  log_f("import: sending: %s", response.c_str());
}
  
int execute(int argc, char **argv) { 
  long long begin_time = millitime();
  char **argptr = argv+1;

  if (!*argptr) usage();
  std::string storename = *argptr++;

  if (!*argptr) usage();
  int uid = atoi(*argptr++);
  if (uid <= 0) usage();
  set_log_prefix(string_printf("%d %d ", getpid(), uid));
  
  {
    std::string arglist;
    for (int i = 0; i < argc; i++) {
      if (i) arglist += " ";
      arglist += std::string("'")+argv[i]+"'";
    }
    log_f("import START: %s", arglist.c_str());
  }

  if (!*argptr) usage();
  std::string dev_nickname = *argptr++;

  std::string format = "";
  std::vector<std::string> files;

  while (*argptr) {
    if (!strcmp(*argptr, "--format")) {
      argptr++;
      if (!*argptr) usage();
      format = *argptr++;
    }
    if (!strcmp(*argptr, "--verbose")) {
      argptr++;
      verbose=true;
    }
    else {
      if (!filename_exists(*argptr)) {
        log_f("import: file %s doesn't exist", *argptr);
        usage();
      }
      files.push_back(*argptr++);
    }
  }

  if (!files.size()) usage();

  FilesystemKVS store(storename.c_str());

  bool write_partial_on_errors = true;
  bool backup_imported_files = false;

  for (unsigned i = 0; i < files.size(); i++) {
    std::string filename = files[i];
    log_f("import: Importing %s into %d %s", filename.c_str(), uid, dev_nickname.c_str());
    if (backup_imported_files) {
      std::string backup_name = "/var/log/bodytrack/imports/" + filename_sans_directory(filename);
      log_f("import: backing up %s to %s", filename.c_str(), backup_name.c_str());
      int ret = system(string_printf("rsync '%s' '%s'", filename.c_str(), backup_name.c_str()).c_str());
      if (ret != 0) log_f("rsync returns non-zero status %d", ret);
    }
    
    ParseInfo info;
    std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > > numeric_data;
    std::map<std::string, simple_shared_ptr<std::vector<DataSample<std::string> > > > string_data;
    std::vector<ParseError> errors;

    std::string this_format;
    if (format != "") {
      this_format = format;
    } else {
      this_format = filename_suffix(filename);
    }
    if (!strcasecmp(this_format.c_str(), "bt") || !strcasecmp(this_format.c_str(), "binary")) {
      parse_bt_file(filename, numeric_data, errors, info);
    } else if (!strcasecmp(this_format.c_str(), "json")) {
      parse_json_file(filename, numeric_data, string_data, errors, info);
    } else {
      throw std::runtime_error(string_printf("Unrecognized format or filename suffix '%s'", this_format.c_str()));
    }

    if (errors.size()) {
      log_f("import: Parse errors:");
      for (unsigned i = 0; i < errors.size(); i++) {
        log_f("import:    %s", errors[i].what());
      }
      if (!numeric_data.size() && !string_data.size()) {
        log_f("import: No data returned");
      } else if (!write_partial_on_errors) {
        log_f("import: Partial data returned, but not adding to store");
      } else {
        log_f("import: Partial data returned;  adding to store");
      }
    }

    std::map<std::string, DataRanges> import_ranges;
    std::map<std::string, DataRanges> channel_ranges;
    Range import_time_range;
    
    for (std::map<std::string, simple_shared_ptr<std::vector<DataSample<double> > > >::iterator i =
           numeric_data.begin(); i != numeric_data.end(); ++i) {
      
      std::string channel_name = i->first;
      simple_shared_ptr<std::vector<DataSample<double> > > samples = i->second;
      std::sort(samples->begin(), samples->end(), DataSample<double>::time_lessthan);
      
      log_f("import: %.6f: %s %zd numeric samples", (*samples)[0].time, channel_name.c_str(), samples->size());
      
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
    
    for (std::map<std::string, simple_shared_ptr<std::vector<DataSample<std::string> > > >::iterator i =
           string_data.begin(); i != string_data.end(); ++i) {
      
      std::string channel_name = i->first;
      simple_shared_ptr<std::vector<DataSample<std::string> > > samples = i->second;
      std::sort(samples->begin(), samples->end(), DataSample<std::string>::time_lessthan);
      
      log_f("%.6f: %s %zd textual samples", (*samples)[0].time, channel_name.c_str(), samples->size());
      
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
      result["min_time"] = Json::Value(import_time_range.min);
      result["max_time"] = Json::Value(import_time_range.max);
    }
    result["channel_specs"] = info.channel_specs;
    log_f("import: finished in %lld msec", millitime() - begin_time);
    bool include_log = false;
    if (include_log) result["log"]=Json::Value(recorded_log());
    emit_json(result);
  }
  return 0;
}

int main(int argc, char **argv)
{
  int exit_code = 1;
  try {
    exit_code = execute(argc, argv);
  } catch (const std::exception &e) {
    log_f("import: caught exception '%s'", e.what());
    Json::Value json_response(Json::objectValue);
    json_response["failure"] = Json::Value(string_printf("exception: %s", e.what()));
    emit_json(json_response);
  }
  return exit_code;
}
 
