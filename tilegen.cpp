#include <string>
#include <iostream>
#include <stdio.h>

#include "MysqlQuery.h"
#include "mysql_common.h"
#include "Tile.h"
#include "Channel.h"

bool debug;

Tile *retrieve_tile(int user_id, std::string dev_nickname, std::string ch_name, int level, long long offset, 
		    int checkpoint=-1/*, lgset=nil*/)
{
  // Lookup tile
  // TODO: fix security here!

  std::string tile_ch_name = dev_nickname + "." + ch_name;
  
  // Fetch existing tile, if exists
  //
  // TODO: optimize by making (user_id, level, offset, ch_name) a unique index, and using
  //      "on duplicate key update" to update tile in-place when regenerating

  {
    MysqlQuery tile_query("select * from Tiles where user_id = %d and level = %d and offset = %lld "
			  "and ch_name = '%s' order by validation_recid desc limit 1",
			  user_id, level, offset, tile_ch_name.c_str());
    
    
    MysqlRow *row = tile_query.fetch_row();
    if (row) {
      Tile *t = new Tile(row);
      if (debug) std::cerr << "got a tile with validation_recid " << t->validation_recid << std::endl;
      
      // Find intersecting logrecs
      {
	double start_time = Tile::offset_at_level_to_unixtime(offset, level);
	double end_time = Tile::offset_at_level_to_unixtime(offset+1, level);
	MysqlQuery q("select id from Logrecs where user_id = %d and dev_nickname = '%s' and "
		     "begin_d < %lf and end_d >= %lf and "
		     "dat_fields LIKE '%%- %s%%' OR dat_fields LIKE '%%name: %s%%' "
		     "order by id desc limit 1",
		     user_id, dev_nickname.c_str(),
		     end_time, start_time,
		     ch_name.c_str(), ch_name.c_str());
	
	MysqlRow *row = q.fetch_row();
	if (row) {
	  long long logrec_id = row->get_long_long("id");
	  if (debug) std::cerr << "most recent logrec is " << logrec_id << std::endl;
	  if (logrec_id <= t->validation_recid) {
	    if (debug) std::cerr << "valid, returning\n";
	    return t;
	  } else {
	    delete t;
	  }
	} else {
	  if (debug) std::cerr << "NO LOGRECS:  todo: delete tile\n";
	  delete t;
	  return NULL;
	}
      }
    }
  }
  return NULL;
}

void test_retrieve_tile()
{
  int user_id = 1;
  std::string dev_nickname = "Josh_Basestation";
  std::string ch_name = "Temperature";
  int level = 0;
  long long offset = 2517760;
  
  Tile *t=retrieve_tile(user_id, dev_nickname, ch_name, level, offset);
  if (t) {
    std::cout << t->json;
  }
}

void test_channel()
{
  int user_id = 1;
  std::string dev_nickname = "Josh_Basestation";
  std::string ch_name = "Temperature";
  double begin_time = 1289093120.98392;
  double end_time   = 1289093631.98183;

  Channel ch(user_id, dev_nickname, ch_name);
  ch.seek(begin_time);
  double t, val;
  fprintf(stderr, "Fetching from %f to %f\n", begin_time, end_time);
  while (ch.fetch_next(t, val)) {
    
    if (t > end_time) {
      fprintf(stderr, "Fetching time %f, done\n", t);
      break;
    }
    fprintf(stderr, "%lf: %g\n", t, val);
  }
}

int main(int argc, char **argv)
{
  debug = true;
  std::string hostname="localhost";
  std::string username="root";
  std::string password="";
  std::string database="bodytrack-dev";
  std::string csv_root = "/Users/admin/projects/bodytrack/website/data";

  Logrec::s_csv_root = csv_root;

  MysqlQuery::s_debug = debug;
  if (debug) std::cerr << "Connecting to mysql...\n";
  
  if (!mysql_init(&mysql)) {
    mysql_abort("mysql_init");
  }
  
  if (!mysql_real_connect(&mysql, hostname.c_str(), username.c_str(), password.c_str(), 
				  database.c_str(), 0, 0, 0)) {
    mysql_abort("connecting to mysql");
  }
  
  //test_retrieve_tile();
  test_channel();

  return 0;
}


