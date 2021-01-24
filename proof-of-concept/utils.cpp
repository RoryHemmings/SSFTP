#include "utils.h"

std::string colorText(const std::string& message)
{
  std::string ret = "\033[31m" + message + "\033[0m";
  return ret;
}
