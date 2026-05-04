#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

struct HttpResponse {
    int statusCode;
    std::string statusMessage;
    std::map<std::string, std::string> headers;
    std::string body;
};

#endif