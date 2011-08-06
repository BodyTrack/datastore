#ifndef INCLUDE_IMPORT_JSON_H
#define INCLUDE_IMPORT_JSON_H

// C++
#include <string>

// Local includes
#include "KVS.h"
#include "Parse.h"

void import_json_file(KVS &store, const std::string &bt_file, int uid, const std::string &dev_nickname, ParseInfo &info);

#endif
