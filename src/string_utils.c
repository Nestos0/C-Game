#include "string_utils.h"

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

int get_strwidth_spec(const char *str)
{
	if (!str)
		return 0;
	const char *ptr = str;
	int len = 0;
	int ret = 0;
	while ((len = mblen(ptr, MB_CUR_MAX)) > 0) {
		ret += (len > 1) ? 2 : 1;
		ptr += len;
	}
	return ret;
}

int set_string(String *obj, const char *str)
{
	if (!obj || !str)
		return -1;

	obj->is_flex = false;
	int len = strlen(str);
	if (obj->data == nullptr || obj->mcapacity < len) {
		int new_cap = (obj->data == nullptr) ? DEFAULT_STRING_CAPACITY :
						       obj->mcapacity * 2;
		char *new_data = realloc(obj->data, sizeof(char) * new_cap);
		if (!new_data)
			return -1;
		obj->data = new_data;
		obj->mcapacity = new_cap;
	}

	strcpy(obj->data, str);
	obj->mlen = strlen(str);
	obj->width = get_strwidth_spec(str);
	return 0;
}

char *get_string_data(String *obj_ptr)
{
	char *ret = (obj_ptr)->is_flex ? (obj_ptr)->flex_data : (obj_ptr)->data;
	return ret;
}

const char *get_ansi_code(const char *tag)
{
	if (tag[0] == '/')
		return "\x1b[0m";
	for (int i = 0; tags[i].name != NULL; i++) {
		if (strcmp(tag, tags[i].name) == 0)
			return tags[i].code;
	}
	return "";
}

char *parse_tag_rec(const char *input, int pair_seq)
{
	size_t in_size = sizeof(input) + sizeof(char) * 8;
	char *output = calloc(1, in_size);

	char stack[MAX_TAG_CAPACITY][MAX_TAG_LENGTH];
	int top = -1;

	const char *p = input;
	char *dest = output;

	while (*p) {
		if (*p == pair_map[pair_seq].start) {
			const char *start = ++p;
			while (*p && *p != pair_map[pair_seq].end)
				++p;

			if (*p == pair_map[pair_seq].end) {
				size_t tag_len = p - start;
				if (tag_len < MAX_TAG_LENGTH) {
					char tag[MAX_TAG_LENGTH] = { 0 };
					strncpy(tag, start, tag_len);

					if (tag[0] == '/') {
						if (get_ansi_code(tag + 1)) {
							if (top >= 0)
								--top;
							const char *code =
								(top >= 0) ?
									get_ansi_code(
										stack[top]) :
									"\x1b[0m";

							dest += sprintf(dest,
									"%s",
									code);
						} else {
							dest += sprintf(dest,
									"[%s]",
									tag);
						}
					} else {
						if (top <
						    MAX_STACK_CAPACITY - 1) {
							strncpy(stack[++top],
								tag,
								MAX_TAG_LENGTH);
						}
						dest += sprintf(
							dest, "%s",
							get_ansi_code(tag));
					}
				}
			}
			++p;
			continue;
		}
		*dest++ = *p++;
	}
	return output;
}

char *parse_bbcode(const char *fmt)
{
	return parse_tag_rec(fmt, 0);
}

int print_color(const char *format, ...)
{
	char raw_buffer[1024];
	va_list args;

	va_start(args, format);
	vsnprintf(raw_buffer, sizeof(raw_buffer), format, args);
	va_end(args);

	char *interpreted = parse_bbcode(raw_buffer);
	if (!interpreted)
		return -1;

	int len = printf("%s", interpreted);
	free(interpreted);
	return len;
}
