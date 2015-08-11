#include <stdlib.h>
#include <string.h>

#include "vui.h"


char* vui_bar;
int vui_crsrx;
int cols;

char* vui_cmd;
char* vui_status;

vui_state* vui_normal_mode;

int cmd_base;
int cmd_len;

static hist_entry* hist_entry_new(vui_cmdline_def* cmdline)
{
	hist_entry* entry = malloc(sizeof(hist_entry));

	entry->prev = cmdline->hist_last_entry;

	if (cmdline->hist_last_entry != NULL)
	{
		cmdline->hist_last_entry->next = entry;
	}

	cmdline->hist_curr_entry = cmdline->hist_last_entry = entry;

	entry->next = NULL;

	entry->len = 1;
	entry->maxlen = cols*2;
	entry->line = malloc(entry->maxlen);

	entry->line[0] = 0;

	return entry;
}

static void hist_entry_kill(hist_entry* entry)
{
	// make prev and next point to each other instead of entry
	hist_entry* oldprev = entry->prev;
	hist_entry* oldnext = entry->next;

	if (oldnext != NULL)
	{
		oldnext->prev = oldprev;
	}

	if (oldprev != NULL)
	{
		oldprev->next = oldnext;
	}

	// free entry's line
	free(entry->line);

	// free entry
	free(entry);
}

static void hist_entry_set(hist_entry* entry, char* str, int len)
{
	entry->len = len + 1;
	if (entry->maxlen < entry->len)
	{
		entry->maxlen = entry->len*2;
		entry->line = realloc(entry->line, entry->maxlen);
	}

	memcpy(entry->line, str, entry->len);

	entry->line[len] = 0;
}

static void hist_entry_shrink(hist_entry* entry)
{
	entry->maxlen = entry->len;

	entry->line = realloc(entry->line, entry->maxlen);
}

static void hist_last_entry_edit(vui_cmdline_def* cmdline)
{
	cmdline->cmd_modified = 1;

	if (cmdline->hist_curr_entry == cmdline->hist_last_entry)
	{
		return;
	}

	hist_entry_set(cmdline->hist_last_entry, cmdline->hist_curr_entry->line, cmdline->hist_curr_entry->len);

	cmdline->hist_curr_entry = cmdline->hist_last_entry;
}

static void hist_last_entry_clear(vui_cmdline_def* cmdline)
{
	cmdline->cmd_modified = 0;

	cmdline->hist_last_entry->len = 1;
	cmdline->hist_last_entry->line[0] = 0;
}


static vui_state* tfunc_normal(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	return NULL;
}

static vui_state* tfunc_normal_to_cmd(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	vui_bar = vui_cmd;
	vui_crsrx = 0;

	char* label = cmdline->label;
	while (*label)
	{
		vui_cmd[vui_crsrx++] = *label++;
	}

	cmd_base = vui_crsrx;

	cmd_len = cmd_base - 1;

	cmdline->cmd_modified = 0;

	cmdline->hist_curr_entry = cmdline->hist_last_entry;

	memset(&vui_cmd[cmd_base], ' ', cols - cmd_base);

	return NULL;
}

static vui_state* tfunc_cmd_key(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	hist_last_entry_edit(cmdline);

	if (vui_crsrx != cmd_len)
	{
		memmove(&vui_cmd[vui_crsrx + 1], &vui_cmd[vui_crsrx], cmd_len - vui_crsrx + 1);
	}
	vui_cmd[vui_crsrx++] = c;
	cmd_len++;

	return NULL;
}

static vui_state* tfunc_cmd_backspace(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return (vui_crsrx <= cmd_base) ? NULL : vui_normal_mode;

	if (cmd_len <= 0)
	{
		vui_bar = vui_status;
		vui_crsrx = -1;
		return vui_normal_mode;
	}

	if (vui_crsrx > cmd_base)
	{
		hist_last_entry_edit(cmdline);

		vui_cmd[cmd_len+1] = ' ';
		vui_crsrx--;
		memmove(&vui_cmd[vui_crsrx], &vui_cmd[vui_crsrx+1], cmd_len-vui_crsrx+1);
		cmd_len--;

		if (!cmd_len)
		{
			hist_last_entry_clear(cmdline);
		}
	}

	return NULL;
}

static vui_state* tfunc_cmd_delete(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	if (vui_crsrx <= cmd_len)
	{
		hist_last_entry_edit(cmdline);

		vui_cmd[cmd_len+1] = ' ';
		memmove(&vui_cmd[vui_crsrx], &vui_cmd[vui_crsrx+1], cmd_len-vui_crsrx+1);
		cmd_len--;

		if (!cmd_len)
		{
			hist_last_entry_clear(cmdline);
		}
	}

	return NULL;
}

static vui_state* tfunc_cmd_left(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	if (vui_crsrx > cmd_base)
	{
		vui_crsrx--;
	}

	return NULL;
}

