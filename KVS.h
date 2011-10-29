#ifndef INCLUDE_KVS_H
#define INCLUDE_KVS_H

// C++
#include <string>
#include <vector>

class KVSLocker;


/// \class KVS KVS.h
/// Virtual base class for key-value store
///
/// Keys are string with restricted content:
/// key consists of A-Z, a-z, 0-9, - (dash), _ (underscore), and . (dot).
/// Dot indicates hierarchy, like a subdirectory.  Key cannot start or end with dot, nor can two
/// or more dots occur without non-dot characters in-between.
///
/// Examples of valid keys:  "abc", "abc.def", "abc.def.ghi", "123a_b-c"
///
/// Examples of invalid keys: "", ".", ".abc", "def.", "abc..def", "abc$"
///
/// Warning:  keys may or may not be case-sensitive, depending on the underlying implementation of the datastore(!).  User of the KVS
/// should not assume one way or the other.

class KVS {
public:
  KVS() : m_verbose(0) {}
  /// Check if key exists
  /// \param key
  /// \return Returns true if found, false if not
  virtual bool has_key(const std::string &key) const = 0;
  /// Set key to value
  /// \param key
  /// \param value
  virtual void set(const std::string &key, const std::string &value) = 0;
  /// Get value
  /// \param key
  /// \param value if found, returns value in this parameter
  /// \return Returns true if found, false if not
  virtual bool get(const std::string &key, std::string &value) const = 0;
  /// Delete key if present
  /// \return Returns true if deleted, false if not present
  virtual bool del(const std::string &key) = 0;
  /// Get subkeys
  /// \param key
  /// \param nlevels:  1=only return immediate children; 2=children and grandchildren; (unsigned int) -1: all children
  /// \return All subkeys, recursively
  virtual void get_subkeys(const std::string &key, std::vector<std::string> &keys, 
			   unsigned int nlevels=-1, bool (*subdir_filter)(const char *subdirname)=0) const = 0;
  /// Set verbosity
  /// \param verbosity 0=don't print; >0 print varying amounts for operations
  void set_verbosity(int verbosity) { m_verbose = verbosity; }
  
private:
  friend class KVSLocker;
  /// Lock key.  Do not call this directly;  instead, use KVSLocker to create a scoped lock
  /// \param key
  virtual void *lock(const std::string &key) = 0;
  /// Unlock key.  Do not call this directly;  instead, use KVSLocker to create a scoped lock
  /// \param lock
  virtual void unlock(void *lock) = 0;
protected:
  int m_verbose;
};

/// \class KVSLocker KVS.h
/// Scoped lock for KVS

class KVSLocker {
public:
  /// Locks key in KVS upon construction; if currently locked, construction will block until lock is available
  /// \param kvs KVS
  /// \param key key to lock
  KVSLocker(KVS &kvs, const std::string &key);
  /// \brief Unlock key in KVS
  ~KVSLocker();
protected:
  void *m_data;
  KVS &m_kvs;
};
  
#endif
