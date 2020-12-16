#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> splitString(std::string str, char sep = ',');

#define STRING_REMOVE_CHAR(str, ch)                                         \
  str.erase(std::remove(str.begin(), str.end(), ch), str.end())

template <typename T>
std::map<T, std::string> generateEnumMap(std::string strMap)
{
  STRING_REMOVE_CHAR(strMap, ' ');
  STRING_REMOVE_CHAR(strMap, '(');

  std::vector<std::string> enumTokens(splitString(strMap));
  std::map<T, std::string> retMap;
  T inxMap;

  inxMap = 0;
  for (auto iter = enumTokens.begin(); iter != enumTokens.end(); ++iter) {
    /* Token: [EnumName | EnumName=EnumValue] */
    std::string enumName;
    if (iter->find('=') == std::string::npos) {
      enumName = *iter;
    } else {
      std::vector<std::string> enumNameValue(splitString(*iter, '='));
      enumName = enumNameValue[0];
      if (std::is_unsigned<T>::value) {
        inxMap = static_cast<T>(std::stoull(enumNameValue[1], 0, 0));
      } else {
        inxMap = static_cast<T>(std::stoll(enumNameValue[1], 0, 0));
      }
    }
    retMap[inxMap++] = enumName;
  }

  return retMap;
}

#define DECLARE_ENUM_WITH_TYPE(E, T, ...)                                   \
  enum E : T { __VA_ARGS__ };                                               \
                                                                            \
  inline const auto &E##MapName()                                           \
  {                                                                         \
    static auto map = generateEnumMap<T>(#__VA_ARGS__);                     \
    return map;                                                             \
  }                                                                         \
  inline const char *E##_name(E enumTmp)                                    \
  {                                                                         \
    return (E##MapName().at(static_cast<T>(enumTmp))).c_str();              \
  }                                                                         \
  inline const size_t E##_count(E enumTmp)                                  \
  {                                                                         \
    (void) enumTmp;                                                         \
    return E##MapName().size();                                             \
  }                                                                         \
  inline const bool E##_is_valid(T value)                                   \
  {                                                                         \
    return (E##MapName().find(value) != E##MapName().end());                \
  }                                                                         \
  inline std::string E##_by_name(E value)                                   \
  {                                                                         \
    return (E##MapName().at(value));                                        \
  }
