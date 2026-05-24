/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 21:07:52
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-24 21:43:38
 * @FilePath: /beagle_play/include/dashboard_server/http_utils.hpp
 * @Description:  HTTP相关的工具函数和类型
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */
#pragma once

// Project
#include <dashboard_server/json_utils.hpp>

// Socket
#include <sys/socket.h>

// STL
#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace dashboard {

// HTTP相关工具函数和类型
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpReply {
    int code{200};
    std::string content_type{"text/plain; charset=utf-8"};
    std::string body;
};
std::string readFile(const std::string& path);
void sendResponse(int client_fd,
                  int code,
                  const std::string& content_type,
                  const std::string& body);
bool readHttpRequest(int client_fd, HttpRequest& request);

}  // namespace dashboard
