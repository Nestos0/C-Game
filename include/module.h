typedef int (*initcall_t)(void);

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];

#define init_register(fn)                 \
	static initcall_t __init_ptr_##fn \
		__attribute__((section("initcalls"), used, aligned(sizeof(int *)))) = fn;

void all_modules_init();
