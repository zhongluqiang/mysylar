#ifndef MYSYLAR_TCP_SERVER_H
#define MYSYLAR_TCP_SERVER_H

#include "address.h"
#include "scheduler.h"
#include "socket.h"
#include <memory>

namespace sylar {

class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer();
    virtual ~TcpServer();
    virtual bool bind(sylar::Address::ptr addr);
    virtual bool start();
    virtual void stop();

    void setName(const std::string &v) { m_name = v; }
    std::string getName() const { return m_name; }
    bool isStop() const { return m_isStop; }

protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);

private:
    Socket::ptr m_sock;
    Scheduler m_worker;
    Scheduler m_acceptWorker;
    std::string m_name;
    bool m_isStop;
};

} // namespace sylar

#endif