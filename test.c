#include <stdio.h>
#include <termios.h>

int main()
{
	printf("\033[H\033[2J");
	fflush(stdout);

	while (1)
		printf("");
	return 0;
}
