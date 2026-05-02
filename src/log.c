#include <stdarg.h>
#include <stdio.h>

int log4engine(const char *file, const char *format, ...)
{
	FILE *fp = fopen(file, "a+");
	va_list args;

	va_start(args, format);
	vfprintf(fp, format, args);
	va_end(args);
	fclose(fp);
	return 0;
}
