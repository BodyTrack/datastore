// C++
#include <stdexcept>

// Local
#include "Binrec.h"
#include "Channel.h"
#include "utils.h"

// Self
#include "ImportBT.h"

void import_bt_file(KVS &store, const std::string &bt_file, int uid)
{
  bool write_partial_on_errors = true;
  std::map<std::string, boost::shared_ptr<std::vector<DataSample<double> > > > data;
  std::vector<ParseError> errors;

  parse_bt_file(bt_file, data, errors);
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
    
    size_t tile_size = 1024*1024*6/4;
    Channel ch(store, uid, channel_name, tile_size);
    ch.add_data(*samples);
  }  
}





  
