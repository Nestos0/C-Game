#include "utils.h"

void init_array(Array *buf, int target_capacity)
{
	(buf)->len = 0;
	(buf)->capacity = target_capacity;
	(buf)->data = malloc(sizeof(int) * (buf)->capacity);
}

void *safe_malloc(size_t size)
{
	void *p = malloc(size);
	if (!p)
		return nullptr;

	MemoryNode *node = (MemoryNode *)malloc(sizeof(MemoryNode));
	node->ptr = p;
	node->next = g_memory_pool;
	g_memory_pool = node;

	return p;
}

void *safe_calloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);
	if (!p)
		return nullptr;

	MemoryNode *node = (MemoryNode *)malloc(sizeof(MemoryNode));
	node->ptr = p;
	node->next = g_memory_pool;
	g_memory_pool = node;

	return p;
}

void cleanup_all_memory()
{
	MemoryNode *current = g_memory_pool;
	while (current != nullptr) {
		MemoryNode *temp = current;
		free(current->ptr);
		current = current->next;
		free(temp);
	}
	g_memory_pool = nullptr;
	printf("所有内存已安全释放。\n");
}
