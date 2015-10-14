#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <string.h>

#include "vui_utf8.h"
#include "vui.h"

#include "vui_translator.h"

vui_state* vui_translator_deadend;

void vui_translator_init(void)
{
	vui_translator_deadend = vui_state_new();
	vui_translator_deadend->root++;
}

vui_translator* vui_translator_new(void)
{
	vui_translator* tr = malloc(sizeof(vui_translator));

	tr->st_start = vui_state_new_deadend();
	tr->st_start->root++;

	tr->str = NULL;
	tr->stk = vui_stack_new();
	tr->stk->dtor = (void (*)(void*))vui_string_kill;

	return tr;
}

void vui_translator_kill(vui_translator* tr)
{
	tr->st_start->root--;

	free(tr);
}

vui_stack* vui_translator_run(vui_translator* tr, char* in)
{
	vui_stack_reset(tr->stk);

	vui_stack_push(tr->stk, tr->str = vui_string_new(NULL));

	vui_next(vui_run_s(tr->st_start, in, 1), 0, 1);

	return tr->stk;
}


vui_state* vui_translator_tfunc_push(vui_state* currstate, unsigned int c, int act, void* data)
{
	vui_translator* tr = data;

	if (act <= 0) return NULL;

	vui_string_shrink(tr->str); // shrink the previous string

	vui_stack_push(tr->stk, tr->str = vui_string_new(NULL)); // push a new one

	return NULL;
}

vui_state* vui_translator_tfunc_putc(vui_state* currstate, unsigned int c, int act, void* data)
{
	vui_translator* tr = data;

	if (act <= 0) return NULL;

	vui_string_putc(tr->str, c);

	return NULL;
}

vui_state* vui_translator_tfunc_pop(vui_state* currstate, unsigned int c, int act, void* data)
{
	vui_translator* tr = data;

	if (act <= 0) return NULL;

	vui_string_kill(vui_stack_pop(tr->stk));
	tr->str = vui_stack_peek(tr->stk);

	return NULL;
}

vui_state* vui_translator_tfunc_puts(vui_state* currstate, unsigned int c, int act, void* data)
{
	vui_translator* tr = data;

	if (act <= 0) return NULL;

	vui_string* str = vui_stack_pop(tr->stk);
	tr->str = vui_stack_peek(tr->stk);
	vui_string_append(tr->str, str);

	return NULL;
}

static inline vui_transition t_escaper_end(vui_state* lt, vui_translator* tr, vui_state* returnto, unsigned char* action, unsigned char* reaction)
{
	vui_transition lt_mid = vui_transition_translator_putc(tr, NULL);

	vui_stack* lt_end_funcs = vui_stack_new();
	vui_transition_multi_push(lt_end_funcs, vui_transition_translator_pop(tr, NULL));
	vui_transition_multi_push(lt_end_funcs, vui_transition_run_s_s(vui_state_new_t_self(vui_transition_translator_putc(tr, NULL)), reaction));

	vui_set_string_t_mid(lt, action, lt_mid, vui_transition_multi(lt_end_funcs, returnto));
}

