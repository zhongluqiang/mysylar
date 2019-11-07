#ifndef MYSYLAR_HOOK_SYS_CALL_H
#define MYSYLAR_HOOK_SYS_CALL_H
#include <unistd.h>

namespace sylar {
bool is_enable_hook_sys_call();
void enable_hook_sys_call();
}; // namespace sylar

extern "C" {
unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usec);
}

#endif