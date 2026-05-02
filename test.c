#include "include/log.h"

int main()
{
	log4engine("./log.txt", "TEST%s\n", "你好");
	return 0;
}
