/*
    This file is part of ydotool.
    Copyright (C) 2018-2022 Reimu NotMoe <reimu@sudomaker.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "ydotool.h"
#include "keymap.h"
#include "keynames.h"
#include "typesafe.h"

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define TS_ENDPOINT_DEFAULT	"https://api.typesafe.ai/v1/systemone"
#define TS_MODEL_DEFAULT	"jev-latest"

#define DO_MAX_REQUEST		1024
#define DO_MAX_ESCAPED		(DO_MAX_REQUEST * 6 + 1)
#define DO_MAX_STATE		(DO_MAX_ESCAPED + 32)
#define DO_MAX_QUESTIONS	32768
#define DO_MAX_BODY		(DO_MAX_STATE + DO_MAX_QUESTIONS + 256)
#define DO_MAX_RESPONSE		65536
#define DO_MAX_TEXT		1024

#define DO_MULTIPLE_LIMIT	0.5
#define DO_SHORTCUT_FALLBACK	0.7

#define SM_CTRL		0x01
#define SM_SHIFT	0x02
#define SM_ALT		0x04
#define SM_SUPER	0x08

static const char *opt_endpoint = TS_ENDPOINT_DEFAULT;
static const char *opt_model = TS_MODEL_DEFAULT;
static const char *opt_layout = "us";
static double opt_min_confidence = 0.5;
static bool opt_dry_run = false;
static bool opt_force = false;
static bool opt_json = false;
static int opt_key_delay_ms = 20;
static int opt_key_hold_ms = 20;

struct do_key {
	const char *name;
	const char *desc;
};

/* Candidate keys offered to the model. Names resolve through keyname_lookup(). */
static const struct do_key do_keys[] = {
	{"KEY_ESC", "Escape/Esc key"},
	{"KEY_TAB", "Tab key"},
	{"KEY_ENTER", "Enter/Return key"},
	{"KEY_SPACE", "Space bar"},
	{"KEY_BACKSPACE", "Backspace key"},
	{"KEY_DELETE", "Delete/Del key"},
	{"KEY_INSERT", "Insert/Ins key"},
	{"KEY_CAPSLOCK", "Caps Lock key"},
	{"KEY_HOME", "Home key"},
	{"KEY_END", "End key"},
	{"KEY_PAGEUP", "Page Up/PgUp key"},
	{"KEY_PAGEDOWN", "Page Down/PgDn key"},
	{"KEY_UP", "Up arrow key"},
	{"KEY_DOWN", "Down arrow key"},
	{"KEY_LEFT", "Left arrow key"},
	{"KEY_RIGHT", "Right arrow key"},
	{"KEY_A", "Letter A"}, {"KEY_B", "Letter B"}, {"KEY_C", "Letter C"},
	{"KEY_D", "Letter D"}, {"KEY_E", "Letter E"}, {"KEY_F", "Letter F"},
	{"KEY_G", "Letter G"}, {"KEY_H", "Letter H"}, {"KEY_I", "Letter I"},
	{"KEY_J", "Letter J"}, {"KEY_K", "Letter K"}, {"KEY_L", "Letter L"},
	{"KEY_M", "Letter M"}, {"KEY_N", "Letter N"}, {"KEY_O", "Letter O"},
	{"KEY_P", "Letter P"}, {"KEY_Q", "Letter Q"}, {"KEY_R", "Letter R"},
	{"KEY_S", "Letter S"}, {"KEY_T", "Letter T"}, {"KEY_U", "Letter U"},
	{"KEY_V", "Letter V"}, {"KEY_W", "Letter W"}, {"KEY_X", "Letter X"},
	{"KEY_Y", "Letter Y"}, {"KEY_Z", "Letter Z"},
	{"KEY_0", "Digit 0"}, {"KEY_1", "Digit 1"}, {"KEY_2", "Digit 2"},
	{"KEY_3", "Digit 3"}, {"KEY_4", "Digit 4"}, {"KEY_5", "Digit 5"},
	{"KEY_6", "Digit 6"}, {"KEY_7", "Digit 7"}, {"KEY_8", "Digit 8"},
	{"KEY_9", "Digit 9"},
	{"KEY_F1", "Function key F1"}, {"KEY_F2", "Function key F2"},
	{"KEY_F3", "Function key F3"}, {"KEY_F4", "Function key F4"},
	{"KEY_F5", "Function key F5"}, {"KEY_F6", "Function key F6"},
	{"KEY_F7", "Function key F7"}, {"KEY_F8", "Function key F8"},
	{"KEY_F9", "Function key F9"}, {"KEY_F10", "Function key F10"},
	{"KEY_F11", "Function key F11"}, {"KEY_F12", "Function key F12"},
	{"KEY_LEFTCTRL", "Control/Ctrl key itself"},
	{"KEY_LEFTSHIFT", "Shift key itself"},
	{"KEY_LEFTALT", "Alt key itself"},
	{"KEY_LEFTMETA", "Windows/Super/Command key itself"},
	{"KEY_MINUS", "Minus/hyphen key"},
	{"KEY_EQUAL", "Equals/plus key"},
	{"KEY_GRAVE", "Grave/backtick key"},
	{"KEY_SEMICOLON", "Semicolon key"},
	{"KEY_APOSTROPHE", "Apostrophe/quote key"},
	{"KEY_COMMA", "Comma key"},
	{"KEY_DOT", "Period key"},
	{"KEY_SLASH", "Slash key"},
	{"KEY_BACKSLASH", "Backslash key"},
	{"KEY_LEFTBRACE", "Left bracket key"},
	{"KEY_RIGHTBRACE", "Right bracket key"},
};