static vui_state* tfunc_cmd_right(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	if (vui_crsrx <= cmd_len)
	{
		vui_crsrx++;
	}

	return NULL;
}

static vui_state* tfunc_cmd_up(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	if (cmdline->hist_curr_entry->prev != NULL && !cmdline->cmd_modified)
	{
#ifdef VUI_DEBUG
		if (cmdline->hist_curr_entry->prev == cmdline->hist_curr_entry)
		{
			vui_debug("own prev!\n");
		}
#endif

		cmdline->hist_curr_entry = cmdline->hist_curr_entry->prev;

		memcpy(&vui_cmd[cmd_base], cmdline->hist_curr_entry->line, cmdline->hist_curr_entry->len);
		vui_crsrx = cmd_base + cmdline->hist_curr_entry->len - 1;
		cmd_len = vui_crsrx - 1;
		memset(&vui_cmd[vui_crsrx], ' ', cols - vui_crsrx);
	}
#ifdef VUI_DEBUG
	else
	{
		vui_debug("no prev\n");
	}
#endif

	return NULL;
}

static vui_state* tfunc_cmd_down(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	if (cmdline->hist_curr_entry->next != NULL && !cmdline->cmd_modified)
	{
#ifdef VUI_DEBUG
		if (cmdline->hist_curr_entry->next == cmdline->hist_curr_entry)
		{
			vui_debug("own next!\n");
		}
#endif

		cmdline->hist_curr_entry = cmdline->hist_curr_entry->next;

		memcpy(&vui_cmd[cmd_base], cmdline->hist_curr_entry->line, cmdline->hist_curr_entry->len);
		vui_crsrx = cmd_base + cmdline->hist_curr_entry->len - 1;
		cmd_len = vui_crsrx - 1;
		cmd_len = vui_crsrx - 1;
		memset(&vui_cmd[vui_crsrx], ' ', cols - vui_crsrx);
	}
#ifdef VUI_DEBUG
	else
	{
		vui_debug("no next\n");
	}
#endif

	return NULL;
}

static vui_state* tfunc_cmd_home(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	vui_crsrx = cmd_base;

	return NULL;
}

static vui_state* tfunc_cmd_end(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	vui_crsrx = cmd_len + 1;

	return NULL;
}

static vui_state* tfunc_cmd_escape(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	if (cmdline->hist_curr_entry != cmdline->hist_last_entry && !cmdline->cmd_modified)
	{
		hist_entry_kill(cmdline->hist_curr_entry);
		cmdline->hist_curr_entry = cmdline->hist_last_entry;
	}

	hist_entry_set(cmdline->hist_curr_entry, &vui_cmd[cmd_base], cmd_len);

	vui_bar = vui_status;
	vui_crsrx = -1;

	hist_entry_shrink(cmdline->hist_curr_entry);

	if (cmdline->hist_curr_entry->line[0] != 0)
	{
#ifdef VUI_DEBUG
		vui_debug("new entry\n");
#endif

		hist_entry_new(cmdline);
	}
#ifdef VUI_DEBUG
	else
	{
		vui_debug("no new entry\n");
	}
#endif

	return NULL;
}

static vui_state* tfunc_cmd_enter(vui_state* currstate, int c, int act, void* data)
{
	vui_cmdline_def* cmdline = data;

	if (!act) return NULL;

	if (cmdline->hist_curr_entry != cmdline->hist_last_entry && !cmdline->cmd_modified)
	{
		hist_entry_kill(cmdline->hist_curr_entry);
		cmdline->hist_curr_entry = cmdline->hist_last_entry;
	}

	hist_entry_set(cmdline->hist_curr_entry, &vui_cmd[cmd_base], cmd_len);

	cmdline->on_submit(cmdline->hist_curr_entry->line);

	vui_bar = vui_status;
	vui_crsrx = -1;

	hist_entry_shrink(cmdline->hist_curr_entry);

	if (cmdline->hist_curr_entry->line[0] != 0)
	{
#ifdef VUI_DEBUG
		vui_debug("new entry\n");
#endif

		hist_entry_new(cmdline);
	}
#ifdef VUI_DEBUG
	else
	{
		vui_debug("no new entry\n");
	}
#endif

	return NULL;
}


void vui_init(int width)
{
	vui_normal_mode = vui_curr_state = vui_state_new(NULL);

	vui_transition transition_normal = {.next = vui_normal_mode, .func = tfunc_normal};

	for (int i=0; i<MAXINPUT; i++)
	{
		vui_set_char_t(vui_normal_mode, i, transition_normal);
	}

	cols = width;

	vui_cmd = malloc((cols+1)*sizeof(char));
	vui_status = malloc((cols+1)*sizeof(char));

	memset(vui_cmd, ' ', cols);
	memset(vui_status, ' ', cols);

	vui_cmd[cols] = 0;
	vui_status[cols] = 0;

	vui_crsrx = -1;

	vui_bar = vui_status;
}

