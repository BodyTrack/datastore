#include "mysql_common.h"

#include <iostream>

#include <mysql.h>

MYSQL mysql;

void mysql_abort(std::string msg)
{
  std::cerr << "MySQL error: " << msg << ": " << mysql_error(&mysql) << std::endl;
  exit(1);
}
