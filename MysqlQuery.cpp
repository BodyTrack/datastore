#include <iostream>

#include "MysqlQuery.h"
#include "mysql_common.h"
#include "utils.h"

bool MysqlQuery::s_debug = false;

MysqlQuery::MysqlQuery(std::string fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  std::string query= string_vprintf(fmt.c_str(), args);
  va_end(args);
  
  if (s_debug) std::cerr << "MySQL query " << query << std::endl;
  
  if (mysql_query(&mysql, query.c_str())) mysql_abort("mysql_query " + query);

  m_result = mysql_store_result(&mysql);
  if (!m_result) mysql_abort("mysql_store_result");
  m_rowcount = 0;
}

MysqlRow *MysqlQuery::fetch_row() {
  MYSQL_ROW row = mysql_fetch_row(m_result);
  if (row == NULL ) return NULL;
  if (!m_rowcount) {
    // Fetch fieldnames
    MYSQL_FIELD *field;
    while ((field = mysql_fetch_field(m_result))) {
      m_fieldnames.push_back(field->name);
    }
  }
  for (unsigned i=0; i<m_fieldnames.size(); i++) {
    m_current_row[m_fieldnames[i]] = row[i] ? row[i] : "<NULL>";
  }
  return &m_current_row;
}

MysqlQuery::~MysqlQuery() {
  mysql_free_result(m_result);
}

