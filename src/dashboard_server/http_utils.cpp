#include <dashboard_server/http_utils.hpp>

namespace dashboard {

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

static const char* statusText(int code) {
    switch (code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

void sendResponse(int client_fd,
                  int code,
                  const std::string& content_type,
                  const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << code << " " << statusText(code) << "\r\n";
    oss << "Content-Type: " << content_type << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body;
    const std::string response = oss.str();
    send(client_fd, response.c_str(), response.size(), 0);
}

bool readHttpRequest(int client_fd, HttpRequest& request) {
    char buffer[4096];
    const ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
    if (n <= 0) {
        return false;
    }

    std::string raw(buffer, static_cast<std::size_t>(n));
    const std::size_t header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return false;
    }

    std::string header_text = raw.substr(0, header_end);
    request.body = raw.substr(header_end + 4);

    std::istringstream header_stream(header_text);
    std::string line;
    if (!std::getline(header_stream, line)) {
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    {
        std::istringstream first_line(line);
        if (!(first_line >> request.method >> request.path >> request.version)) {
            return false;
        }
    }

    while (std::getline(header_stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = toLowerCopy(trim(line.substr(0, colon)));
        std::string value = trim(line.substr(colon + 1));
        request.headers[key] = value;
    }

    std::size_t content_length = 0;
    const auto it = request.headers.find("content-length");
    if (it != request.headers.end()) {
        content_length = static_cast<std::size_t>(std::stoul(it->second));
    }

    while (request.body.size() < content_length) {
        const ssize_t more = recv(client_fd, buffer, sizeof(buffer), 0);
        if (more <= 0) {
            return false;
        }
        request.body.append(buffer, static_cast<std::size_t>(more));
    }

    if (request.body.size() > content_length) {
        request.body.resize(content_length);
    }

    return true;
}

}  // namespace dashboard