struct shortcut_def {
	const char *name;
	uint16_t key;
	uint8_t mods;
};

static const struct shortcut_def shortcuts[] = {
	{"copy", KEY_C, SM_CTRL},
	{"paste", KEY_V, SM_CTRL},
	{"cut", KEY_X, SM_CTRL},
	{"undo", KEY_Z, SM_CTRL},
	{"redo", KEY_Z, SM_CTRL | SM_SHIFT},
	{"select_all", KEY_A, SM_CTRL},
	{"save", KEY_S, SM_CTRL},
	{"find", KEY_F, SM_CTRL},
	{"new_window", KEY_N, SM_CTRL},
	{"new_tab", KEY_T, SM_CTRL},
	{"close", KEY_W, SM_CTRL},
	{"quit", KEY_Q, SM_CTRL},
};

struct jbuf {
	char *buf;
	size_t cap;
	size_t len;
	int err;
};

static void jb_append(struct jbuf *b, const char *s) {
	size_t n = strlen(s);

	if (b->err)
		return;

	if (b->len + n + 1 > b->cap) {
		b->err = 1;
		return;
	}

	memcpy(b->buf + b->len, s, n);
	b->len += n;
	b->buf[b->len] = '\0';
}

static void jb_printf(struct jbuf *b, const char *fmt, ...) {
	va_list ap;
	int n;

	if (b->err)
		return;

	va_start(ap, fmt);
	n = vsnprintf(b->buf + b->len, b->cap - b->len, fmt, ap);
	va_end(ap);

	if (n < 0 || (size_t)n >= b->cap - b->len) {
		b->err = 1;
		return;
	}

	b->len += (size_t)n;
}

