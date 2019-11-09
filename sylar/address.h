#ifndef MYSYLAR_ADDRESS_H
#define MYSYLAR_ADDRESS_H

#include <memory>
#include <netinet/in.h> //for sockaddr_in
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

namespace sylar {

class IPAddress;
class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);
    static bool Lookup(std::vector<Address::ptr> &result,
                       const std::string &host, int family = AF_INET,
                       int type = 0, int protocol = 0);
    static Address::ptr LookupAny(const std::string &host, int family = AF_INET,
                                  int type = 0, int protocol = 0);
    static std::shared_ptr<IPAddress>
    LookupAnyIPAddress(const std::string &host, int family = AF_INET,
                       int type = 0, int protocol = 0);
    virtual ~Address() {}

    int getFamily() const;
    virtual const sockaddr *getAddr() const = 0;
    virtual sockaddr *getAddr()             = 0;
    virtual socklen_t getAddrLen() const    = 0;

    virtual std::ostream &insert(std::ostream &os) const = 0;
    std::string toString();

    bool operator<(const Address &rhs) const;
    bool operator==(const Address &rhs) const;
    bool operator!=(const Address &rhs) const;
};

class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    static IPAddress::ptr Create(const char *address, uint16_t port = 0);
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len)   = 0;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len)       = 0;

    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;
};

class IPV4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPV4Address> ptr;

    static IPV4Address::ptr Create(const char *address, uint16_t port = 0);

    IPV4Address(const sockaddr_in &address);
    IPV4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream &insert(std::ostream &os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in m_addr;
};

} // end namespace sylar

#endif