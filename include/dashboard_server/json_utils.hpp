/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 21:07:20
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-24 22:06:37
 * @FilePath: /beagle_play/include/dashboard_server/json_utils.hpp
 * @Description: json处理。 用于 前端和后端的通信
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */
#pragma once

// STL
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

namespace dashboard {

// 字符串处理和JSON相关的工具函数
std::string trim(std::string text);
std::string toLowerCopy(std::string text);
std::string jsonEscape(const std::string& input);
std::string jsonNumber(double value);
std::string jsonNumber(long long value);
std::string jsonBool(bool value);
bool parseFlatJsonObject(const std::string& text,
                         std::map<std::string, std::string>& out,
                         std::string& error);
bool parseDoubleToken(const std::string& text, double& out);
bool parseIntToken(const std::string& text, int& out);

}  // namespace dashboard
