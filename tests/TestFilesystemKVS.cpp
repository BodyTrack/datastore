// C
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// C++
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// BOOST
#include <boost/thread/thread.hpp>

// Module to test
#include "FilesystemKVS.h"

std::map<std::string, std::string> check;

void test_new_key(KVS &kvs, const std::string &key, const std::string &val)
{
  fprintf(stderr, "test_new_key(\"%s\",...)\n", key.c_str());
  std::string tmp="nope";
  assert(!kvs.has_key(key));
  assert(!kvs.get(key, tmp));
  kvs.set(key, val);
  check[key]=val;
  assert(kvs.has_key(key));
  assert(kvs.get(key, tmp));
  assert(tmp == val);
}

void test_overwrite_key(KVS &kvs, const std::string &key, const std::string &val)
{
  fprintf(stderr, "test_overwrite_key(\"%s\",...)\n", key.c_str());
  std::string tmp="nope";
  assert(kvs.has_key(key));
  assert(kvs.get(key, tmp));
  kvs.set(key, val);
  check[key]=val;
  assert(kvs.has_key(key));
  assert(kvs.get(key, tmp));
  assert(tmp == val);
}  

void test_invalid_key(KVS &kvs, const std::string &key)
{
  fprintf(stderr, "test_invalid_key(\"%s\")\n", key.c_str());

  try {
    kvs.has_key(key);
    assert(0);
  } catch (std::runtime_error) {}

  try {
    std::string val;
    kvs.get(key, val);
    assert(0);
  } catch (std::runtime_error) {}
  
  try {
    std::string val="foo";
    kvs.set(key, val);
    assert(0);
  } catch (std::runtime_error) {}
  
}

void test_read_modify_write(KVS &kvs, const std::string &key, const std::string &add, bool quiet=false)
{
  if (!quiet) fprintf(stderr, "test_read_modify_write(\"%s\")\n", key.c_str());
  KVSLocker locker(kvs, key);
  std::string val;
  kvs.get(key, val);
  val = add + val;
  kvs.set(key, val);
  check[key]=val;
  // Sleep 0-10 msec
  usleep(random()%10000);
  std::string test;
  kvs.get(key, test);
  assert(val==test);
}

void test_read_modify_write_thread(KVS *kvs, const std::string &key, const std::string &add)
{
  try {
    test_read_modify_write(*kvs, key, add, true);
  } catch (std::runtime_error &err) {
    fprintf(stderr, "test_read_modify_write_thread caught error '%s'\n", err.what());
    assert(0);
  }
}

void test_multithreaded_locking(KVS &kvs)
{
  fprintf(stderr, "test_multithreaded_locking()\n");
  std::string key="abcdef.ghijkl";
  kvs.set(key, "");
  check[key]="";
  unsigned nthreads=200;
  std::vector<boost::thread*> threads;
  for (unsigned int i=0; i<nthreads; i++) {
    threads.push_back(new boost::thread(test_read_modify_write_thread, &kvs, key, std::string(1, (char)i)));
  }
  for (unsigned int i=0; i<nthreads; i++) {
    threads[i]->join();
    delete threads[i];
  }
  std::string val;
  kvs.get(key, val);
  assert(val.size() == nthreads);
  for (unsigned int i=0; i<nthreads; i++) {
    assert(val.find(std::string(1, (char)i)) != std::string::npos);
  }
}

void confirm_all_keys(KVS &kvs)
{
  fprintf(stderr, "confirm_all_keys()\n");
  for (std::map<std::string, std::string>::const_iterator i = check.begin(); i != check.end(); i++) {
    assert(kvs.has_key(i->first));
    std::string val="nope";
    assert(kvs.get(i->first, val));
    assert(i->second == val);
  }
}

std::string generate_val(size_t len)
{
  std::string ret(len, ' ');
  for (size_t i = 0; i < len; i++) ret[i] = i%256;
  return ret;
}

int main(int argc, char **argv) {
  system("rm -rf test.kvs");
  system("mkdir test.kvs");
  {
    FilesystemKVS kvs("test.kvs");
    
    // Test simple key
    test_new_key(kvs, "abc", "123");
    
    // Test zero-length value
    test_new_key(kvs, "abcd", "");
    
    // Test compound keys
    test_new_key(kvs, "abc.def.ghi", "1234");
    test_new_key(kvs, "abc.def", "12345");
    test_new_key(kvs, "abc.def.g-hi.jk_l", "123456");

    // Test invalid keys
    test_invalid_key(kvs, "");
    test_invalid_key(kvs, ".");
    test_invalid_key(kvs, ".abc");
    test_invalid_key(kvs, "abc.");
    test_invalid_key(kvs, "abc..def");
    test_invalid_key(kvs, "abc/def");
    test_invalid_key(kvs, "abc*def");
    
    // Test overwriting existing key
    test_overwrite_key(kvs, "abc", "12345678");
    test_overwrite_key(kvs, "abc.def.g-hi.jk_l", "");
    
    // Test 1MB value
    std::string largeval = generate_val(1024*1024);
    test_new_key(kvs, "large.value", largeval);

    // Test lock
    test_read_modify_write(kvs, "abc", "123");
    test_read_modify_write(kvs, "abc.def", "123");
    test_read_modify_write(kvs, "abc.def.ghi", "123");
    test_read_modify_write(kvs, "abc.def.ghi.jkl", "123");
    test_read_modify_write(kvs, "abc.def.ghi.jkl.mno", "123");

    // Test multithreaded locking
    test_multithreaded_locking(kvs);
    
    // Double-check all keys
    confirm_all_keys(kvs);
  }
  fprintf(stderr, "Tests succeeded\n");
  return 0;
};
