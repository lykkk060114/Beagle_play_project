#include <common/json_utils.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace gateway {

std::string trim(std::string text) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
    text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(), text.end());
    return text;
}

std::string json_escape(const std::string& input) {
    std::ostringstream oss;
    for (char ch : input) {
        switch (ch) {
            case '\\': oss << "\\\\"; break;
            case '"': oss << "\\\""; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch))
                        << std::dec << std::setfill(' ');
                } else {
                    oss << ch;
                }
                break;
        }
    }
    return oss.str();
}

bool parse_flat_json(const std::string& text,
                     std::map<std::string, std::string>& out,
                     std::string& error) {
    std::size_t pos = 0;
    auto skip_ws = [&]() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
    };
    auto read_string = [&](std::string& value) {
        if (pos >= text.size() || text[pos] != '"') {
            return false;
        }
        const std::size_t start = ++pos;
        const std::size_t end = text.find('"', start);
        if (end == std::string::npos) {
            return false;
        }
        value = text.substr(start, end - start);
        pos = end + 1;
        return true;
    };
    auto read_value = [&](std::string& value) {
        skip_ws();
        if (pos < text.size() && text[pos] == '"') {
            return read_string(value);
        }
        const std::size_t start = pos;
        while (pos < text.size() && text[pos] != ',' && text[pos] != '}') {
            ++pos;
        }
        value = trim(text.substr(start, pos - start));
        return !value.empty();
    };

    skip_ws();
    if (pos >= text.size() || text[pos++] != '{') {
        error = "expected object";
        return false;
    }

    while (true) {
        skip_ws();
        if (pos < text.size() && text[pos] == '}') {
            return true;
        }

        std::string key;
        std::string value;
        if (!read_string(key)) {
            error = "invalid key";
            return false;
        }
        skip_ws();
        if (pos >= text.size() || text[pos++] != ':') {
            error = "missing colon";
            return false;
        }
        if (!read_value(value)) {
            error = "invalid value";
            return false;
        }
        out[key] = value;

        skip_ws();
        if (pos < text.size() && text[pos] == ',') {
            ++pos;
            continue;
        }
        if (pos < text.size() && text[pos] == '}') {
            return true;
        }
        error = "missing comma or object end";
        return false;
    }
}

bool parse_double_token(const std::string& text, double& out) {
    try {
        std::size_t consumed = 0;
        out = std::stod(trim(text), &consumed);
        return consumed > 0;
    } catch (...) {
        return false;
    }
}

bool parse_int_token(const std::string& text, int& out) {
    try {
        std::size_t consumed = 0;
        long value = std::stol(trim(text), &consumed);
        if (consumed == 0) {
            return false;
        }
        out = static_cast<int>(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_bool_token(const std::string& text, bool& out) {
    const std::string value = trim(text);
    if (value == "true" || value == "1") {
        out = true;
        return true;
    }
    if (value == "false" || value == "0") {
        out = false;
        return true;
    }
    return false;
}

}  // namespace gateway
