// System
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>

// C++
#include <stdexcept>

// Local
#include "Log.h"
#include "utils.h"

// Self
#include "FilesystemKVS.h"

// \brief Instantiate FilesystemKVS
// \param root Directory to use as root of store.  Should already exist
FilesystemKVS::FilesystemKVS(const char *root) : m_root(root) {
  if (m_root == "" || m_root[m_root.size()-1] == '/')
    throw std::runtime_error("store path " + std::string(root) + " shouldn't end with '/'");
  if (!filename_exists(root))
    throw std::runtime_error(std::string(root) + " does not exist");
  log_f("FilesystemKVS: opening %s", root);
}

/// \brief Check if key exists
/// \param key
/// \return Returns true if found, false if not
bool FilesystemKVS::has_key(const std::string &key) const {
  struct stat statbuf;
  int ret= stat(key_to_path(key).c_str(), &statbuf);
  return (ret == 0);
}

/// \brief Set key to value
/// \param key
/// \param value
void FilesystemKVS::set(const std::string &key, const std::string &value) {
  std::string path = key_to_path(key); 
  FILE *out = fopen(path.c_str(), "w");
  if (!out) {
    make_parent_directories(path);
    out = fopen(path.c_str(), "w");
    if (!out) throw std::runtime_error("fopen " +  path);
  }
  if (value.size()) {
    if (1 != fwrite(&value[0], value.size(), 1, out)) {
      fclose(out);
      throw std::runtime_error("fwrite " + path);
    }
  }
  if (m_verbose) log_f("FilesystemKVS::set(%s) wrote %zd bytes to %s", key.c_str(), value.length(), path.c_str());
  fclose(out);
}

/// \brief Get value
/// \param key
/// \param value if found, returns value in this parameter
/// \return Returns true if found, false if not
///
/// See FilesystemKVS class description for the mapping between datastore and filesystem.
bool FilesystemKVS::get(const std::string &key, std::string &value) const {
  std::string path = key_to_path(key); 
  FILE *in = fopen(path.c_str(), "r");
  if (!in) {
    if (m_verbose) log_f("FilesystemKVS::get(%s) found no file at %s, returning false", key.c_str(), path.c_str());
    return false;
  }
  struct stat statbuf;
  if (0 != fstat(fileno(in), &statbuf)) {
    fclose(in);
    throw std::runtime_error("fstat " + path);
  }
  value.resize(statbuf.st_size);
  if (statbuf.st_size) {
    if (1 != fread(&value[0], statbuf.st_size, 1, in)) {
      fclose(in);
      throw std::runtime_error("fread " + path);
    }
  }
  fclose(in);
  if (m_verbose) log_f("FilesystemKVS::get(%s) read %zd bytes from %s", key.c_str(), value.length(), path.c_str());
  return true;
}

/// Delete key if present
/// \param key
/// \return Returns true if deleted, false if not present
bool FilesystemKVS::del(const std::string &key) {
  std::string path = key_to_path(key);
  return unlink(path.c_str()) == 0;
}

/// Lock key.  Do not call this directly;  instead, use KVSLocker to create a scoped lock.
/// \param key
///
/// Uses flock on file that contains value.
void *FilesystemKVS::lock(const std::string &key) {
  std::string path = key_to_path(key); 
  FILE *f = fopen(path.c_str(), "a");
  if (!f) {
    make_parent_directories(path);
    f = fopen(path.c_str(), "a");
    if (!f) throw std::runtime_error("fopen " +  path);
  }
  int fd= fileno(f);
  if (-1 == flock(fd, LOCK_EX)) {
    fclose(f);
    throw std::runtime_error("flock " + path);
  }
  if (m_verbose) log_f("FilesystemKVS::lock(%s) locked %s (fd=%d)", key.c_str(), path.c_str(), fd);
  return (void*)f;
}

/// Unlock key after it has been locked with lock.  Do not call this directly;  instead, use KVSLocker to create a scoped lock
/// \param key
void FilesystemKVS::unlock(void *lock) {
  FILE *f = (FILE*)lock;
  int fd = fileno(f);
  int ret = flock(fd, LOCK_UN);
  fclose(f);
  if (ret == -1) throw std::runtime_error("funlock");
  if (m_verbose) log_f("FilesystemKVS::unlock unlocked fd %d", fd);
}

/// Return path to file that stores the value associated with key
/// \param key Key
/// \return Returns path to file that stores value associated with key
std::string FilesystemKVS::key_to_path(const std::string &key) const {
  // Validate key
  if (key == "" || key[0] == '.' || key[key.size()-1] == '.' || key.find("..") != std::string::npos) {
    throw std::runtime_error("Invalid key '" + key + "'");
  }
  std::string path = key;
  for (size_t i = 0; i < key.size(); i++) {
    if (key[i] == '.') {
      path[i] = '/';
    } else if (!isalnum(key[i]) && key[i] != '_' && key[i] != '-') {
      throw std::runtime_error("Invalid key '" + key + "'");
    }
  }
  return m_root + "/" + path + ".val";
}

void FilesystemKVS::make_parent_directories(const std::string &path) {
  size_t lastDirDelim= path.rfind('/');
  if (lastDirDelim == std::string::npos) throw std::runtime_error("make_parent_directories " + path);
  std::string subpath = path.substr(0, lastDirDelim);
  if (0 == mkdir(subpath.c_str(), 0777) || errno == EEXIST) return;
  
  if (errno == ENOENT) {
    make_parent_directories(subpath);
    if (0 == mkdir(subpath.c_str(), 0777) || errno == EEXIST) return;
  }

  throw std::runtime_error("make_parent_directories " + path);
}