static void show_help() {
	puts(
		"Usage: do [OPTION]... <REQUEST>\n"
		"Interpret a natural-language input request with TypeSafe and perform it.\n"
		"\n"
		"Examples:\n"
		"  ydotool do press ctrl+alt+t\n"
		"  ydotool do \"copy the selected text\"\n"
		"  ydotool do \"scroll down three notches\"\n"
		"  ydotool do --dry-run \"double-click with the left mouse button\"\n"
		"\n"
		"Options:\n"
		"  -n, --dry-run              Print the resolved action and exit without sending events\n"
		"  -j, --json                 Print the raw TypeSafe response\n"
		"  -c, --min-confidence=N     Minimum confidence to act (default: 0.5)\n"
		"  -f, --force                Act even when confidence is below the threshold\n"
		"  -l, --layout=NAME          Keyboard layout used by type actions (default: us)\n"
		"      --model=NAME           TypeSafe model (default: jev-latest)\n"
		"      --endpoint=URL         TypeSafe System One endpoint\n"
		"  -d, --key-delay=N          Delay N milliseconds between keys (default: 20)\n"
		"  -H, --key-hold=N           Hold each key for N milliseconds (default: 20)\n"
		"  -h, --help                 Display this help and exit\n"
		"\n"
		"Requires the TYPESAFE_API_KEY environment variable. The request text is sent\n"
		"to TypeSafe (api.typesafe.ai) for interpretation.\n"
		"Requests describing several sequential actions are refused: ydotool performs\n"
		"one action per invocation, and chaining is the shell's job."
	);
}

static void build_questions(struct jbuf *b) {
	jb_append(b, "{\"action\":{\"type\":\"choice\",\"instructions\":\"What input action is `request` asking for?\",\"criteria\":{");
	jb_append(b, "\"key_combo\":\"Press and release keyboard keys: a shortcut, a single key, or a modifier held while another key is pressed.\",");
	jb_append(b, "\"type_text\":\"Type literal text characters.\",");
	jb_append(b, "\"click\":\"Click a mouse button.\",");
	jb_append(b, "\"scroll\":\"Scroll the mouse wheel.\",");
	jb_append(b, "\"move_mouse\":\"Move the mouse pointer.\",");
	jb_append(b, "\"shortcut\":\"A standard application command such as copy, paste, save, undo, select all, new tab or close tab.\",");
	jb_append(b, "\"unsupported\":\"Not a concrete input action this tool can perform.\"}},");

	jb_append(b, "\"key\":{\"type\":\"choice\",\"instructions\":\"If `request` presses keyboard keys, what is the single main key? If only a modifier key itself is pressed (for example just Ctrl), name that modifier here.\",\"criteria\":{");
	for (size_t i = 0; i < sizeof(do_keys) / sizeof(do_keys[0]); i++)
		jb_printf(b, "\"%s\":\"%s\",", do_keys[i].name, do_keys[i].desc);
	jb_append(b, "\"KEY_NONE\":\"No single main key is named\"}},");

	jb_append(b, "\"ctrl\":{\"type\":\"noul\",\"instructions\":\"Does `request` hold Ctrl down as a modifier while another main key is pressed?\"},");
	jb_append(b, "\"shift\":{\"type\":\"noul\",\"instructions\":\"Does `request` hold Shift down as a modifier while another main key is pressed?\"},");
	jb_append(b, "\"alt\":{\"type\":\"noul\",\"instructions\":\"Does `request` hold Alt down as a modifier while another main key is pressed?\"},");
	jb_append(b, "\"super\":{\"type\":\"noul\",\"instructions\":\"Does `request` hold the Windows/Super/Command key down as a modifier while another main key is pressed?\"},");

	jb_append(b, "\"button\":{\"type\":\"choice\",\"instructions\":\"If `request` clicks a mouse button, which one?\",\"criteria\":{");
	jb_append(b, "\"left\":\"Left mouse button\",\"right\":\"Right mouse button\",\"middle\":\"Middle mouse button\",\"none\":\"No mouse click\"}},");
	jb_append(b, "\"button_stated\":{\"type\":\"noul\",\"instructions\":\"Does `request` explicitly say which mouse button to click?\"},");

	jb_append(b, "\"direction\":{\"type\":\"choice\",\"instructions\":\"If `request` scrolls or moves the pointer, which direction?\",\"criteria\":{");
	jb_append(b, "\"up\":\"Up / away from user\",\"down\":\"Down / toward user\",\"left\":\"Left\",\"right\":\"Right\",\"none\":\"Neither scrolling nor pointer movement\"}},");

	jb_append(b, "\"count\":{\"type\":\"choice\",\"instructions\":\"If `request` clicks or scrolls, how many times or notches?\",\"criteria\":{");
	jb_append(b, "\"1\":\"once / one\",\"2\":\"twice / two / double\",\"3\":\"three\",\"many\":\"more than three, or not stated\"}},");
	jb_append(b, "\"count_stated\":{\"type\":\"noul\",\"instructions\":\"Does `request` explicitly state how many times to click or how many notches to scroll?\"},");

	jb_append(b, "\"shortcut\":{\"type\":\"choice\",\"instructions\":\"Which standard application command does `request` describe, regardless of the keys used to trigger it?\",\"criteria\":{");
	jb_append(b, "\"copy\":\"Copy the selection to the clipboard\",");
	jb_append(b, "\"paste\":\"Paste the clipboard contents\",");
	jb_append(b, "\"cut\":\"Cut the selection to the clipboard\",");
	jb_append(b, "\"undo\":\"Undo the last action\",");
	jb_append(b, "\"redo\":\"Redo the last undone action\",");
	jb_append(b, "\"select_all\":\"Select everything\",");
	jb_append(b, "\"save\":\"Save the current document\",");
	jb_append(b, "\"find\":\"Open find or search\",");
	jb_append(b, "\"new_window\":\"Open a new window or document\",");
	jb_append(b, "\"new_tab\":\"Open a new tab\",");
	jb_append(b, "\"close\":\"Close the current tab or window\",");
	jb_append(b, "\"quit\":\"Quit the application\",");
	jb_append(b, "\"none\":\"No standard application command is meant\"}},");
	jb_append(b, "\"is_shortcut\":{\"type\":\"noul\",\"instructions\":\"Does `request` describe a standard application command like copy, paste, save or undo?\"},");

	jb_append(b, "\"multiple\":{\"type\":\"noul\",\"instructions\":\"Does `request` describe two or more distinct input actions that must happen one after another (for example 'press escape then type yes'), rather than a single action that uses modifiers or repeats the same click?\"}");
	jb_append(b, "}");
}

