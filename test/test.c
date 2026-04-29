// file: demo.c
#include <stdio.h>

typedef void (*initcall_t)(void);

/* 声明 section 边界（GNU ld / lld 自动提供） */
extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];

/* module_init 宏 */
#define module_init(fn) \
    static initcall_t __init_##fn \
    __attribute__((section("initcalls"), used)) = fn;

/* -------- 测试函数 -------- */

void init_a(void) {
    printf("init_a called\n");
}
module_init(init_a);

void init_b(void) {
    printf("init_b called\n");
}
module_init(init_b);

/* -------- 主程序 -------- */

int main(void) {
    printf("running initcalls...\n");

    for (initcall_t *p = __start_initcalls;
         p < __stop_initcalls;
         ++p) {
        (*p)();
    }

    return 0;
}
