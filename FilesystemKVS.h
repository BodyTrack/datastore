#ifndef FILESYSTEM_KVS_H
#define FILESYSTEM_KVS_H

#include "KVS.h"

/// \class FilesystemKVS FilesystemKVS.h
/// Key-value store based on filesystem (one value per file);  implements KVH.
///
/// Filesystem layout:
/// Each key corresponds to a file in the filesystem.  Keys names are translated to file path by converting all "." characters to "/".

class FilesystemKVS : public KVS {
public:
  FilesystemKVS(const char *root);
  virtual bool has_key(const std::string &key) const;
  virtual void set(const std::string &key, const std::string &value);
  virtual bool get(const std::string &key, std::string &value) const;
  virtual bool del(const std::string &key);
private:
  std::string m_root;

  std::string key_to_path(const std::string &key) const;
  static void make_parent_directories(const std::string &path);

  virtual void *lock(const std::string &key);
  virtual void unlock(void *lock);
};

#endif

  
