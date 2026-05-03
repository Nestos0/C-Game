#include "calc.h"
#include <stddef.h>

size_t next_power_of_2(size_t n)
{
	if (n == 0)
		return 1;
	n--; // 如果 n 本身是 2 的幂，减 1 可以保证结果还是 n
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFF // 如果是 64 位系统
	n |= n >> 32;
#endif
	return n + 1;
}
