#include <dashboard_server/json_utils.hpp>

namespace dashboard {

std::string trim(std::string text) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
    text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(), text.end());
    return text;
}

std::string toLowerCopy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

std::string jsonEscape(const std::string& input) {
    std::ostringstream oss;
    for (char ch : input) {
        switch (ch) {
            case '\\': oss << "\\\\"; break;
            case '"': oss << "\\\""; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    oss << "\\u";
                    oss << std::hex << std::setw(4) << std::setfill('0')
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

std::string jsonNumber(double value) {
    std::ostringstream oss;
    oss << std::setprecision(10) << std::defaultfloat << value;
    return oss.str();
}

std::string jsonNumber(long long value) {
    return std::to_string(value);
}

std::string jsonBool(bool value) {
    return value ? "true" : "false";
}

bool parseFlatJsonObject(const std::string& text,
                         std::map<std::string, std::string>& out,
                         std::string& error) {
    std::size_t pos = 0;
    auto skipWs = [&]() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
    };
    auto readString = [&](std::string& value) {
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
    auto readValue = [&](std::string& value) {
        skipWs();
        if (pos < text.size() && text[pos] == '"') {
            return readString(value);
        }
        const std::size_t start = pos;
        while (pos < text.size() && text[pos] != ',' && text[pos] != '}') {
            ++pos;
        }
        value = trim(text.substr(start, pos - start));
        return !value.empty();
    };

    skipWs();
    if (pos >= text.size() || text[pos++] != '{') {
        error = "expected object";
        return false;
    }

    while (true) {
        skipWs();
        if (pos < text.size() && text[pos] == '}') {
            return true;
        }

        std::string key;
        std::string value;
        if (!readString(key)) {
            error = "invalid key";
            return false;
        }
        skipWs();
        if (pos >= text.size() || text[pos++] != ':') {
            error = "missing colon";
            return false;
        }
        if (!readValue(value)) {
            error = "invalid value";
            return false;
        }
        out[key] = value;

        skipWs();
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

bool parseDoubleToken(const std::string& text, double& out) {
    try {
        std::size_t consumed = 0;
        out = std::stod(trim(text), &consumed);
        return consumed > 0;
    } catch (...) {
        return false;
    }
}

bool parseIntToken(const std::string& text, int& out) {
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

}  // namespace dashboard
