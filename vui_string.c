#include <stdarg.h>
#include <string.h>

// for vsnprintf
#include <stdio.h>

#include "vui_debug.h"

#include "vui_mem.h"

#include "vui_utf8.h"

#include "vui_string.h"


vui_string* vui_string_new_prealloc_at(vui_string* str, size_t maxn)
{
	if (str == NULL)
	{
		str = vui_new(vui_string);
	}

	str->n = 0;
	str->maxn = maxn;
	str->s = vui_new_array(char, str->maxn);
	str->s[0] = 0;

	return str;
}

vui_string* vui_string_new_array_at(vui_string* str, size_t n, const char* s)
{
	str = vui_string_new_prealloc_at(str, n);

	vui_string_putn(str, n, s);

	return str;
}

vui_string* vui_string_new_str_at(vui_string* str, const char* s)
{
	str = vui_string_new_at(str);

	vui_string_puts(str, s);

	return str;
}

vui_string* vui_string_dup_at(vui_string* str, const vui_string* orig)
{
	str = vui_string_new_prealloc_at(str, orig->maxn);

	memcpy(str->s, orig->s, orig->n);

	str->n = orig->n;

	return str;
}

void vui_string_kill(vui_string* str)
{
	if (str == NULL) return;

	vui_string_dtor(str);

	vui_free(str);
}

void vui_string_dtor(vui_string* str)
{
	if (str == NULL) return;

	str->n = str->maxn = 0;

	if (str->s == NULL) return;

	vui_free(str->s);

	str->s = NULL;
}

char* vui_string_release(vui_string* str)
{
	if (str == NULL) return NULL;

	vui_string_shrink(str);

	char* s = str->s;

	vui_free(str);

	return s;
}

void vui_string_release_replace(vui_string* str, char** s)
{
	if (str == NULL) return;

	if (s == NULL) return;

	char* s_old = *s;

	*s = vui_string_release(str);

	if (s_old != NULL)
	{
		vui_free(s_old);
	}
}


static inline void vui_string_grow(vui_string* str, size_t maxn_new)
{
#if defined(VUI_DEBUG) && defined(VUI_DEBUG_STRING)
	printf("realloc: %zd, %zd -> %zd\n", str->n, str->maxn, maxn_new);
#endif
	str->maxn = maxn_new;
	str->s = vui_resize_array(str->s, char, str->maxn);
}


char* vui_string_shrink(vui_string* str)
{
	if (str == NULL) return NULL;

#if defined(VUI_DEBUG) && defined(VUI_DEBUG_STRING)
	vui_debugf("shrink(\"%s\")\n", vui_string_get(str));
#endif

	vui_string_grow(str, str->n + 1);

	return vui_string_get(str);
}


void vui_string_putc(vui_string* str, char c)
{
	if (str == NULL) return;

#if defined(VUI_DEBUG) && defined(VUI_DEBUG_STRING)
	vui_debugf("putc(\"%s\", '%c')\n", vui_string_get(str), c);
#endif

	// make room for two more chars: c and null terminator
	if (str->maxn < str->n + 2)
	{
		vui_string_grow(str, (str->n + 2)*2);
	}

	str->s[str->n++] = c;
}

void vui_string_puts(vui_string* str, const char* s)
{
	if (str == NULL) return;

	if (s == NULL) return;

	for (;*s;s++)
	{
		vui_string_putc(str, *s);
	}
}

void vui_string_putn(vui_string* str, size_t n, const char* s)
{
	if (str == NULL) return;

	if (s == NULL) return;

	for (size_t i = 0; i < n; i++)
	{
		vui_string_putc(str, *s++);
	}
}

void vui_string_append_printf(vui_string* str, const char* fmt, ...)
{
	if (str == NULL) return;

	if (fmt == NULL) return;

	va_list argp;

	va_start(argp, fmt);
	size_t len = vsnprintf(NULL, 0, fmt, argp) + 2;
	va_end(argp);

#if defined(VUI_DEBUG) && defined(VUI_DEBUG_STRING)
	vui_debugf("printf(%s, %s, ...)\n", vui_string_get(str), fmt);
#endif

	size_t n = str->n + len;
	// make room for new stuff and null terminator
	if (str->maxn < n + 1)
	{
		str->maxn = (n + 1)*2;
		vui_string_grow(str, (n + 1)*2);
	}

	va_start(argp, fmt);
	str->n += vsnprintf(&str->s[str->n], len, fmt, argp);
	va_end(argp);
}

void vui_string_append(vui_string* str, const vui_string* str2)
{
	if (str == NULL) return;

	if (str2 == NULL) return;

#if defined(VUI_DEBUG) && defined(VUI_DEBUG_STRING)
	vui_debugf("append(\"%s\", \"%s\")\n", vui_string_get(str), vui_string_get(str));
#endif


	size_t n = str->n + str2->n;
	// make room for str2 and null terminator
	if (str->maxn < n + 1)
	{
		vui_string_grow(str, (n + 1)*2);
	}

	memcpy(&str->s[str->n], str2->s, str2->n);

	str->n = n;
}

void vui_string_putq(vui_string* str, char c)
{
	if (str == NULL) return;

	switch (c)
	{
		case '\n':
			vui_string_puts(str, "\\n");
			break;
		case '\r':
			vui_string_puts(str, "\\r");
			break;
		case '\t':
			vui_string_puts(str, "\\t");
			break;
		case '\0':
			vui_string_puts(str, "\\0");
			break;
		case '\\':
			vui_string_puts(str, "\\\\");
			break;
		case '"':
			vui_string_puts(str, "\\\"");
			break;
		default:
			if (c >= 32 && c < 127)
			{
				vui_string_putc(str, c);
			}
			else
			{
				vui_string_append_printf(str, "\\x%02x", (unsigned char)c);
			}
			break;
	}
}

void vui_string_append_quote(vui_string* str, const vui_string* str2)
{
	if (str == NULL) return;

	if (str2 == NULL) return;

#if defined(VUI_DEBUG) && defined(VUI_DEBUG_STRING)
	vui_debugf("append_quote(\"%s\", \"%s\")\n", vui_string_get(str), vui_string_get(str));
#endif

	for (size_t i = 0; i < str2->n; i++)
	{
		vui_string_putq(str, str2->s[i]);
	}
}

void vui_string_put(vui_string* str, unsigned int c)
{
	if (str == NULL) return;

	char s[16];
	vui_utf8_encode(c, s);

	vui_string_puts(str, s);
}

void vui_string_get_replace(vui_string* str, char** s)
{
	if (str == NULL) return;

	if (s == NULL) return;

	char* s_old = *s;

	*s = vui_string_get(str);

	if (s_old != NULL)
	{
		vui_free(s_old);
	}
}
