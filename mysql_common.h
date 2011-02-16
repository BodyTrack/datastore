#ifndef MYSQL_COMMON_INCLUDE_H
#define MYSQL_COMMON_INCLUDE_H

#include <string>

#include <mysql.h>

extern MYSQL mysql;
void mysql_abort(std::string msg);

#endif
