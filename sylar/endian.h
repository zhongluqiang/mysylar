#ifndef MYSYLAR_ENDIAN_H
#define MYSYLAR_ENDIAN_H

#include <byteswap.h>
#include <endian.h>
#include <stdint.h>
#include <type_traits>

#define SYLAR_LITTLE_ENDIAN 1
#define SYLAR_BIG_ENDIAN 2

namespace sylar {

// 8字节的字节序转换
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

// 4字节的字节序转换
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

// 2字节的字节序列转换
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN
template <class T> T byteswapOnLittleEndian(T t) { return t; }
template <class T> T byteswapOnBigEndian(T t) { return byteswap(t); }
#else
template <class T> T byteswapOnLittleEndian(T t) { return byteswap(t); }
template <class T> T byteswapOnBigEndian(T t) { return t; }
#endif

} // namespace sylar

#endif