vui_frag* vui_frag_accept_escaped(vui_translator* tr)
{
	vui_state* escaper = vui_state_new_putc(tr);

	escaper->name = strdup("escaper");

	escaper->gv_norank = 1;

	vui_stack* exits = vui_stack_new();

	vui_state* exit = vui_state_new();

	vui_transition leave = vui_transition_translator_push(tr, exit);
	vui_set_char_t(escaper, ' ', leave);

	vui_stack_push(exits, exit);

	vui_state* eschelper = vui_state_new_t(vui_transition_translator_putc(tr, escaper));

	vui_stack* lt_abort_funcs = vui_stack_new();
	vui_transition_multi_push(lt_abort_funcs, vui_transition_translator_putc(tr, NULL));
	vui_transition_multi_push(lt_abort_funcs, vui_transition_translator_puts(tr, NULL));

	vui_stack* lt_enter_funcs = vui_stack_new();
	vui_transition_multi_push(lt_enter_funcs, vui_transition_translator_push(tr, NULL));
	vui_transition_multi_push(lt_enter_funcs, vui_transition_translator_putc(tr, NULL));

	vui_state* lt = vui_state_new_t(vui_transition_multi(lt_abort_funcs, escaper));
	vui_set_char_t(escaper, '<', vui_transition_multi(lt_enter_funcs, lt));

	t_escaper_end(lt, tr, escaper, "cr>", "\n");
	t_escaper_end(lt, tr, escaper, "enter>", "\n");
	t_escaper_end(lt, tr, escaper, "tab>", "\t");
	t_escaper_end(lt, tr, escaper, "lt>", "<");
	t_escaper_end(lt, tr, escaper, "left>", vui_utf8_encode_alloc(VUI_KEY_LEFT));
	t_escaper_end(lt, tr, escaper, "l>", vui_utf8_encode_alloc(VUI_KEY_LEFT));
	t_escaper_end(lt, tr, escaper, "right>", vui_utf8_encode_alloc(VUI_KEY_RIGHT));
	t_escaper_end(lt, tr, escaper, "r>", vui_utf8_encode_alloc(VUI_KEY_RIGHT));
	t_escaper_end(lt, tr, escaper, "up>", vui_utf8_encode_alloc(VUI_KEY_UP));
	t_escaper_end(lt, tr, escaper, "u>", vui_utf8_encode_alloc(VUI_KEY_UP));
	t_escaper_end(lt, tr, escaper, "down>", vui_utf8_encode_alloc(VUI_KEY_DOWN));
	t_escaper_end(lt, tr, escaper, "d>", vui_utf8_encode_alloc(VUI_KEY_DOWN));
	t_escaper_end(lt, tr, escaper, "home>", vui_utf8_encode_alloc(VUI_KEY_HOME));
	t_escaper_end(lt, tr, escaper, "end>", vui_utf8_encode_alloc(VUI_KEY_END));
	t_escaper_end(lt, tr, escaper, "backspace>", vui_utf8_encode_alloc(VUI_KEY_BACKSPACE));
	t_escaper_end(lt, tr, escaper, "bs>", vui_utf8_encode_alloc(VUI_KEY_BACKSPACE));
	t_escaper_end(lt, tr, escaper, "delete>", vui_utf8_encode_alloc(VUI_KEY_DELETE));
	t_escaper_end(lt, tr, escaper, "del>", vui_utf8_encode_alloc(VUI_KEY_DELETE));
	t_escaper_end(lt, tr, escaper, "escape>", "\x27");
	t_escaper_end(lt, tr, escaper, "esc>", "\x27");
	t_escaper_end(lt, tr, escaper, "space>", " ");
	t_escaper_end(lt, tr, escaper, "sp>", " ");

	vui_state* bsl = vui_state_new_t(vui_transition_multi(lt_abort_funcs, escaper));
	vui_set_char_t(escaper, '\\', vui_transition_multi(lt_enter_funcs, bsl));

	t_escaper_end(bsl, tr, escaper, "n", "\n");
	t_escaper_end(bsl, tr, escaper, "t", "\t");
	t_escaper_end(bsl, tr, escaper, "\\", "\n");
	t_escaper_end(bsl, tr, escaper, " ", " ");
	t_escaper_end(bsl, tr, escaper, "<", "<");

	return vui_frag_new_stk(escaper, exits);
}

vui_frag* vui_frag_deadend(void)
{
	return vui_frag_new(vui_state_new_deadend(), NULL, 0);
}

vui_frag* vui_frag_accept_any(vui_translator* tr)
{
	return vui_frag_new(vui_state_new_t_self(vui_transition_translator_putc(tr, NULL)), NULL, 0);
}