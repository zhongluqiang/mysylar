#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
/*
typedef struct ucontext_t {
    struct ucontext_t *uc_link; //当前上下文结束时用于替换的上下文
    sigset_t uc_sigmask;
    stack_t uc_stack;           //当前上下文使用的栈
    mcontext_t uc_mcontext;
    ...
} ucontext_t;
*/

/*
makecontext()用于生成一个ucontext_t上下文对象，其实就是将一个函数和具体的ucontext_t
变量进行关联，当函数执行时，将使用ucontext_t指定的栈空间，当函数结束时，将恢复ucontext_t
的uc_link指定的上下文。生成的上下文不会立即激活，而是要使用setcontext或swapcontext进行激活，
激活上下文后，与之关联的函数也就开始执行了。
 */

/*
swapcontext()用于将当前上下文替换成指定的上下文，并且将当前的上下文保存在指定的参数里，
这个函数不会返回，而是会跳转到指定的上下文的函数中执行
*/

static ucontext_t uctx_main, uctx_func1, uctx_func2;

#define handle_error(msg)                                                      \
    do {                                                                       \
        perror(msg);                                                           \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

static void func1(void) {
    printf("func1: started\n");
    printf("func1: swapcontex(&uctx_func1, &uctx_func2)\n");
    if (swapcontext(&uctx_func1, &uctx_func2) == -1)
        handle_error("swapcontext");
    printf("func1:returning\n");
}

static void func2(void) {
    printf("func2: started\n");
    printf("func2: swapcontex(&uctx_func2, &uctx_func1)\n");
    if (swapcontext(&uctx_func2, &uctx_func1) == -1)
        handle_error("swapcontext");
    printf("func2:returning\n");
}

int main(int argc, char *argv[]) {
    char func1_stack[16384];
    char func2_stack[16384];

    if (getcontext(&uctx_func1) == -1)
        handle_error("getcontext");

    uctx_func1.uc_link          = &uctx_main;
    uctx_func1.uc_stack.ss_sp   = func1_stack;
    uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
    makecontext(
        &uctx_func1, func1,
        0); // uctx_func1作为func1执行时的上下文，其返回时恢复uctx_main指定的上下文

    if (getcontext(&uctx_func2) == -1)
        handle_error("getcontext");

    uctx_func2.uc_link          = (argc > 1) ? NULL : &uctx_func1;
    uctx_func2.uc_stack.ss_sp   = func2_stack;
    uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
    makecontext(
        &uctx_func2, func2,
        0); // uctx_func2作为func2执行时的上下文，其返回时恢复uctx_func1指定的上下文

    printf("main: swapcontext(&uctx_main, &uctx_func2)\n");
    if (swapcontext(&uctx_main, &uctx_func2) ==
        -1) //将当前上下文替换为mctx_func2，即激活uctx_func2指定的上下文，同时将旧的当前上下文，即原main函数的上下文保存在uctx_main里
        handle_error("swapcontext");

    printf("main: exiting\n");
    exit(EXIT_SUCCESS);
}
