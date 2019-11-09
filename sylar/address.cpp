#include "address.h"
#include "log.h"
#include <arpa/inet.h>
#include <netdb.h> //for getaddrinfo
#include <sstream>
#include <string.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

//生成连续bits位为1的掩码，例如0xffffff00对应bits为24
template <class T> static T CreateMask(uint32_t bits) {
    return (~0) << (sizeof(T) * 8 - bits);
}

Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
    if (!addr) {
        return nullptr;
    }

    Address::ptr result;
    switch (addr->sa_family) {
    case AF_INET:
        result.reset(new IPV4Address(*(const sockaddr_in *)addr));
        break;
    default:
        SYLAR_LOG_ERROR(g_logger) << "sockaddr family not support";
        break;
    }

    return result;
}

bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family, int type, int protocol) {
    addrinfo hints, *results, *next;
    hints.ai_flags     = 0;
    hints.ai_family    = family;
    hints.ai_socktype  = type;
    hints.ai_protocol  = protocol;
    hints.ai_addrlen   = 0;
    hints.ai_canonname = nullptr;
    hints.ai_addr      = nullptr;
    hints.ai_next      = nullptr;

    std::string node;
    const char *service = nullptr;

    if (!host.empty()) {
        service = (const char *)memchr(host.c_str(), ':', host.size());
        node    = host.substr(0, service - host.c_str());
    }

    if (node.empty()) {
        node = host;
    }

    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if (error) {
        SYLAR_LOG_ERROR(g_logger)
            << "Address::Lookup getaddress(" << host << ", " << family << ", "
            << type << ") err=" << errno << "errstr=" << strerror(errno);
        return false;
    }

    next = results;
    while (next) {
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return true;
}

Address::ptr Address::LookupAny(const std::string &host, int family, int type,
                                int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host, int family,
                                           int type, int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        for (auto &i : result) {
            std::cout << i->toString() << std::endl;
        }
        for (auto &i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) {
                return v;
            }
        }
    }
    return nullptr;
}

int Address::getFamily() const { return getAddr()->sa_family; }

std::string Address::toString() {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address &rhs) const {
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result       = memcmp(getAddr(), rhs.getAddr(), minlen);
    if (result < 0) {
        return true;
    } else if (result > 0) {
        return false;
    } else if (getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}

bool Address::operator==(const Address &rhs) const {
    return getAddrLen() == rhs.getAddrLen() &&
           memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address &rhs) const { return !(*this == rhs); }

IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags  = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address, nullptr, &hints, &results);
    if (error) {
        SYLAR_LOG_ERROR(g_logger)
            << "IPAddress::Create(" << address << ", " << port
            << ") error=" << error << " errno=" << errno
            << " errstr=" << strerror(errno);
        return nullptr;
    }

    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
            Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));

        if (result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

IPV4Address::ptr IPV4Address::Create(const char *address, uint16_t port) {
    IPV4Address::ptr rt(new IPV4Address);
    rt->m_addr.sin_port = htons(port);
    int result          = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if (result <= 0) {
        SYLAR_LOG_ERROR(g_logger)
            << "IPV4Addr::Create(" << address << ", " << port
            << ") rt=" << result << "errno=" << errno
            << "errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPV4Address::IPV4Address(const sockaddr_in &address) { m_addr = address; }

IPV4Address::IPV4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family      = AF_INET;
    m_addr.sin_port        = htons(port);
    m_addr.sin_addr.s_addr = htonl(address);
}

sockaddr *IPV4Address::getAddr() { return (sockaddr *)&m_addr; }

const sockaddr *IPV4Address::getAddr() const { return (sockaddr *)&m_addr; }

socklen_t IPV4Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream &IPV4Address::insert(std::ostream &os) const {
    uint32_t addr = ntohl(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "." << (addr & 0xff);
    os << ":" << ntohs(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPV4Address::broadcastAddress(uint32_t prefix_len) {
    if (prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |= ~htonl(CreateMask<uint32_t>(prefix_len));
    return IPV4Address::ptr(new IPV4Address(baddr));
}

IPAddress::ptr IPV4Address::networkAddress(uint32_t prefix_len) {
    if (prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in naddr(m_addr);
    naddr.sin_addr.s_addr &= htonl(CreateMask<uint32_t>(prefix_len));
    return IPV4Address::ptr(new IPV4Address(naddr));
}

IPAddress::ptr IPV4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family      = AF_INET;
    subnet.sin_addr.s_addr = htonl(CreateMask<uint32_t>(prefix_len));
    return IPV4Address::ptr(new IPV4Address(subnet));
}

uint32_t IPV4Address::getPort() const { return ntohs(m_addr.sin_port); }

void IPV4Address::setPort(uint16_t v) { m_addr.sin_port = htons(v); }

} // end namespace sylar
