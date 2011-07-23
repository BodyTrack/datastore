#include "KVS.h"

KVSLocker::KVSLocker(KVS &kvs, const std::string &key) : m_kvs(kvs) {
  m_data = m_kvs.lock(key);
}

KVSLocker::~KVSLocker() {
  m_kvs.unlock(m_data);
}
