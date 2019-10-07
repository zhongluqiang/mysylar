#ifndef MYSYLAR_UTIL_H
#define MYSYLAR_UTIL_H

#include <execinfo.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace sylar {

pid_t GetThreadId();
uint32_t GetFiberId();

void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);
std::string BacktraceToString(int size, int skip = 2,
                              const std::string &prefix = "");

} // namespace sylar

#endif // MYSYLAR_UTIL_H
