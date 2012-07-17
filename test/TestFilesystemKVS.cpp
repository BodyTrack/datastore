// C
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/wait.h>

// C++
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// Local
#include "utils.h"

// Module to test
#include "FilesystemKVS.h"

std::map<std::string, std::string> check;

void test_new_key(KVS &kvs, const std::string &key, const std::string &val)
{
  fprintf(stderr, "test_new_key(\"%s\",...)\n", key.c_str());
  std::string tmp="nope";
  tassert(!kvs.has_key(key));
  tassert(!kvs.get(key, tmp));
  kvs.set(key, val);
  check[key]=val;
  tassert(kvs.has_key(key));
  tassert(kvs.get(key, tmp));
  tassert(tmp == val);
}

void test_overwrite_key(KVS &kvs, const std::string &key, const std::string &val)
{
  fprintf(stderr, "test_overwrite_key(\"%s\",...)\n", key.c_str());
  std::string tmp="nope";
  tassert(kvs.has_key(key));
  tassert(kvs.get(key, tmp));
  kvs.set(key, val);
  check[key]=val;
  tassert(kvs.has_key(key));
  tassert(kvs.get(key, tmp));
  tassert(tmp == val);
}  

void test_invalid_key(KVS &kvs, const std::string &key)
{
  fprintf(stderr, "test_invalid_key(\"%s\")\n", key.c_str());

  try {
    kvs.has_key(key);
    tassert(0);
  } catch (std::runtime_error) {}

  try {
    std::string val;
    kvs.get(key, val);
    tassert(0);
  } catch (std::runtime_error) {}
  
  try {
    std::string val="foo";
    kvs.set(key, val);
    tassert(0);
  } catch (std::runtime_error) {}
  
}

void test_read_modify_write(KVS &kvs, const std::string &key, const std::string &add, 
                            int count_per_process=1)
{
  for (int i = 0; i < count_per_process; i ++) {
    KVSLocker locker(kvs, key);
    std::string val;
    kvs.get(key, val);
    val = add + val;
    kvs.set(key, val);
    check[key]=val;
    // Sleep 0-10 msec
    //usleep(random()%10000);
    std::string test;
    kvs.get(key, test);
    tassert(val==test);
  }
}

void test_multithreaded_locking(KVS &kvs)
{
  fprintf(stderr, "test_multithreaded_locking()\n");
  std::string key="abcdef.ghijkl";
  kvs.set(key, "");
  check[key]="";
  unsigned nprocesses = 50;
  int count_per_process = 25;
  std::vector<int> pids;
  fprintf(stderr, "Making %d processes", nprocesses);
  for (unsigned int i=0; i<nprocesses; i++) {
    int child = fork();
    if (child) {
      pids.push_back(child);
    } else {
      test_read_modify_write(kvs, key, std::string(1, (char)i), count_per_process);
      fprintf(stderr, ".");
      exit(0);
    }
  }
  fprintf(stderr, "done\n");
  fprintf(stderr, "Waiting for %d processes", nprocesses);
  for (unsigned int i=0; i < nprocesses; i++) {
    int stat = 0;
    tassert(waitpid(pids[i], &stat, 0) == pids[i]);
    tassert(stat == 0);
    fprintf(stderr, ".");
  }
  fprintf(stderr, "done\n");
  std::string val;
  kvs.get(key, val);
  tassert_equals(val.size(), nprocesses * count_per_process);
  std::vector<int> count(nprocesses);
  for (unsigned int i=0; i < val.size(); i++) {
    count[val[i]]++;
  }
  for (unsigned int i=0; i<nprocesses; i++) {
    tassert_equals(count[i], count_per_process);
  }
}

void confirm_all_keys(KVS &kvs)
{
  fprintf(stderr, "confirm_all_keys()\n");

  std::set<std::string> inserted_set;

  for (std::map<std::string, std::string>::const_iterator i = check.begin(); i != check.end(); i++) {
    tassert(kvs.has_key(i->first));
    std::string val="nope";
    tassert(kvs.get(i->first, val));
    tassert(i->second == val);
    inserted_set.insert(i->first);
  }

  std::vector<std::string> subkeys;
  kvs.get_subkeys("", subkeys);
  std::set<std::string> subkey_set(subkeys.begin(), subkeys.end());

  tassert(subkey_set == inserted_set);
}

std::string generate_val(size_t len)
{
  std::string ret(len, ' ');
  for (size_t i = 0; i < len; i++) ret[i] = i%256;
  return ret;
}

void sys_check(const char *cmd) {
  if (system(cmd)) {
    fprintf(stderr, "Executing '%s' failed, aborting\n", cmd);
    exit(1);
  }
}

int main(int argc, char **argv) {
  sys_check("rm -rf test.kvs");
  sys_check("mkdir test.kvs");
  {
    FilesystemKVS kvs("test.kvs");
    
    confirm_all_keys(kvs);
    
    // Test simple key
    test_new_key(kvs, "abc", "123");
    
    // Test zero-length value
    test_new_key(kvs, "abcd", "");
    
    // Test compound keys
    test_new_key(kvs, "abc.def.ghi", "1234");
    test_new_key(kvs, "abc.def", "12345");
    test_new_key(kvs, "abc.def.g-hi.jk_l", "123456");

    confirm_all_keys(kvs);

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

    // Double-check all keys
    confirm_all_keys(kvs);

    // Test multithreaded locking
    test_multithreaded_locking(kvs);

    // confirm_all_keys will no longer work since we wrote from multiple processes
  }
  fprintf(stderr, "Tests succeeded\n");
  return 0;
};
