#include <assert.h>

#include <json/json.h>

#include "utils.h"

#include <string.h>

int main(int argc, char **argv)
{
  assert(rtrim(Json::FastWriter().write(Json::Value(0))) == "0");
  assert(rtrim(Json::FastWriter().write(Json::Value(0.0))) == "0");
  assert(rtrim(Json::FastWriter().write(Json::Value(0.01))) == "0.01");
  assert(rtrim(Json::FastWriter().write(Json::Value(10))) == "10");
  assert(rtrim(Json::FastWriter().write(Json::Value(123456789012345.0))) == "123456789012345");
  assert(rtrim(Json::FastWriter().write(Json::Value(2e20))) == "2e20");
  assert(rtrim(Json::FastWriter().write(Json::Value(2.01e20))) == "2.01e20");
  assert(rtrim(Json::FastWriter().write(Json::Value(2e-20))) == "2e-20");
  assert(rtrim(Json::FastWriter().write(Json::Value(2.01e-20))) == "2.01e-20");
  return 0;
}
