#ifndef MYSYLAR_HTTP_PARSER_H
#define MYSYLAR_HTTP_PARSER_H

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace sylar {
namespace http {

class HttpRequestParser{
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();

    //解析协议，返回实际解析的长度，并且将已解析的数据移除
    size_t excute(char *data, size_t len);
    int isFinished();
    int hasError();
    HttpRequest::ptr getData() const {return m_data;}
    void setError(int v) {m_error = v;}
    uint64_t getContentLength();
    const http_parser& getParser() const {return m_parser;}

public:
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBodySize();

private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    int m_error; //1000:invalid method, 1001:invalid version, 1002:invalid field
};

class HttpResponseParser{
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();

    size_t execute(char *data, size_t len, bool chunk);
    int isFinished();
    int hasError();
    HttpResponse::ptr getData() const {return m_data;}
    void setError(int v) {m_error = v;}
    uint64_t getContentLength();
    const httpclient_parser &getParser() const {return m_parser;}

public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();

private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    int m_error;
};

}
}

#endif