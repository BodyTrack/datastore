// System
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

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
  if (m_verbose) log_f("FilesystemKVS: opening %s", root);
}

/// \brief Check if key exists
/// \param key
/// \return Returns true if found, false if not
bool FilesystemKVS::has_key(const std::string &key) const {
  struct stat statbuf;
  int ret= stat(value_key_to_path(key).c_str(), &statbuf);
  return (ret == 0);
}

/// \brief Set key to value
/// \param key
/// \param value
void FilesystemKVS::set(const std::string &key, const std::string &value) {
  std::string path = value_key_to_path(key); 
  FILE *out = fopen(path.c_str(), "wb");
  if (!out) {
    make_parent_directories(path);
    out = fopen(path.c_str(), "wb");
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
  std::string path = value_key_to_path(key); 
  FILE *in = fopen(path.c_str(), "rb");
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
  std::string path = value_key_to_path(key);
  return unlink(path.c_str()) == 0;
}

/// Get subkeys
/// \param key
/// \param nlevels:  1=only return immediate children; 2=children and grandchildren; (unsigned int) -1: all children
/// \return All subkeys, recursively
void FilesystemKVS::get_subkeys(const std::string &key, std::vector<std::string> &keys, 
				unsigned int nlevels, bool (*subdir_filter)(const char *subdirname)) const {
  std::string path = directory_key_to_path(key);
  std::string prefix = (key == "") ? "" : key+".";
  DIR *dir = opendir(path.c_str());
  if (!dir) return;
  while (1) {
    struct dirent *ent = readdir(dir);
    if (!ent) break;
    if (!strcmp(ent->d_name, ".")) continue;
    if (!strcmp(ent->d_name, "..")) continue;
    if (filename_suffix(ent->d_name) == "val") {
      keys.push_back(prefix+filename_sans_suffix(ent->d_name));
    } else if (nlevels > 1 && (!subdir_filter || (*subdir_filter)(ent->d_name))) {
      // If it's a directory, recurse, honoring symlinks
      bool is_directory = false;
      std::string fullpath = path+"/"+ent->d_name;
      switch (ent->d_type) {
      case DT_DIR: is_directory = true; break;
      case DT_LNK: {
	struct stat statbuf;
	int ret = stat(fullpath.c_str(), &statbuf);
	if (ret != 0) {
	  perror(("stat " + fullpath).c_str());
	} else {
	  is_directory = S_ISDIR(statbuf.st_mode);
	}
      }
      }
      if (is_directory) get_subkeys(prefix+ent->d_name, keys, nlevels-1);
    }
  }
  closedir(dir);
}

/// Lock key.  Do not call this directly;  instead, use KVSLocker to create a scoped lock.
/// \param key
///
/// Uses flock on file that contains value.
void *FilesystemKVS::lock(const std::string &key) {
  std::string path = value_key_to_path(key); 
  FILE *f = fopen(path.c_str(), "ab");
  if (!f) {
    make_parent_directories(path);
    f = fopen(path.c_str(), "a");
    if (!f) throw std::runtime_error("fopen " +  path);
  }
  int fd= fileno(f);
  if (m_verbose) log_f("FilesystemKVS::lock(%s) about to lock %s (fd=%d)", key.c_str(), path.c_str(), fd);
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
std::string FilesystemKVS::value_key_to_path(const std::string &key) const {
  // Validate key
  if (key == "") throw std::runtime_error("Invalid key '" + key + "'");
  return directory_key_to_path(key) + ".val";
}

/// Return path to directory that stores subkey children of key
/// \param key Key
/// \return Returns path to directory that stores subkey children of key
std::string FilesystemKVS::directory_key_to_path(const std::string &key) const {
  if (key == "") return m_root;
  if (key[0] == '.' || key[key.size()-1] == '.' || key.find("..") != std::string::npos) {
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
  return m_root + "/" + path;
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

