#include "module.h"
#include <locale.h>
#include <text/utf8.h>

int get_utf8_width(uint8_t byte)
{
	if ((byte & 0x80) == 0x00)
		return 1; // 0xxxxxxx
	if ((byte & 0xE0) == 0xC0)
		return 2; // 110xxxxx
	if ((byte & 0xF0) == 0xE0)
		return 3; // 1110xxxx
	if ((byte & 0xF8) == 0xF0)
		return 4; // 11110xxx
	return 1;
}

int __utf8_init(void)
{
	setlocale(LC_ALL, "");
	return 0;
}
init_register(__utf8_init);