static const struct shortcut_def *shortcut_lookup(const char *name) {
	for (size_t i = 0; i < sizeof(shortcuts) / sizeof(shortcuts[0]); i++) {
		if (strcmp(shortcuts[i].name, name) == 0)
			return &shortcuts[i];
	}
	return NULL;
}

static int count_value(const char *count, double stated) {
	if (strcmp(count, "1") == 0)
		return 1;
	if (strcmp(count, "2") == 0)
		return 2;
	if (strcmp(count, "3") == 0)
		return 3;
	return stated > DO_MULTIPLE_LIMIT ? 5 : 1;
}

static const char *ci_strstr(const char *hay, const char *needle) {
	size_t nlen = strlen(needle);

	for (; *hay; hay++) {
		size_t i;

		for (i = 0; i < nlen; i++) {
			if (!hay[i] || tolower((unsigned char)hay[i]) != tolower((unsigned char)needle[i]))
				break;
		}

		if (i == nlen)
			return hay;
	}

	return NULL;
}

/* Pull the literal text out of a "type ..." request. */
static int extract_text(const char *req, char *out, size_t out_sz) {
	const char *start = NULL;
	const char *end = NULL;
	size_t len;

	for (const char *p = req; *p; p++) {
		if (*p != '"' && *p != '\'')
			continue;

		const char *close = strchr(p + 1, *p);

		if (close && close > p + 1) {
			start = p + 1;
			end = close;
			break;
		}
	}

	if (!start) {
		static const char *keywords[] = {"type", "say", "write", "enter"};

		for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]) && !start; i++) {
			const char *hit = ci_strstr(req, keywords[i]);

			if (!hit)
				continue;

			start = hit + strlen(keywords[i]);
		}

		if (!start)
			return -1;

		while (*start == ' ' || *start == ':' || *start == ',')
			start++;

		static const char *skip[] = {"the text ", "the string ", "the following ", "the words "};

		for (size_t i = 0; i < sizeof(skip) / sizeof(skip[0]); i++) {
			size_t n = strlen(skip[i]);

			if (strncmp(start, skip[i], n) == 0) {
				start += n;
				break;
			}
		}

		end = start + strlen(start);
		while (end > start && (end[-1] == ' ' || end[-1] == '.' || end[-1] == '"' || end[-1] == '\''))
			end--;
	}

	len = (size_t)(end - start);

	if (len == 0 || len + 1 > out_sz)
		return -1;

	memcpy(out, start, len);
	out[len] = '\0';

	return 0;
}

