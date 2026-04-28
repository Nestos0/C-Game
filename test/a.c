#include <stdio.h>
#include <string.h>

int main()
{
	char text[] = "TEXT[";
	char *p = text;
	while (*p && *p != '[')
		++p;
	size_t len = p - text;
	char dest[len];
	strncpy(dest, text, len);

	printf("%s\n", dest);
}
