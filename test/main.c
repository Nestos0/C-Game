#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <stdarg.h>

#define DEFAULT_S_DATA_SIZE 255

typedef struct {
	char *data;
	size_t width;
	size_t mcapacity;
} String;

typedef struct {
	int range[2];
	char *tag;
} BBcode;

int is_full_width(wchar_t wc);

int get_strwidth(char *str);

int get_strwidth_spec(char *str);

int set_string(String *obj, char *str);

int mini_printf(const char *fmt, ...);

int main()
{
	setlocale(LC_ALL, "");

	String *test_str;

	test_str = (String *)malloc(sizeof(String));

	set_string(test_str, "C语言");
	const char *ptr = test_str->data;
	int len;

	printf("字符串: %s\n", test_str->data);

	while ((len = mblen(ptr, MB_CUR_MAX)) > 0) {
		printf("发现字符，占用字节数: %d\n", len);

		ptr += len;
	}

	if (len == -1) {
		printf("解析过程中遇到无效字符。\n");
	}

	printf("字符串长度 %ld\n", test_str->width);

	return 0;
}

int is_full_width(wchar_t wc)
{
	if (wc >= 0x1100 &&
	    (wc <= 0x115f || // Hangul Jamo
	     wc == 0x2329 || wc == 0x232a ||
	     (wc >= 0x2e80 && wc <= 0xa4cf && wc != 0x303f) || // CJK ... Yi
	     (wc >= 0xac00 && wc <= 0xd7a3) || // Hangul Syllables
	     (wc >= 0xf900 && wc <= 0xfaff) || // CJK Compatibility Ideographs
	     (wc >= 0xfe10 && wc <= 0xfe19) || // Vertical forms
	     (wc >= 0xfe30 && wc <= 0xfe6f) || // CJK Compatibility Forms
	     (wc >= 0xff00 && wc <= 0xff60) || // Fullwidth Forms
	     (wc >= 0xffe0 && wc <= 0xffe6)))
		return 1;
	if (!wc)
		return -1;
	return 0;
}

int get_strwidth(char *str)
{
	char *ptr = str;
	int len = 0;
	int ret = 0;
	wchar_t wc;
	while ((len = mblen(ptr, MB_CUR_MAX)) > 0) {
		if (len > 1) {
			mbtowc(&wc, ptr, len);
			ret += is_full_width(wc) + 1;
		} else
			++ret;
		ptr += len;
	}
	return ret;
}

int get_strwidth_spec(char *str)
{
	char *ptr = str;
	int len = 0;
	int ret = 0;
	while ((len = mblen(ptr, MB_CUR_MAX)) > 0) {
		ret += (len > 1) ? 2 : 1;
		ptr += len;
	}
	return ret;
}

int set_string(String *obj, char *str)
{
	int len = strlen(str);
	if (obj->data == NULL || obj->mcapacity < len) {
		int new_cap = (obj->data == NULL) ? DEFAULT_S_DATA_SIZE :
						    obj->mcapacity * 2;
		char *new_data = realloc(obj->data, sizeof(char) * new_cap);
		if (!new_data)
			return -1;
		obj->data = new_data;
		obj->mcapacity = new_cap;
	}

	strcpy(obj->data, str);
	obj->width = get_strwidth(str);
	return 0;
}

void print_int(int n)
{
	char buf[12]; // int32 最大是 10 位，加上符号和 \0 足够了
	int i = 0;
	unsigned int num;

	// 1. 处理负数
	if (n < 0) {
		putchar('-');
		num = (unsigned int)(-n); // 转为无符号防止 -2147483648 溢出
	} else {
		num = (unsigned int)n;
	}

	// 2. 逆序转换
	if (num == 0) {
		buf[i++] = '0';
	} else {
		while (num > 0) {
			buf[i++] = (num % 10) + '0';
			num /= 10;
		}
	}

	// 3. 反向输出缓冲区
	while (i > 0) {
		putchar(buf[--i]);
	}
}

int color_printf(const char *fmt, ...)
{
	const char *p = fmt;

	va_list args;
	va_start(args, fmt);

	int op[2] = { 0 };

	int bb_count = 0;
	char buf[8];
	char bbcode[8];

	int i = 0;

	while (*p != '\0') {
		if (*p == '[') {
			op[(op[0]) ? 0 : 1] = i;
			++p;
			++i;
			if (op[0] != 0 && *p != '/') {
				op[0] = 0;
				continue;
			}
			while (*p != ']') {
				buf[bb_count++] = *p;
				++p;
				++i;
			}
			bb_count = 0;
		}
		++p;
		++i;
	}

	va_end(args);
	return 0;
}