/* First positive integer in the request, used as the pixel step for moves. */
static int extract_amount(const char *req, int fallback) {
	for (const char *p = req; *p; p++) {
		if (!isdigit((unsigned char)*p))
			continue;

		long v = strtol(p, NULL, 10);

		if (v > 0 && v <= 100000)
			return (int)v;

		while (isdigit((unsigned char)*p))
			p++;
	}

	return fallback;
}

static void emit_combo(uint16_t key, uint8_t mods) {
	if (mods & SM_CTRL)
		uinput_emit(EV_KEY, KEY_LEFTCTRL, 1, 1);
	if (mods & SM_SHIFT)
		uinput_emit(EV_KEY, KEY_LEFTSHIFT, 1, 1);
	if (mods & SM_ALT)
		uinput_emit(EV_KEY, KEY_LEFTALT, 1, 1);
	if (mods & SM_SUPER)
		uinput_emit(EV_KEY, KEY_LEFTMETA, 1, 1);

	uinput_emit(EV_KEY, key, 1, 1);

	usleep(opt_key_hold_ms * 1000);

	uinput_emit(EV_KEY, key, 0, 1);

	if (mods & SM_SUPER)
		uinput_emit(EV_KEY, KEY_LEFTMETA, 0, 1);
	if (mods & SM_ALT)
		uinput_emit(EV_KEY, KEY_LEFTALT, 0, 1);
	if (mods & SM_SHIFT)
		uinput_emit(EV_KEY, KEY_LEFTSHIFT, 0, 1);
	if (mods & SM_CTRL)
		uinput_emit(EV_KEY, KEY_LEFTCTRL, 0, 1);

	usleep(opt_key_delay_ms * 1000);
}

static void emit_click(const char *button, int count) {
	uint16_t code = BTN_LEFT;

	if (strcmp(button, "right") == 0)
		code = BTN_RIGHT;
	else if (strcmp(button, "middle") == 0)
		code = BTN_MIDDLE;

	for (int i = 0; i < count; i++) {
		uinput_emit(EV_KEY, code, 1, 1);
		usleep(opt_key_hold_ms * 1000);
		uinput_emit(EV_KEY, code, 0, 1);
		usleep(opt_key_delay_ms * 1000);
	}
}

static void emit_scroll(const char *direction, int count) {
	uint16_t axis = REL_WHEEL;
	int value = count;

	if (strcmp(direction, "up") == 0)
		value = count;
	else if (strcmp(direction, "down") == 0)
		value = -count;
	else if (strcmp(direction, "left") == 0) {
		axis = REL_HWHEEL;
		value = -count;
	} else {
		axis = REL_HWHEEL;
		value = count;
	}

	uinput_emit(EV_REL, axis, value, 1);
	usleep(opt_key_delay_ms * 1000);
}

static void emit_move(const char *direction, int amount) {
	if (strcmp(direction, "up") == 0)
		uinput_emit(EV_REL, REL_Y, -amount, 1);
	else if (strcmp(direction, "down") == 0)
		uinput_emit(EV_REL, REL_Y, amount, 1);
	else if (strcmp(direction, "left") == 0)
		uinput_emit(EV_REL, REL_X, -amount, 1);
	else
		uinput_emit(EV_REL, REL_X, amount, 1);

	usleep(opt_key_delay_ms * 1000);
}

struct plan {
	const char *action;
	const char *key;
	const char *shortcut;
	const char *button;
	const char *direction;
	const char *count;
	bool ctrl, shift, alt, super;
	double confidence;
};

