#ifndef MYSYLAR_UTIL_H
#define MYSYLAR_UTIL_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace sylar {

pid_t GetThreadId();
uint32_t GetFiberId();

} // namespace sylar

#endif // MYSYLAR_UTIL_H