void vui_resize(int width)
{
	// if the number of columns actually changed
	if (width != cols)
	{
		vui_cmd = realloc(vui_cmd, (width+1)*sizeof(char));
		vui_status = realloc(vui_status, (width+1)*sizeof(char));

		int minw, maxw;
		if (width > cols)
		{
			minw = cols;
			maxw = width;
		}
		else
		{
			minw = width;
			maxw = cols;
		}

		memset(&vui_cmd[minw], ' ', maxw-minw);
		memset(&vui_status[minw], ' ', maxw-minw);

		vui_cmd[cols] = 0;
		vui_status[cols] = 0;
		cols = width;
	}

}

vui_state* vui_mode_new(char* cmd, char* name, char* label, int mode, vui_transition func_enter, vui_transition func_in, vui_transition func_exit)
{
	vui_state* state = vui_state_new(NULL);

	if (func_enter.next == NULL) func_enter.next = state;
	if (mode != VUI_NEW_MODE_IN_MANUAL)
	{
		if (func_in.next == NULL) func_in.next = state;
	}
	if (func_exit.next == NULL) func_exit.next = vui_normal_mode;

	for (int i=0; i<MAXINPUT; i++)
	{
		vui_set_char_t(state, i, func_in);
	}

	vui_set_string_t(vui_normal_mode, cmd, func_enter);
	vui_set_char_t(state, VUI_KEY_ESCAPE, func_exit);

	return state;
}

vui_cmdline_def* vui_cmdline_mode_new(char* cmd, char* name, char* label, vui_cmdline_submit_callback on_submit)
{
	vui_cmdline_def* cmdline = malloc(sizeof(vui_cmdline_def));
	cmdline->on_submit = on_submit;
	cmdline->label = label;

	vui_transition transition_normal_to_cmd = {.next = NULL, .func = tfunc_normal_to_cmd, .data = cmdline};
	vui_transition transition_cmd = {.next = NULL, .func = NULL, .data = cmdline};
	vui_transition transition_cmd_escape = {.next = vui_normal_mode, .func = tfunc_cmd_escape, .data = cmdline};

	vui_state* cmdline_state = vui_mode_new(cmd, name, "", VUI_NEW_MODE_IN_MANUAL, transition_normal_to_cmd, transition_cmd, transition_cmd_escape);

	vui_transition transition_cmd_up = {.next = cmdline_state, .func = tfunc_cmd_up, .data = cmdline};
	vui_transition transition_cmd_down = {.next = cmdline_state, .func = tfunc_cmd_down, .data = cmdline};
	vui_transition transition_cmd_left = {.next = cmdline_state, .func = tfunc_cmd_left, .data = cmdline};
	vui_transition transition_cmd_right = {.next = cmdline_state, .func = tfunc_cmd_right, .data = cmdline};
	vui_transition transition_cmd_enter = {.next = vui_normal_mode, .func = tfunc_cmd_enter, .data = cmdline};
	vui_transition transition_cmd_backspace = {.next = NULL, .func = tfunc_cmd_backspace, .data = cmdline};
	vui_transition transition_cmd_delete = {.next = cmdline_state, .func = tfunc_cmd_delete, .data = cmdline};
	vui_transition transition_cmd_home = {.next = cmdline_state, .func = tfunc_cmd_home, .data = cmdline};
	vui_transition transition_cmd_end = {.next = cmdline_state, .func = tfunc_cmd_end, .data = cmdline};
	vui_transition transition_cmd_key = {.next = cmdline_state, .func = tfunc_cmd_key, .data = cmdline};

	vui_set_char_t(cmdline_state, VUI_KEY_UP, transition_cmd_up);
	vui_set_char_t(cmdline_state, VUI_KEY_DOWN, transition_cmd_down);
	vui_set_char_t(cmdline_state, VUI_KEY_LEFT, transition_cmd_left);
	vui_set_char_t(cmdline_state, VUI_KEY_RIGHT, transition_cmd_right);
	vui_set_char_t(cmdline_state, VUI_KEY_ENTER, transition_cmd_enter);
	vui_set_char_t(cmdline_state, VUI_KEY_BACKSPACE, transition_cmd_backspace);
	vui_set_char_t(cmdline_state, VUI_KEY_DELETE, transition_cmd_delete);
	//vui_set_char_t(cmdline_state, VUI_KEY_ESCAPE, transition_cmd_escape);
	vui_set_char_t(cmdline_state, VUI_KEY_HOME, transition_cmd_home);
	vui_set_char_t(cmdline_state, VUI_KEY_END, transition_cmd_end);
	vui_set_range_t(cmdline_state, 32, 127, transition_cmd_key);

	cmdline->cmdline_state = cmdline_state;

	hist_entry_new(cmdline);

	return cmdline;
}
