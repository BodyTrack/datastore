#include <iostream>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "Logrec.h"
#include "utils.h"

bool Logrec::s_debug = true;
std::string Logrec::s_csv_root;

Logrec::Logrec(MysqlRow *row, const std::string &ch_name) {
  MysqlRow::iterator i;
  //for (i= row->begin(); i != row->end(); i++) {
  //  std::cerr << i->first << " => " << i->second << std::endl;
  //}
  row->get(id, "id");
  row->get(type, "type");
  row->get(user_id, "user_id");
  row->get(dev_id, "dev_id");
  row->get(device_id, "device_id");
  row->get(dev_nickname, "dev_nickname");
  row->get(device_id, "device_class");
  row->get(firmware_version, "firmware_version");
  row->get(begin_d, "begin_d");
  row->get(end_d, "end_d");
  row->get(dat_fields, "dat_fields");
  row->get(num_datapoints, "num_datapoints");
  row->get(comment, "comment");
  
  m_csv_valid = false;

  // Parse dat_fields
  const char *df = dat_fields.c_str();
  // Skip to first newline
  while (*df && *df != '\n') df++;
  while (*df) {
    // Skip 3 chars
    if (!*df) break; df++;
    if (!*df) break; df++;
    if (!*df) break; df++;
    const char *begin = df;
    // Skip to newline
    while (*df && *df != '\n') df++;
    fieldnames.push_back(std::string(begin, df));
  }
  m_csv_cols = fieldnames.size();
  if (s_debug) {
    std::cerr << "Read " << summary() << std::endl;
  }
  m_fetch_ch_name = ch_name;
  m_fetch_index = -1;
  for (int i=1; i<m_csv_cols; i++) {
    if (fieldnames[i] == m_fetch_ch_name) m_fetch_index = i;
  }
  if (m_fetch_index < 1) {
    std::cerr << "Can't find field " << m_fetch_ch_name << ", aborting\n";
    std::cerr << "dat_fields: '" << dat_fields << "'\n";
    exit(-1);
  }
  
  //mport_id => <NULL>
  //nsfw => <NULL>
  //photo_content_type => <NULL>
  //photo_file_name => <NULL>
  //photo_file_size => <NULL>
  //photo_updated_at => <NULL>
  //updated_at => 2011-02-12 08:47:25
  //created_at => 2011-02-12 08:47:26
  //datafile_id => <NULL>
  //datahash => --- !map:HashWithIndifferentAccess 
  //hardware_version: "1"
  //method: :post
  //action: binupload
  //channel_specs: !map:HashWithIndifferentAccess 
  //controller: logrecs
  //firmware_version: "1.01"
  //start_of_file_time: !ruby/object:Bintime 
}

std::string Logrec::summary() {
  std::string ret = string_printf("[Logrec %lld %f-%f (", id, begin_d, end_d);
  for (int i=1; i<m_csv_cols; i++) {
    if (i>1) ret += " ";
    ret += fieldnames[i];
  }
  ret += ")";
  if (m_csv_valid) ret += string_printf(" %d records", m_csv_rows);
  ret += "]";
  return ret;
}

void Logrec::seek(double t) {
  // TODO: actually perform seek if read
  m_current_time = t;
}
  
bool Logrec::next_time(double &t, bool force_read) {
  if (!m_csv_valid) {
    if (m_current_time < begin_d && !force_read) { t= begin_d; return true; }
    if (m_current_time > end_d)   { return false; }
    read_csv();
    for (m_csv_row = 0;
         m_csv_row < m_csv_rows && m_csv_data[m_csv_row * m_csv_cols] < m_current_time;
         m_csv_row++);
    std::cerr << "skipped " << m_csv_row << " rows\n";
  }
  if (m_csv_row >= m_csv_rows) return false;
  t = m_csv_data[m_csv_row * m_csv_cols];
  assert(0 <= m_csv_row && m_csv_row < m_csv_rows);
  return true;
}

static bool parse_double(double &val, const char *&ptr, const char *end) {
  while (ptr < end && (isspace(*ptr) || *ptr == ',')) ptr++;
  if (ptr >= end) return false;
  char *endptr=NULL;
  val = strtod(ptr, &endptr);
  if (endptr) {
    ptr=endptr;
    return true;
  }
  return false;
}

void Logrec::read_csv() {
  std::cerr << "read_csv:  filename is " << csv_filename() << std::endl;
  
  // memmap the file
  int csv_fd = open(csv_filename().c_str(), O_RDONLY);
  if (csv_fd < 0) {
    perror(("open " +csv_filename()).c_str());
    exit(-1);
  }
  struct stat csv_stat;
  if (fstat(csv_fd, &csv_stat) < 0) {
    perror(("fstat " +csv_filename()).c_str());
    exit(-1);
  }
  size_t len = csv_stat.st_size;

  void *csv_data = mmap(NULL, len, PROT_READ, MAP_FILE|MAP_PRIVATE, csv_fd, 0);
  if (csv_data == (const char*)MAP_FAILED) {
    perror(("mmap " + csv_filename()).c_str());
    exit(-1);
  }
  const char *csv_ptr = (const char*)csv_data;
  const char *csv_end = csv_ptr + len;

  m_csv_data.clear();
  double val;
  while (parse_double(val, csv_ptr, csv_end)) {
    m_csv_data.push_back(val);
  }

  m_csv_rows = m_csv_data.size() / m_csv_cols;
  assert((size_t)(m_csv_rows * m_csv_cols) == m_csv_data.size());
  
  m_csv_valid = true;
  if (munmap((void*)csv_data, len) == -1) {
    perror(("munmap " +csv_filename()).c_str());
    exit(-1);
  }

  close(csv_fd);

  std::cerr << "Read " << m_csv_rows << " records of " << m_csv_cols << " fields each\n";
}

bool Logrec::fetch_next(double &t, double &val) {
  if (!next_time(t, true)) {
    if (s_debug) fprintf(stderr, "fetch_next(%lld): end of data\n", id);
    return false;
  }
  assert(0 <= m_csv_row && m_csv_row < m_csv_rows);
  val = m_csv_data[m_csv_row * m_csv_cols + m_fetch_index];
  if (s_debug) fprintf(stderr, "fetch_next(%lld): row=%d t=%f val=%g\n",
                       id, m_csv_row, t, val);
  m_csv_row++;
  return true;
}

std::string Logrec::csv_filename() {
  return string_printf("%s/%d/Logdata/%d.csv", s_csv_root.c_str(), user_id, id);
}
