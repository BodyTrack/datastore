#ifndef INCLUDE_IMPORT_BT_H
#define INCLUDE_IMPORT_BT_H

// Local includes
#include "Binrec.h"
#include "KVS.h"

void import_bt_file(KVS &store, const std::string &infile, int uid, const std::string &dev_nickname, ParseInfo &info);

#endif
