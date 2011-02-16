#include "Tile.h"

Tile::Tile(MysqlRow *row) {
  row->get(id, "id");
  row->get(ch_name, "ch_name");
  row->get(level, "level");
  row->get(offset, "offset");
  row->get(validation_recid, "validation_recid");
  row->get(json, "json");
  row->get(user_id, "user_id");
}
