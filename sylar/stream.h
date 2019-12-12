#ifndef MYSYLAR_STREAM_H
#define MYSYLAR_STREAM_H

#include "bytearray.h"
#include <memory>

namespace sylar {

class Stream {
public:
    typedef std::shared_ptr<Stream> ptr;

    virtual ~Stream() {}

    virtual int read(void *buffer, size_t length)      = 0;
    virtual int read(ByteArray::ptr ba, size_t length) = 0;
    virtual int readFixsize(void *buffer, size_t length);
    virtual int readFixsize(ByteArray::ptr, size_t length);

    virtual int write(const void *buffer, size_t length) = 0;
    virtual int write(ByteArray::ptr ba, size_t length)  = 0;
    virtual int writeFixSize(const void *buffer, size_t length);
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);

    virtual void close() = 0;
};

} // namespace sylar

#endif