#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> splitString(std::string str, char sep = ',');

#define DECLARE_ENUM_WITH_TYPE(E, T, ...)                                                                    \
    enum E : T                                                                                          \
    {                                                                                                         \
        __VA_ARGS__                                                                                           \
    };                                                                                                        \
inline std::map<T, std::string> E##MapName(generateEnumMap<T>(#__VA_ARGS__));                                    \
inline std::string E##_name(E enumTmp) { return E##MapName[static_cast<T>(enumTmp)]; }                          \

#define DECLARE_ENUM(E, ...) DECLARE_ENUM_WITH_TYPE(E, int32_t, __VA_ARGS__)
template <typename T>
std::map<T, std::string> generateEnumMap(std::string strMap);
