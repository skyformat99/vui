#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>

#include <curses.h>

#include "vui_debug.h"

#include "vui.h"
#include "vui_combinator.h"
#include "vui_translator.h"
#include "vui_gc.h"

#include "vui_graphviz.h"


WINDOW* statusline;

int VUI_KEY_UP = KEY_UP;
int VUI_KEY_DOWN = KEY_DOWN;
int VUI_KEY_LEFT = KEY_LEFT;
int VUI_KEY_RIGHT = KEY_RIGHT;
int VUI_KEY_ENTER = '\n';
int VUI_KEY_BACKSPACE = 127;
int VUI_KEY_DELETE = KEY_DC;
int VUI_KEY_ESCAPE = 27;
int VUI_KEY_HOME = KEY_HOME;
int VUI_KEY_END = KEY_END;
int VUI_KEY_MODIFIER_CONTROL = -'@';

FILE* dbgf;

vui_cmdline_def* cmd_mode;
vui_cmdline_def* search_mode;

#if defined(VUI_DEBUG)
void vui_debug(char* s)
{
	fwrite(s, 1, strlen(s), dbgf);
}

void vui_debugf(const char* format, ...)
{
	va_list argp;
	va_start(argp, format);
	vfprintf(dbgf, format, argp);
	va_end(argp);
}
#endif

static void sighandler(int signo)
{
        switch (signo)
        {
                case SIGWINCH:
                {
			fprintf(dbgf, "winch\n");

			endwin();
			refresh();
			clear();

			wresize(statusline, 1, COLS);

			mvwin(statusline, LINES-1, 0);

			vui_resize(COLS);

			vui_showcmd_setup(COLS - 20, 10);

			mvwaddstr(statusline, 0, 0, vui_bar);
			wmove(statusline, 0, vui_crsrx);
			curs_set(vui_crsrx >= 0);
			wrefresh(statusline);

                        break;
                }

                default:
                {

                }
        }
}

static vui_state* tfunc_quit(vui_state* currstate, unsigned int c, int act, void* data)
{
	if (act <= 0) return vui_return(act);

	if (vui_count != 0)
	{
		fprintf(dbgf, "count: %d\n", vui_count);

		vui_reset();

		return vui_return(act);
	}

	fprintf(dbgf, "quit\n");
	endwin();
	printf("Quitting!\n");

	exit(0);
}

static vui_state* tfunc_winch(vui_state* currstate, unsigned int c, int act, void* data)
{
	if (act <= 0) return NULL;

	fprintf(dbgf, "winch\n");

	wresize(statusline, 1, COLS);

	mvwin(statusline, LINES-1, 0);

	vui_resize(COLS);

	vui_showcmd_setup(COLS - 20, 10);

	return NULL;
}

void on_cmd_submit(vui_stack* cmd)
{
#define arg(i) vui_string_get(vui_stack_index(cmd, i))

	char* op = arg(0);

	if (op != NULL)
	{
		fprintf(dbgf, "op: \"%s\"\n", op);
	}
	else
	{
		fprintf(dbgf, "op: NULL");
		return;
	}

	if (!strcmp(op, "q"))
	{
		endwin();
		printf("Quitting!\n");
		exit(0);
	}
	else if (!strcmp(op, "map"))
	{
		char* action = arg(1);
		if (action == NULL)
		{
			fprintf(dbgf, "action: NULL\n");
			return;
		}
		else
		{
			fprintf(dbgf, "action: \"%s\"\n", action);
		}
		char* reaction = arg(2);
		if (reaction == NULL)
		{
			fprintf(dbgf, "reaction: NULL\n");
			return;
		}
		else
		{
			fprintf(dbgf, "reaction: \"%s\"\n", reaction);
		}

		vui_map(vui_normal_mode, action, reaction);
	}
#undef arg
}

void on_search_submit(char* cmd)
{
	fprintf(dbgf, "search: \"%s\"\n", cmd);
}


