#ifndef MYSYLAR_SERVLET_H
#define MYSYLAR_SERVLET_H

#include "http.h"
#include "http_session.h"
#include "sylar/thread.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace sylar {
namespace http {

//一个Servlet对应一个或者一类URL解析
class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;
    Servlet(const std::string &name)
        : m_name(name) {}

    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                           HttpSession::ptr session) = 0;

    const std::string &getName() const { return m_name; }

protected:
    std::string m_name;
};

class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t(HttpRequest::ptr request,
                                  HttpResponse::ptr response,
                                  HttpSession::ptr session)>
        callback;

    FunctionServlet(callback cb);
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                           HttpSession::ptr session) override;

private:
    callback m_cb;
};

class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet();

    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                           HttpSession::ptr session) override;
};

class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType;

    ServletDispatch();
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                           HttpSession::ptr session) override;

    void addServlet(const std::string &uri, Servlet::ptr slt);
    void addServlet(const std::string &uri, FunctionServlet::callback cb);

    void addGlobServlet(const std::string &uri, Servlet::ptr slt);
    void addGlobServlet(const std::string &uri, FunctionServlet::callback cb);

    void delServlet(const std::string &uri);
    void delGlobServlet(const std::string &uri);

    void setDefault(Servlet::ptr v) { m_default = v; }
    Servlet::ptr getDefault() const { return m_default; }

    Servlet::ptr getServlet(const std::string &uri);
    Servlet::ptr getGlobServlet(const std::string &uri);

    //优先精准匹配，其次模糊匹配，最后返回默认
    Servlet::ptr getMatchedServlet(const std::string &uri);

private:
    RWMutexType m_mutex;

    //精准匹配servlet map, uri(sylar/xxx) -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;

    //模糊匹配servlet, uri(sylar/*) -> servlet
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;

    Servlet::ptr m_default;
};

} // namespace http
} // namespace sylar

#endif