static void print_plan(const struct plan *p, const char *request) {
	printf("{"
	       "\"request\":\"%s\","
	       "\"action\":\"%s\","
	       "\"key\":\"%s\","
	       "\"shortcut\":\"%s\","
	       "\"modifiers\":{\"ctrl\":%s,\"shift\":%s,\"alt\":%s,\"super\":%s},"
	       "\"button\":\"%s\","
	       "\"direction\":\"%s\","
	       "\"count\":\"%s\","
	       "\"confidence\":%.2f"
	       "}\n",
	       request, p->action, p->key, p->shortcut,
	       p->ctrl ? "true" : "false", p->shift ? "true" : "false",
	       p->alt ? "true" : "false", p->super ? "true" : "false",
	       p->button, p->direction, p->count, p->confidence);
}

int tool_do(int argc, char **argv) {
	char request[DO_MAX_REQUEST] = {0};
	char escaped[DO_MAX_ESCAPED];
	char state[DO_MAX_STATE];
	static char questions[DO_MAX_QUESTIONS];
	static char body[DO_MAX_BODY];
	static char response[DO_MAX_RESPONSE];

	if (argc < 2) {
		show_help();
		return 0;
	}

	while (1) {
		int c;

		static struct option long_options[] = {
			{"dry-run", no_argument, 0, 'n'},
			{"json", no_argument, 0, 'j'},
			{"force", no_argument, 0, 'f'},
			{"min-confidence", required_argument, 0, 'c'},
			{"layout", required_argument, 0, 'l'},
			{"model", required_argument, 0, 1},
			{"endpoint", required_argument, 0, 2},
			{"key-delay", required_argument, 0, 'd'},
			{"key-hold", required_argument, 0, 'H'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "njfc:l:d:H:h", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'n':
				opt_dry_run = true;
				break;
			case 'j':
				opt_json = true;
				break;
			case 'f':
				opt_force = true;
				break;
			case 'c':
				opt_min_confidence = strtod(optarg, NULL);
				break;
			case 'l':
				opt_layout = optarg;
				break;
			case 'd':
				opt_key_delay_ms = strtol(optarg, NULL, 10);
				break;
			case 'H':
				opt_key_hold_ms = strtol(optarg, NULL, 10);
				break;
			case 1:
				opt_model = optarg;
				break;
			case 2:
				opt_endpoint = optarg;
				break;
			case 'h':
				show_help();
				exit(0);
			case '?':
				return 2;
			default:
				abort();
		}
	}

	if (optind >= argc) {
		show_help();
		return 2;
	}

	size_t req_len = 0;
	for (int i = optind; i < argc; i++) {
		size_t n = strlen(argv[i]);

		if (req_len + n + 2 > sizeof(request)) {
			fprintf(stderr, "ydotool: do: error: request is too long\n");
			return 2;
		}

		if (req_len)
			request[req_len++] = ' ';
		memcpy(request + req_len, argv[i], n);
		req_len += n;
		request[req_len] = '\0';
	}

	if (strlen(opt_model) > 64 || strchr(opt_model, '"') || strchr(opt_model, '\\')) {
		fprintf(stderr, "ydotool: do: error: invalid model name\n");
		return 2;
	}

	const struct keymap *km = keymap_get(opt_layout);
	if (!km) {
		fprintf(stderr, "ydotool: do: error: unknown layout '%s'. Available layouts: ", opt_layout);
		keymap_print_list(stderr);
		fprintf(stderr, "\n");
		return 2;
	}

	if (typesafe_json_escape(request, escaped, sizeof(escaped)) < 0) {
		fprintf(stderr, "ydotool: do: error: request is too long\n");
		return 2;
	}

	snprintf(state, sizeof(state), "{\"request\":\"%s\"}", escaped);

	struct jbuf qb = { questions, sizeof(questions), 0, 0 };
	build_questions(&qb);
	if (qb.err) {
		fprintf(stderr, "ydotool: do: internal error: question set too large\n");
		return 2;
	}

	snprintf(body, sizeof(body), "{\"state\":%s,\"model\":\"%s\",\"questions\":%s}",
		 state, opt_model, questions);

	if (typesafe_ask(opt_endpoint, body, response, sizeof(response)) < 0)
		return 2;

	if (opt_json) {
		puts(response);
		if (opt_dry_run)
			return 0;
	}

	char action[32] = {0};
	char key[64] = {0};
	char button[32] = {0};
	char direction[32] = {0};
	char count[32] = {0};
	char shortcut[32] = {0};
	double action_conf = -1, key_conf = -1, button_conf = -1;
	double direction_conf = -1, count_conf = -1, shortcut_conf = -1;
	double ctrl = -1, shift = -1, alt = -1, super = -1;
	double button_stated = -1, count_stated = -1;
	double is_shortcut = -1, multiple = -1;

	if (typesafe_choice(response, "action", action, sizeof(action), &action_conf) < 0) {
		fprintf(stderr, "ydotool: do: error: malformed TypeSafe response (no action)\n");
		return 2;
	}

	typesafe_choice(response, "key", key, sizeof(key), &key_conf);
	typesafe_choice(response, "button", button, sizeof(button), &button_conf);
	typesafe_choice(response, "direction", direction, sizeof(direction), &direction_conf);
	typesafe_choice(response, "count", count, sizeof(count), &count_conf);
	typesafe_choice(response, "shortcut", shortcut, sizeof(shortcut), &shortcut_conf);
	typesafe_noul(response, "ctrl", &ctrl);
	typesafe_noul(response, "shift", &shift);
	typesafe_noul(response, "alt", &alt);
	typesafe_noul(response, "super", &super);
	typesafe_noul(response, "button_stated", &button_stated);
	typesafe_noul(response, "count_stated", &count_stated);
	typesafe_noul(response, "is_shortcut", &is_shortcut);
	typesafe_noul(response, "multiple", &multiple);

	if (multiple > DO_MULTIPLE_LIMIT) {
		fprintf(stderr, "ydotool: do: error: the request describes several sequential actions\n"
				"ydotool performs one action per invocation; chain commands in your shell instead\n");
		return 2;
	}

	if (strcmp(action, "unsupported") == 0) {
		fprintf(stderr, "ydotool: do: error: no concrete input action recognized in \"%s\"\n", request);
		return 2;
	}

	struct plan p = {
		.action = action,
		.key = key,
		.shortcut = shortcut,
		.button = button,
		.direction = direction,
		.count = count,
		.ctrl = ctrl > 0.5,
		.shift = shift > 0.5,
		.alt = alt > 0.5,
		.super = super > 0.5,
		.confidence = action_conf,
	};

	/* A shortcut may arrive as the action, or as a fallback when no main key was named. */
	const struct shortcut_def *sc = NULL;

	if (strcmp(action, "shortcut") == 0 ||
	    (is_shortcut > DO_SHORTCUT_FALLBACK && strcmp(shortcut, "none") != 0 &&
	     strcmp(key, "KEY_NONE") == 0)) {
		sc = shortcut_lookup(shortcut);

		if (!sc) {
			fprintf(stderr, "ydotool: do: error: unknown shortcut '%s'\n", shortcut);
			return 2;
		}

		p.action = "shortcut";
		p.key = "KEY_NONE";
		p.ctrl = (sc->mods & SM_CTRL) != 0;
		p.shift = (sc->mods & SM_SHIFT) != 0;
		p.alt = (sc->mods & SM_ALT) != 0;
		p.super = (sc->mods & SM_SUPER) != 0;

		if (shortcut_conf >= 0 && shortcut_conf < p.confidence)
			p.confidence = shortcut_conf;
	} else if (strcmp(action, "key_combo") == 0) {
		if (strcmp(key, "KEY_NONE") == 0) {
			fprintf(stderr, "ydotool: do: error: no key identified in \"%s\"\n", request);
			return 2;
		}

		if (key_conf >= 0 && key_conf < p.confidence)
			p.confidence = key_conf;
	} else if (strcmp(action, "click") == 0) {
		if (strcmp(button, "none") == 0)
			p.button = "left";

		if (button_conf >= 0 && button_conf < p.confidence)
			p.confidence = button_conf;
	} else if (strcmp(action, "scroll") == 0 || strcmp(action, "move_mouse") == 0) {
		if (strcmp(direction, "none") == 0) {
			fprintf(stderr, "ydotool: do: error: no direction identified in \"%s\"\n", request);
			return 2;
		}

		if (direction_conf >= 0 && direction_conf < p.confidence)
			p.confidence = direction_conf;

		if (strcmp(action, "scroll") == 0 && count_conf >= 0 && count_conf < p.confidence)
			p.confidence = count_conf;
	}

	/*
	 * Noul answers near 0.5 are uncertain. Modifier answers only matter for a
	 * key combo whose main key is not itself a modifier; elsewhere they are
	 * speculative questions the model answers loosely.
	 */
	bool key_is_modifier = strcmp(key, "KEY_LEFTCTRL") == 0 ||
			       strcmp(key, "KEY_RIGHTCTRL") == 0 ||
			       strcmp(key, "KEY_LEFTSHIFT") == 0 ||
			       strcmp(key, "KEY_RIGHTSHIFT") == 0 ||
			       strcmp(key, "KEY_LEFTALT") == 0 ||
			       strcmp(key, "KEY_RIGHTALT") == 0 ||
			       strcmp(key, "KEY_LEFTMETA") == 0 ||
			       strcmp(key, "KEY_RIGHTMETA") == 0;

	if (strcmp(p.action, "key_combo") == 0 && !key_is_modifier) {
		double modifiers[] = {ctrl, shift, alt, super};

		for (size_t i = 0; i < sizeof(modifiers) / sizeof(modifiers[0]); i++) {
			double certainty = modifiers[i] > 0.5 ? modifiers[i] - 0.5 : 0.5 - modifiers[i];
			certainty *= 2.0;

			if (certainty < p.confidence)
				p.confidence = certainty;
		}
	}

	if (!opt_force && p.confidence < opt_min_confidence) {
		print_plan(&p, escaped);
		fprintf(stderr, "ydotool: do: error: confidence %.2f is below the threshold %.2f; "
				"use --force to act anyway or rephrase\n", p.confidence, opt_min_confidence);
		return 2;
	}

	if (opt_dry_run) {
		print_plan(&p, escaped);
		return 0;
	}

	if (ydotool_connect() < 0)
		return 2;

	if (sc) {
		uint8_t mods = sc->mods;
		emit_combo(sc->key, mods);
	} else if (strcmp(action, "key_combo") == 0) {
		int kc = keyname_lookup(key);

		if (kc < 0) {
			fprintf(stderr, "ydotool: do: error: unknown key '%s'\n", key);
			return 2;
		}

		uint8_t mods = 0;

		if (p.ctrl) mods |= SM_CTRL;
		if (p.shift) mods |= SM_SHIFT;
		if (p.alt) mods |= SM_ALT;
		if (p.super) mods |= SM_SUPER;

		emit_combo((uint16_t)kc, mods);
	} else if (strcmp(action, "type_text") == 0) {
		char text[DO_MAX_TEXT];

		if (extract_text(request, text, sizeof(text)) < 0) {
			fprintf(stderr, "ydotool: do: error: could not find literal text to type in \"%s\"\n", request);
			return 2;
		}

		if (type_string(text, false, km, opt_key_delay_ms, opt_key_hold_ms) < 0)
			return 2;
	} else if (strcmp(action, "click") == 0) {
		int n = count_value(count, count_stated);
		emit_click(p.button, n);
	} else if (strcmp(action, "scroll") == 0) {
		int n = count_value(count, count_stated);
		emit_scroll(direction, n);
	} else if (strcmp(action, "move_mouse") == 0) {
		emit_move(direction, extract_amount(request, 50));
	} else {
		fprintf(stderr, "ydotool: do: error: unsupported action '%s'\n", action);
		return 2;
	}

	return 0;
}