int main(int argc, char** argv)
{
	printf("Opening log...\n");
	dbgf = fopen("log", "a");

	fprintf(dbgf, "start\n");

	initscr();
	cbreak();
	noecho();

	endwin();

	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sighandler;
	sigaction(SIGWINCH, &sa, NULL);


	statusline = newwin(1, COLS, LINES-1, 0);

	keypad(statusline, TRUE);
	meta(statusline, TRUE);


	vui_init(COLS);

	vui_set_char_t_u(vui_normal_mode, KEY_RESIZE, vui_transition_new2(tfunc_winch, NULL));

	vui_count_init();

	vui_register_init();

	vui_macro_init('q', '@');

	vui_showcmd_setup(COLS - 20, 10);

	vui_translator_init();


	vui_translator* cmd_tr = vui_translator_new();

	vui_state* cmd_tr_start = cmd_tr->st_start;
	cmd_tr_start->name = "cmd_tr";


	vui_state* cmd_tr_q = vui_state_new_deadend();

	vui_set_string_t_mid(cmd_tr_start, "q", vui_transition_translator_putc(cmd_tr, NULL), vui_transition_translator_putc(cmd_tr, cmd_tr_q));


	vui_state* cmd_tr_map = vui_frag_release(
	                                         vui_frag_cat(vui_frag_accept_escaped(cmd_tr), vui_frag_accept_escaped(cmd_tr)),
						 vui_state_new_deadend());

	vui_set_string_t_mid(cmd_tr_start, "map ", vui_transition_translator_putc(cmd_tr, NULL), vui_transition_translator_push(cmd_tr, cmd_tr_map));


	cmd_tr_start->root++;


	cmd_mode = vui_cmdline_mode_new(":", "command", ":", cmd_tr, on_cmd_submit);

	search_mode = vui_cmdline_mode_new("/", "search", "/", NULL, on_search_submit);


	vui_transition transition_quit = vui_transition_new2(tfunc_quit, NULL);

	vui_set_char_t_u(vui_normal_mode, 'Q', transition_quit);

	vui_set_string_t_nocall(vui_normal_mode, "ZZ", transition_quit);


	vui_gc_run();

	FILE* f = fopen("vui.dot", "w");
	vui_stack* gv_roots = vui_stack_new();
	vui_stack_push(gv_roots, vui_normal_mode);
	vui_stack_push(gv_roots, cmd_tr_start);
	vui_gv_write(f, gv_roots);
	fclose(f);

	while (1)
	{
		int c = wgetch(statusline);

		char c2[3] = {"??"};
		if (c >= 32 && c < 127)
		{
			c2[0] = c;
			c2[1] = 0;
		}
		else if (c >= 0 && c < 32)
		{
			c2[0] = '^';
			c2[1] = c + '@';
		}
		else
		{
			c2[0] = '?';
			c2[1] = '?';
		}

		fprintf(dbgf, "char %d: %s\n", c, c2);

		if (c >= 0)
		{
			endwin();

			vui_input(c);
			mvwaddstr(statusline, 0, 0, vui_bar);
			wmove(statusline, 0, vui_crsrx);
			curs_set(vui_crsrx >= 0);
			wrefresh(statusline);

#if 0
			if (vui_curr_state == vui_normal_mode)
			{
				fprintf(dbgf, "normal mode\n");
			}
			else if (vui_curr_state == vui_count_mode)
			{
				fprintf(dbgf, "count mode\n");
			}
			else if (vui_curr_state == cmd_mode->cmdline_state)
			{
				fprintf(dbgf, "cmd mode\n");
			}
			else if (vui_curr_state == search_mode->cmdline_state)
			{
				fprintf(dbgf, "search mode\n");
			}
#else
			if (vui_curr_state->name != NULL)
			{
				fprintf(dbgf, "%s\n", vui_curr_state->name);
			}
#endif
			else
			{
				fprintf(dbgf, "unknown mode\n");
			}
		}
	}

	endwin();
}
