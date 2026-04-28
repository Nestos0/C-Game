#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#define MAX_TAG_LEN 16
#define MAX_COLOR_STACK 10

// ANSI 颜色映射表
typedef struct {
	const char *name;
	const char *code;
} ColorMap;

static const ColorMap colors[] = {
	{ "red", "\x1b[31m" },	   { "green", "\x1b[32m" },
	{ "yellow", "\x1b[33m" },  { "blue", "\x1b[34m" },
	{ "magenta", "\x1b[35m" }, { "cyan", "\x1b[36m" },
	{ "white", "\x1b[37m" },   { NULL, "\x1b[0m" } // 默认重置
};

// 获取 ANSI 转义码
const char *find_ansi(const char *tag)
{
	if (tag[0] == '/')
		return "\x1b[0m";
	for (int i = 0; colors[i].name != NULL; i++) {
		if (strcmp(tag, colors[i].name) == 0)
			return colors[i].code;
	}
	return "";
}

/**
 * 核心逻辑：将 [color] 标签转换为 ANSI 序列
 * 这是一个轻量级的流式解析实现，不需要复杂的链表存储
 */
char *translate_bbcode(const char *input)
{
	size_t in_len = strlen(input);
	// 预留足够空间（ANSI 码通常比标签短，但为了安全分配 2 倍空间）
	char *output = calloc(1, in_len * 2 + 128);
	if (!output)
		return NULL;

	const char *p = input;
	char *out_ptr = output;

	// 颜色栈：用于处理嵌套（虽然 ANSI 颜色通常是覆盖式的，但栈可以帮助处理 [/]）
	char stack[MAX_COLOR_STACK][MAX_TAG_LEN];
	int top = -1;

	while (*p) {
		if (*p == '[') {
			const char *start = ++p;
			while (*p && *p != ']')
				p++;

			if (*p == ']') {
				size_t tag_len = p - start;
				if (tag_len < MAX_TAG_LEN) {
					char tag[MAX_TAG_LEN] = { 0 };
					strncpy(tag, start, tag_len);

					if (tag[0] == '/') {
						char *str = (char *)find_ansi(
							tag + 1);
						if (str != NULL &&
						    str[0] != '\0') {
							if (top >= 0)
								top--;
							const char *code =
								(top >= 0) ?
									find_ansi(
										stack[top]) :
									"\x1b[0m";
							out_ptr += sprintf(
								out_ptr, "%s",
								code);
						} else {
							out_ptr += sprintf(
								out_ptr, "[%s]",
								tag);
						}
					} else {
						// 处理开始标签 [color]
						if (top < MAX_COLOR_STACK - 1) {
							strncpy(stack[++top],
								tag,
								MAX_TAG_LEN);
						}
						out_ptr +=
							sprintf(out_ptr, "%s",
								find_ansi(tag));
					}
				}
				p++; // 跳过 ']'
				continue;
			}
			p = start -
			    1; // 回溯：如果不是合法的标签，按普通字符处理
		}
		*out_ptr++ = *p++;
	}

	// 强制重置颜色防止污染终端
	sprintf(out_ptr, "\x1b[0m");
	return output;
}

/**
 * 格式化彩色打印
 */
int print_color(const char *format, ...)
{
	char raw_buffer[1024];
	va_list args;

	// 1. 处理 printf 风格的格式化
	va_start(args, format);
	vsnprintf(raw_buffer, sizeof(raw_buffer), format, args);
	va_end(args);

	// 2. 解析 BBCode 标签
	char *interpreted = translate_bbcode(raw_buffer);
	if (!interpreted)
		return -1;

	// 3. 输出并清理
	int len = printf("%s", interpreted);
	free(interpreted);
	return len;
}

int main()
{
	// 测试嵌套与并列
	print_color(
		"[red]Critical Error:[/red] [yellow]Disk usage is at [magenta]%d%%[/magenta][/yellow]\n",
		85);

	print_color(
		"[cyan]System [white]Message[/white]:[/cyan] Welcome, [white]%s[/white].\n",
		"Admin");

	// 测试未闭合标签（安全性测试）
	print_color(
		"[blue]Blue text with [green]green nested[/blue] and reset.\n");

	print_color(
		"[blue][/arst]Blue Text[/blue]End [green]1[/green] [green]1[/green] [green]1[/green] [green]1[/green] [green]1[/green] [green]1[/green] [green]1[/green] [green]1[/green] [green]1[/green] [blue]Blue [green]Green[/green]Blue[/blue] \n");

	return 0;
}
