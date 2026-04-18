#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#define DEFAULT_S_DATA_SIZE 255

typedef struct {
	char *data;
	size_t width;
	size_t mcapacity;
} String;

int is_full_width(wchar_t wc);

int get_strwidth(char *str);

int get_strwidth_spec(char *str);

int set_string(String *obj, char *str);

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
	wchar_t wc;
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
