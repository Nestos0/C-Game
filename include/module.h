typedef int (*initcall_t)(void);

extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];
extern initcall_t __start_exitcalls[];
extern initcall_t __stop_exitcalls[];

#define APP_INIT(fn)                                                           \
	[[gnu::section("initcalls"), gnu::used, gnu::aligned(sizeof(void *))]] \
	static initcall_t __init_ptr_##fn = fn;

#define APP_EXIT(fn)                                                           \
	[[gnu::section("exitcalls"), gnu::used, gnu::aligned(sizeof(void *))]] \
	static initcall_t __exit_ptr_##fn = fn;

void all_modules_init();
void all_modules_exit();
