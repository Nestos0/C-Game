#include "utils.h"

void init_array(Array *buf, int target_capacity)
{
	(buf)->len = 0;
	(buf)->capacity = target_capacity;
	(buf)->data = malloc(sizeof(int) * (buf)->capacity);
}
