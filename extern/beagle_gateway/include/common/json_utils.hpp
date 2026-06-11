#pragma once

#include <map>
#include <string>

namespace gateway {

std::string trim(std::string text);
std::string json_escape(const std::string& input);
bool parse_flat_json(const std::string& text,
                     std::map<std::string, std::string>& out,
                     std::string& error);
bool parse_double_token(const std::string& text, double& out);
bool parse_int_token(const std::string& text, int& out);
bool parse_bool_token(const std::string& text, bool& out);

}  // namespace gateway
