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
#include "mcp_json.h"
#include "perception.h"

#include <gio/gio.h>

#include <stdarg.h>
#include <string.h>

#ifndef VERSION
#define VERSION "unknown"
#endif

#define MCP_MAX_TOKENS		512
#define MCP_MAX_LINE		(1024 * 1024)
#define MCP_MAX_WINDOWS		64

static const char *opt_layout = "us";
static int opt_key_delay_ms = 20;
static int opt_key_hold_ms = 20;
static bool opt_read_only = false;
static bool opt_dry_run = false;
static bool opt_require_focus = false;
static bool opt_allow_dangerous = false;
static int opt_rate_limit = 20;
static bool opt_log_args = false;
static int opt_screenshot_timeout_s = 60;
static const struct perception_ops *perception = &perception_gnome;

static GMainLoop *main_loop;

/* ---------------------------------------------------------------- helpers */

static void json_escape_append(GString *s, const char *in) {
	for (const unsigned char *p = (const unsigned char *)in; *p; p++) {
		switch (*p) {
		case '"':  g_string_append(s, "\\\""); break;
		case '\\': g_string_append(s, "\\\\"); break;
		case '\n': g_string_append(s, "\\n"); break;
		case '\r': g_string_append(s, "\\r"); break;
		case '\t': g_string_append(s, "\\t"); break;
		default:
			if (*p < 0x20)
				g_string_append_printf(s, "\\u%04x", *p);
			else
				g_string_append_c(s, (char)*p);
		}
	}
}

static const char *ci_strstr(const char *hay, const char *needle) {
	size_t nlen = strlen(needle);

	for (; *hay; hay++) {
		size_t i;

		for (i = 0; i < nlen; i++) {
			if (!hay[i] ||
			    g_ascii_tolower(hay[i]) != g_ascii_tolower(needle[i]))
				break;
		}

		if (i == nlen)
			return hay;
	}

	return NULL;
}

static void content_text(GString *content, const char *text) {
	if (content->len)
		g_string_append_c(content, ',');

	g_string_append(content, "{\"type\":\"text\",\"text\":\"");
	json_escape_append(content, text);
	g_string_append(content, "\"}");
}

static void content_image(GString *content, const char *b64, const char *mime) {
	if (content->len)
		g_string_append_c(content, ',');

	g_string_append_printf(content,
		"{\"type\":\"image\",\"data\":\"%s\",\"mimeType\":\"%s\"}", b64, mime);
}

static void send_line(const char *json) {
	fputs(json, stdout);
	fputc('\n', stdout);
	fflush(stdout);
}

static void send_protocol_error(const char *id_raw, int code, const char *msg) {
	GString *s = g_string_new("{\"jsonrpc\":\"2.0\",\"id\":");
	g_string_append(s, id_raw ? id_raw : "null");
	g_string_append_printf(s, ",\"error\":{\"code\":%d,\"message\":\"", code);
	json_escape_append(s, msg);
	g_string_append(s, "\"}}");
	send_line(s->str);
	g_string_free(s, TRUE);
}

static void send_tool_result(const char *id_raw, GString *content, bool is_error) {
	GString *s = g_string_new("{\"jsonrpc\":\"2.0\",\"id\":");

	g_string_append(s, id_raw ? id_raw : "null");
	g_string_append(s, ",\"result\":{\"content\":[");
	g_string_append_len(s, content->str, content->len);
	g_string_append_printf(s, "],\"isError\":%s}}", is_error ? "true" : "false");

	send_line(s->str);
	g_string_free(s, TRUE);
}

static void audit(const char *tool, const char *fmt, ...) {
	va_list ap;

	fprintf(stderr, "ydotool: mcp: tool=%s ", tool);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

/* ---------------------------------------------------------------- actions */

static int ensure_daemon(char *err, size_t err_sz) {
	if (fd_daemon_socket >= 0)
		return 0;

	if (ydotool_connect() < 0) {
		snprintf(err, err_sz, "ydotoold is not reachable; start it or set YDOTOOL_SOCKET");
		return -1;
	}

	return 0;
}

static bool rate_ok(void) {
	static gint64 window_start;
	static int count;

	gint64 now = g_get_monotonic_time();

	if (now - window_start > G_TIME_SPAN_SECOND) {
		window_start = now;
		count = 0;
	}

	return ++count <= opt_rate_limit;
}

static bool is_dangerous(uint16_t *codes, int n) {
	bool ctrl = false, alt = false, del = false, sysrq = false, fn = false;

	for (int i = 0; i < n; i++) {
		switch (codes[i]) {
		case KEY_LEFTCTRL:
		case KEY_RIGHTCTRL:
			ctrl = true;
			break;
		case KEY_LEFTALT:
		case KEY_RIGHTALT:
			alt = true;
			break;
		case KEY_DELETE:
			del = true;
			break;
		case KEY_SYSRQ:
			sysrq = true;
			break;
		default:
			if (codes[i] >= KEY_F1 && codes[i] <= KEY_F12)
				fn = true;
		}
	}

	if (ctrl && alt && (del || fn))
		return true;
	if (alt && sysrq)
		return true;

	return false;
}

static void emit_combo(uint16_t *codes, int n) {
	for (int i = 0; i < n; i++)
		uinput_emit(EV_KEY, codes[i], 1, 1);

	usleep(opt_key_hold_ms * 1000);

	for (int i = n - 1; i >= 0; i--)
		uinput_emit(EV_KEY, codes[i], 0, 1);

	usleep(opt_key_delay_ms * 1000);
}

static int check_focus(jsmntok_t *t, int ntok, const char *json, int args,
		       char *err, size_t err_sz) {
	char expect[128] = {0};
	int tok = mj_get(t, ntok, json, args, "expect_window");

	if (tok >= 0)
		mj_str(t, ntok, json, tok, expect, sizeof(expect));

	if (!expect[0] && !opt_require_focus)
		return 0;

	struct window_info act;

	if (perception->active_window(&act) < 0) {
		if (opt_require_focus) {
			snprintf(err, err_sz, "focus required but the active window is unknown");
			return -1;
		}
		return 0;
	}

	if (expect[0] && !ci_strstr(act.title, expect) && !ci_strstr(act.app, expect)) {
		snprintf(err, err_sz, "active window is \"%s\" (%s), expected \"%s\"",
			 act.title, act.app, expect);
		return -1;
	}

	return 0;
}

static void append_window_json(GString *s, const struct window_info *w) {
	g_string_append(s, "{\"app\":\"");
	json_escape_append(s, w->app);
	g_string_append(s, "\",\"title\":\"");
	json_escape_append(s, w->title);
	g_string_append_printf(s,
		"\",\"geometry\":{\"x\":%d,\"y\":%d,\"width\":%d,\"height\":%d},"
		"\"active\":%s}", w->x, w->y, w->w, w->h, w->active ? "true" : "false");
}

/* ------------------------------------------------------------ tool schemas */

struct mcp_tool {
	const char *name;
	const char *description;
	const char *schema;
	bool action;
};

#define SCHEMA_EMPTY "{\"type\":\"object\",\"properties\":{}}"
#define SCHEMA_EXPECT ",\"expect_window\":{\"type\":\"string\",\"description\":\"Refuse unless the active window title or application contains this substring\"}"

static const struct mcp_tool tools[] = {
	{
		"computer_get_state",
		"Report the active window and the list of windows with their geometry. "
		"Call this before acting to see where input will go.",
		SCHEMA_EMPTY, false,
	},
	{
		"computer_screenshot",
		"Capture the full screen as a PNG image. The desktop may ask the user "
		"to allow the capture.",
		SCHEMA_EMPTY, false,
	},
	{
		"computer_focus_window",
		"Best-effort focus of a window whose title or application contains the "
		"given substring. On GNOME Wayland the compositor does not allow direct "
		"activation, so this cycles windows and verifies the result.",
		"{\"type\":\"object\",\"properties\":{"
		"\"match\":{\"type\":\"string\",\"description\":\"Substring of the window title or application\"}"
		"},\"required\":[\"match\"]}", false,
	},
	{
		"computer_type",
		"Type literal text into the focused window.",
		"{\"type\":\"object\",\"properties\":{"
		"\"text\":{\"type\":\"string\"},"
		"\"layout\":{\"type\":\"string\",\"enum\":[\"us\",\"de\",\"fr\",\"dvorak\",\"colemak\"]},"
		"\"key_delay_ms\":{\"type\":\"integer\"},"
		"\"key_hold_ms\":{\"type\":\"integer\"}"
		SCHEMA_EXPECT "},\"required\":[\"text\"]}", true,
	},
	{
		"computer_press",
		"Press and release a combination of keys, for example [\"ctrl\",\"alt\",\"t\"].",
		"{\"type\":\"object\",\"properties\":{"
		"\"keys\":{\"type\":\"array\",\"items\":{\"type\":\"string\"},\"minItems\":1},"
		"\"hold_ms\":{\"type\":\"integer\"},"
		"\"repeat\":{\"type\":\"integer\",\"minimum\":1}"
		SCHEMA_EXPECT "},\"required\":[\"keys\"]}", true,
	},
	{
		"computer_key",
		"Press, hold down or release a single named key.",
		"{\"type\":\"object\",\"properties\":{"
		"\"key\":{\"type\":\"string\"},"
		"\"action\":{\"type\":\"string\",\"enum\":[\"press\",\"down\",\"up\"]}"
		SCHEMA_EXPECT "},\"required\":[\"key\"]}", true,
	},
	{
		"computer_click",
		"Click a mouse button one or more times.",
		"{\"type\":\"object\",\"properties\":{"
		"\"button\":{\"type\":\"string\",\"enum\":[\"left\",\"right\",\"middle\"]},"
		"\"count\":{\"type\":\"integer\",\"minimum\":1}"
		SCHEMA_EXPECT "}}", true,
	},
	{
		"computer_scroll",
		"Scroll the mouse wheel.",
		"{\"type\":\"object\",\"properties\":{"
		"\"direction\":{\"type\":\"string\",\"enum\":[\"up\",\"down\",\"left\",\"right\"]},"
		"\"amount\":{\"type\":\"integer\",\"minimum\":1}"
		SCHEMA_EXPECT "},\"required\":[\"direction\"]}", true,
	},
	{
		"computer_move",
		"Move the mouse pointer. Use dx/dy for a relative move or x/y for an "
		"absolute position.",
		"{\"type\":\"object\",\"properties\":{"
		"\"dx\":{\"type\":\"integer\"},\"dy\":{\"type\":\"integer\"},"
		"\"x\":{\"type\":\"integer\"},\"y\":{\"type\":\"integer\"}"
		SCHEMA_EXPECT "}}", true,
	},
#ifdef HAVE_TYPESAFE
	{
		"computer_do",
		"Interpret a natural-language input request with TypeSafe and perform "
		"it. Use the structured tools for precise control.",
		"{\"type\":\"object\",\"properties\":{"
		"\"request\":{\"type\":\"string\"},"
		"\"force\":{\"type\":\"boolean\"}"
		"},\"required\":[\"request\"]}", true,
	},
#endif
};

/* ----------------------------------------------------------- tool handlers */

static GString *h_get_state(jsmntok_t *t, int ntok, const char *json, int args,
			    char *err, size_t err_sz) {
	(void)t; (void)ntok; (void)json; (void)args; (void)err; (void)err_sz;

	GString *out = g_string_new("{");
	struct window_info act;
	bool have = perception->active_window(&act) == 0;

	g_string_append(out, "\"active_window\":");
	if (have)
		append_window_json(out, &act);
	else
		g_string_append(out, "null");

	g_string_append_printf(out, ",\"focus_known\":%s,\"windows\":[",
			       have ? "true" : "false");

	struct window_info wins[MCP_MAX_WINDOWS];
	size_t n = perception->scan(wins, MCP_MAX_WINDOWS);

	for (size_t i = 0; i < n; i++) {
		if (i)
			g_string_append_c(out, ',');
		append_window_json(out, &wins[i]);
	}

	g_string_append(out, "]}");

	GString *content = g_string_new(NULL);
	content_text(content, out->str);
	g_string_free(out, TRUE);

	return content;
}

static GString *h_screenshot(jsmntok_t *t, int ntok, const char *json, int args,
			     char *err, size_t err_sz) {
	(void)t; (void)ntok; (void)json; (void)args;

	char *data = NULL;
	size_t len = 0;
	char perr[256] = {0};

	GString *content = g_string_new(NULL);

	if (perception->screenshot(&data, &len, perr, sizeof(perr)) < 0) {
		snprintf(err, err_sz, "%s", perr[0] ? perr : "screenshot failed");
		content_text(content, err);
		return content;
	}

	char *b64 = g_base64_encode((const guchar *)data, len);
	free(data);

	char meta[64];
	snprintf(meta, sizeof(meta), "{\"mode\":\"screen\",\"bytes\":%zu}", len);

	content_image(content, b64, "image/png");
	content_text(content, meta);

	g_free(b64);

	return content;
}

static GString *h_focus_window(jsmntok_t *t, int ntok, const char *json, int args,
			       char *err, size_t err_sz) {
	char match[128] = {0};
	int tok = mj_get(t, ntok, json, args, "match");

	if (tok < 0 || mj_str(t, ntok, json, tok, match, sizeof(match)) < 0 || !match[0]) {
		snprintf(err, err_sz, "missing 'match'");
		return NULL;
	}

	struct window_info wins[MCP_MAX_WINDOWS];
	size_t n = perception->scan(wins, MCP_MAX_WINDOWS);
	int target = -1;

	for (size_t i = 0; i < n; i++) {
		if (ci_strstr(wins[i].title, match) || ci_strstr(wins[i].app, match)) {
			target = (int)i;
			break;
		}
	}

	GString *content = g_string_new(NULL);

	if (target < 0) {
		snprintf(err, err_sz, "no window matches \"%s\"", match);
		return content;
	}

	struct window_info act;
	bool known = perception->active_window(&act) == 0;
	int attempts = 0;

	if (known && (ci_strstr(act.title, match) || ci_strstr(act.app, match))) {
		GString *out = g_string_new("{\"focused\":true,\"attempts\":0,\"window\":");

		append_window_json(out, &act);
		g_string_append_c(out, '}');
		content_text(content, out->str);
		g_string_free(out, TRUE);
		return content;
	}

	char derr[256] = {0};

	if (ensure_daemon(derr, sizeof(derr)) < 0) {
		snprintf(err, err_sz, "%s", derr);
		return content;
	}

	if (!rate_ok()) {
		snprintf(err, err_sz, "rate limit exceeded");
		return content;
	}

	for (size_t i = 0; i < n + 2; i++) {
		uint16_t combo[] = {KEY_LEFTALT, KEY_ESC};

		attempts++;
		emit_combo(combo, 2);

		if (perception->wait_active_change(700, &act) < 0)
			continue;

		if (ci_strstr(act.title, match) || ci_strstr(act.app, match)) {
			GString *out = g_string_new("{\"focused\":true,\"attempts\":");
			g_string_append_printf(out, "%d,\"window\":", attempts);
			append_window_json(out, &act);
			g_string_append_c(out, '}');
			content_text(content, out->str);
			g_string_free(out, TRUE);
			audit("computer_focus_window", "match=\"%s\" attempts=%d ok", match, attempts);
			return content;
		}
	}

	if (perception->active_window(&act) == 0)
		snprintf(err, err_sz, "could not focus \"%s\"; active window is \"%s\" (%s)",
			 match, act.title, act.app);
	else
		snprintf(err, err_sz, "could not focus \"%s\"", match);

	audit("computer_focus_window", "match=\"%s\" attempts=%d failed", match, attempts);
	return content;
}

static int resolve_keys(jsmntok_t *t, int ntok, const char *json, int array_tok,
			uint16_t *codes, int max, char *err, size_t err_sz) {
	int n = mj_size(t, ntok, array_tok);

	if (n <= 0 || n > max) {
		snprintf(err, err_sz, "keys must be an array of 1 to %d names", max);
		return -1;
	}

	int i = array_tok + 1;

	for (int k = 0; k < n; k++) {
		char name[64];

		if (mj_str(t, ntok, json, i, name, sizeof(name)) < 0) {
			snprintf(err, err_sz, "key names must be strings");
			return -1;
		}

		int code = keyname_lookup(name);

		if (code < 0) {
			snprintf(err, err_sz, "unknown key '%s'", name);
			return -1;
		}

		codes[k] = (uint16_t)code;
		i = mj_skip(t, ntok, i);
	}

	return n;
}

static int prepare_action(jsmntok_t *t, int ntok, const char *json, int args,
			  uint16_t *codes, int n, char *err, size_t err_sz) {
	if (!opt_allow_dangerous && n > 0 && is_dangerous(codes, n)) {
		snprintf(err, err_sz, "refusing a dangerous key combination "
			 "(control+alt+delete, VT switch or magic sysrq); "
			 "pass --allow-dangerous to override");
		return -1;
	}

	if (opt_dry_run)
		return 0;

	if (check_focus(t, ntok, json, args, err, err_sz) < 0)
		return -1;

	if (!rate_ok()) {
		snprintf(err, err_sz, "rate limit exceeded (%d actions per second)", opt_rate_limit);
		return -1;
	}

	if (ensure_daemon(err, err_sz) < 0)
		return -1;

	return 0;
}

static GString *h_type(jsmntok_t *t, int ntok, const char *json, int args,
		       char *err, size_t err_sz) {
	char text[4096] = {0};
	char layout[32] = {0};
	long delay = opt_key_delay_ms, hold = opt_key_hold_ms;
	int tok = mj_get(t, ntok, json, args, "text");

	if (tok < 0 || mj_str(t, ntok, json, tok, text, sizeof(text)) < 0) {
		snprintf(err, err_sz, "missing or too long 'text'");
		return NULL;
	}

	tok = mj_get(t, ntok, json, args, "layout");
	if (tok >= 0)
		mj_str(t, ntok, json, tok, layout, sizeof(layout));

	tok = mj_get(t, ntok, json, args, "key_delay_ms");
	if (tok >= 0)
		mj_int(t, ntok, json, tok, &delay);

	tok = mj_get(t, ntok, json, args, "key_hold_ms");
	if (tok >= 0)
		mj_int(t, ntok, json, tok, &hold);

	const struct keymap *km = keymap_get(layout[0] ? layout : opt_layout);

	if (!km) {
		snprintf(err, err_sz, "unknown layout '%s'", layout);
		return NULL;
	}

	if (prepare_action(t, ntok, json, args, NULL, 0, err, err_sz) < 0)
		return NULL;

	GString *content = g_string_new(NULL);
	GString *out = g_string_new(NULL);

	if (opt_dry_run) {
		g_string_append_printf(out, "{\"dry_run\":true,\"action\":\"type\",\"chars\":%zu}",
				       strlen(text));
	} else {
		if (type_string(text, false, km, (int)delay, (int)hold) < 0) {
			snprintf(err, err_sz, "failed to type text");
			g_string_free(out, TRUE);
			return content;
		}

		g_string_append_printf(out, "{\"ok\":true,\"action\":\"type\",\"chars\":%zu}",
				       strlen(text));
	}

	content_text(content, out->str);
	g_string_free(out, TRUE);

	if (opt_log_args)
		audit("computer_type", "text=\"%s\"", text);
	else
		audit("computer_type", "chars=%zu dry_run=%d", strlen(text), opt_dry_run);

	return content;
}

static GString *h_press(jsmntok_t *t, int ntok, const char *json, int args,
			char *err, size_t err_sz) {
	uint16_t codes[16];
	int tok = mj_get(t, ntok, json, args, "keys");
	int n;

	if (tok < 0 || (n = resolve_keys(t, ntok, json, tok, codes, 16, err, err_sz)) < 0)
		return NULL;

	long hold = opt_key_hold_ms, repeat = 1;

	tok = mj_get(t, ntok, json, args, "hold_ms");
	if (tok >= 0)
		mj_int(t, ntok, json, tok, &hold);

	tok = mj_get(t, ntok, json, args, "repeat");
	if (tok >= 0)
		mj_int(t, ntok, json, tok, &repeat);

	if (repeat < 1 || repeat > 100) {
		snprintf(err, err_sz, "repeat must be between 1 and 100");
		return NULL;
	}

	if (prepare_action(t, ntok, json, args, codes, n, err, err_sz) < 0)
		return NULL;

	GString *content = g_string_new(NULL);
	GString *out = g_string_new(NULL);

	if (opt_dry_run) {
		g_string_append(out, "{\"dry_run\":true,\"action\":\"press\",\"keys\":[");
	} else {
		int saved = opt_key_hold_ms;
		opt_key_hold_ms = (int)hold;

		for (long r = 0; r < repeat; r++)
			emit_combo(codes, n);

		opt_key_hold_ms = saved;

		g_string_append(out, "{\"ok\":true,\"action\":\"press\",\"keys\":[");
	}

	for (int i = 0; i < n; i++)
		g_string_append_printf(out, "%s%d", i ? "," : "", codes[i]);

	g_string_append(out, "]}");
	content_text(content, out->str);
	g_string_free(out, TRUE);

	audit("computer_press", "keys=%d repeat=%ld dry_run=%d", n, repeat, opt_dry_run);
	return content;
}

static GString *h_key(jsmntok_t *t, int ntok, const char *json, int args,
		      char *err, size_t err_sz) {
	char name[64] = {0};
	char action[16] = "press";
	int tok = mj_get(t, ntok, json, args, "key");

	if (tok < 0 || mj_str(t, ntok, json, tok, name, sizeof(name)) < 0) {
		snprintf(err, err_sz, "missing 'key'");
		return NULL;
	}

	tok = mj_get(t, ntok, json, args, "action");
	if (tok >= 0)
		mj_str(t, ntok, json, tok, action, sizeof(action));

	int code = keyname_lookup(name);

	if (code < 0) {
		snprintf(err, err_sz, "unknown key '%s'", name);
		return NULL;
	}

	if (strcmp(action, "press") != 0 && strcmp(action, "down") != 0 &&
	    strcmp(action, "up") != 0) {
		snprintf(err, err_sz, "action must be press, down or up");
		return NULL;
	}

	uint16_t codes[] = {(uint16_t)code};

	if (prepare_action(t, ntok, json, args, codes, 1, err, err_sz) < 0)
		return NULL;

	if (!opt_dry_run) {
		if (strcmp(action, "down") == 0) {
			uinput_emit(EV_KEY, code, 1, 1);
		} else if (strcmp(action, "up") == 0) {
			uinput_emit(EV_KEY, code, 0, 1);
		} else {
			uinput_emit(EV_KEY, code, 1, 1);
			usleep(opt_key_hold_ms * 1000);
			uinput_emit(EV_KEY, code, 0, 1);
			usleep(opt_key_delay_ms * 1000);
		}
	}

	GString *content = g_string_new(NULL);
	GString *out = g_string_new(NULL);

	g_string_append_printf(out,
		"{\"ok\":true,\"dry_run\":%s,\"action\":\"key\",\"key\":%d,\"key_action\":\"%s\"}",
		opt_dry_run ? "true" : "false", code, action);

	content_text(content, out->str);
	g_string_free(out, TRUE);

	audit("computer_key", "key=%s action=%s dry_run=%d", name, action, opt_dry_run);
	return content;
}

static GString *h_click(jsmntok_t *t, int ntok, const char *json, int args,
			char *err, size_t err_sz) {
	char button[16] = "left";
	long count = 1;
	int tok = mj_get(t, ntok, json, args, "button");

	if (tok >= 0)
		mj_str(t, ntok, json, tok, button, sizeof(button));

	tok = mj_get(t, ntok, json, args, "count");
	if (tok >= 0)
		mj_int(t, ntok, json, tok, &count);

	if (count < 1 || count > 100) {
		snprintf(err, err_sz, "count must be between 1 and 100");
		return NULL;
	}

	uint16_t code = BTN_LEFT;

	if (strcmp(button, "right") == 0)
		code = BTN_RIGHT;
	else if (strcmp(button, "middle") == 0)
		code = BTN_MIDDLE;
	else if (strcmp(button, "left") != 0) {
		snprintf(err, err_sz, "button must be left, right or middle");
		return NULL;
	}

	uint16_t codes[] = {code};

	if (prepare_action(t, ntok, json, args, codes, 1, err, err_sz) < 0)
		return NULL;

	GString *content = g_string_new(NULL);
	GString *out = g_string_new(NULL);

	if (!opt_dry_run) {
		for (long i = 0; i < count; i++) {
			uinput_emit(EV_KEY, code, 1, 1);
			usleep(opt_key_hold_ms * 1000);
			uinput_emit(EV_KEY, code, 0, 1);
			usleep(opt_key_delay_ms * 1000);
		}
	}

	g_string_append_printf(out, "{\"ok\":true,\"dry_run\":%s,\"action\":\"click\",\"button\":\"%s\",\"count\":%ld}",
			       opt_dry_run ? "true" : "false", button, count);
	content_text(content, out->str);
	g_string_free(out, TRUE);

	audit("computer_click", "button=%s count=%ld dry_run=%d", button, count, opt_dry_run);
	return content;
}

static GString *h_scroll(jsmntok_t *t, int ntok, const char *json, int args,
			 char *err, size_t err_sz) {
	char direction[16] = {0};
	long amount = 1;
	int tok = mj_get(t, ntok, json, args, "direction");

	if (tok < 0 || mj_str(t, ntok, json, tok, direction, sizeof(direction)) < 0) {
		snprintf(err, err_sz, "missing 'direction'");
		return NULL;
	}

	tok = mj_get(t, ntok, json, args, "amount");
	if (tok >= 0)
		mj_int(t, ntok, json, tok, &amount);

	if (amount < 1 || amount > 100) {
		snprintf(err, err_sz, "amount must be between 1 and 100");
		return NULL;
	}

	uint16_t axis = REL_WHEEL;
	int value = (int)amount;

	if (strcmp(direction, "up") == 0) {
		value = (int)amount;
	} else if (strcmp(direction, "down") == 0) {
		value = -(int)amount;
	} else if (strcmp(direction, "left") == 0) {
		axis = REL_HWHEEL;
		value = -(int)amount;
	} else if (strcmp(direction, "right") == 0) {
		axis = REL_HWHEEL;
		value = (int)amount;
	} else {
		snprintf(err, err_sz, "direction must be up, down, left or right");
		return NULL;
	}

	if (prepare_action(t, ntok, json, args, NULL, 0, err, err_sz) < 0)
		return NULL;

	GString *content = g_string_new(NULL);
	GString *out = g_string_new(NULL);

	if (!opt_dry_run)
		uinput_emit(EV_REL, axis, value, 1);

	g_string_append_printf(out,
		"{\"ok\":true,\"dry_run\":%s,\"action\":\"scroll\",\"direction\":\"%s\",\"amount\":%ld}",
		opt_dry_run ? "true" : "false", direction, amount);
	content_text(content, out->str);
	g_string_free(out, TRUE);

	audit("computer_scroll", "direction=%s amount=%ld dry_run=%d",
	      direction, amount, opt_dry_run);
	return content;
}

static GString *h_move(jsmntok_t *t, int ntok, const char *json, int args,
		       char *err, size_t err_sz) {
	long dx = 0, dy = 0, x = 0, y = 0;
	int tok;
	bool have_dx = false, have_dy = false, have_x = false, have_y = false;

	if ((tok = mj_get(t, ntok, json, args, "dx")) >= 0 &&
	    mj_int(t, ntok, json, tok, &dx) == 0)
		have_dx = true;
	if ((tok = mj_get(t, ntok, json, args, "dy")) >= 0 &&
	    mj_int(t, ntok, json, tok, &dy) == 0)
		have_dy = true;
	if ((tok = mj_get(t, ntok, json, args, "x")) >= 0 &&
	    mj_int(t, ntok, json, tok, &x) == 0)
		have_x = true;
	if ((tok = mj_get(t, ntok, json, args, "y")) >= 0 &&
	    mj_int(t, ntok, json, tok, &y) == 0)
		have_y = true;

	bool absolute = have_x || have_y;

	if (absolute && (have_dx || have_dy)) {
		snprintf(err, err_sz, "use dx/dy for relative or x/y for absolute, not both");
		return NULL;
	}

	if (absolute && (!have_x || !have_y)) {
		snprintf(err, err_sz, "absolute move needs both x and y");
		return NULL;
	}

	if (!absolute && !have_dx && !have_dy) {
		snprintf(err, err_sz, "provide dx/dy or x/y");
		return NULL;
	}

	if (prepare_action(t, ntok, json, args, NULL, 0, err, err_sz) < 0)
		return NULL;

	GString *content = g_string_new(NULL);
	GString *out = g_string_new(NULL);

	if (!opt_dry_run) {
		if (absolute) {
			uinput_emit(EV_REL, REL_X, INT32_MIN, 0);
			uinput_emit(EV_REL, REL_Y, INT32_MIN, 1);
			uinput_emit(EV_REL, REL_X, (int32_t)x, 0);
			uinput_emit(EV_REL, REL_Y, (int32_t)y, 1);
		} else {
			if (have_dx)
				uinput_emit(EV_REL, REL_X, (int32_t)dx, !have_dy);
			if (have_dy)
				uinput_emit(EV_REL, REL_Y, (int32_t)dy, 1);
		}
	}

	g_string_append_printf(out,
		"{\"ok\":true,\"dry_run\":%s,\"action\":\"move\",\"absolute\":%s,\"dx\":%ld,\"dy\":%ld,\"x\":%ld,\"y\":%ld}",
		opt_dry_run ? "true" : "false", absolute ? "true" : "false", dx, dy, x, y);
	content_text(content, out->str);
	g_string_free(out, TRUE);

	audit("computer_move", "absolute=%d dry_run=%d", absolute, opt_dry_run);
	return content;
}

#ifdef HAVE_TYPESAFE
static GString *h_do(jsmntok_t *t, int ntok, const char *json, int args,
		     char *err, size_t err_sz) {
	char request[1024] = {0};
	bool force = false;
	int tok = mj_get(t, ntok, json, args, "request");

	if (tok < 0 || mj_str(t, ntok, json, tok, request, sizeof(request)) < 0 ||
	    !request[0]) {
		snprintf(err, err_sz, "missing 'request'");
		return NULL;
	}

	tok = mj_get(t, ntok, json, args, "force");
	if (tok >= 0)
		mj_bool(t, ntok, json, tok, &force);

	char *exe = g_file_read_link("/proc/self/exe", NULL);

	if (!exe) {
		snprintf(err, err_sz, "cannot resolve ydotool binary path");
		return NULL;
	}

	GPtrArray *argv = g_ptr_array_new();
	g_ptr_array_add(argv, exe);
	g_ptr_array_add(argv, "do");
	g_ptr_array_add(argv, "--json");

	char *layout_flag = g_strdup_printf("--layout=%s", opt_layout);
	g_ptr_array_add(argv, layout_flag);

	char *delay_flag = g_strdup_printf("--key-delay=%d", opt_key_delay_ms);
	char *hold_flag = g_strdup_printf("--key-hold=%d", opt_key_hold_ms);
	g_ptr_array_add(argv, delay_flag);
	g_ptr_array_add(argv, hold_flag);

	if (force)
		g_ptr_array_add(argv, "--force");

	if (opt_dry_run)
		g_ptr_array_add(argv, "--dry-run");

	g_ptr_array_add(argv, request);
	g_ptr_array_add(argv, NULL);

	gchar *out = NULL, *errout = NULL;
	gint status = 0;
	GError *gerr = NULL;

	gboolean ok = g_spawn_sync(NULL, (gchar **)argv->pdata, NULL, G_SPAWN_DEFAULT,
				   NULL, NULL, &out, &errout, &status, &gerr);

	GString *content = g_string_new(NULL);

	if (!ok) {
		snprintf(err, err_sz, "failed to run do: %s", gerr ? gerr->message : "?");
	} else {
		GString *text = g_string_new(NULL);

		if (out && out[0])
			g_string_append(text, out);

		if (errout && errout[0]) {
			if (text->len)
				g_string_append_c(text, '\n');
			g_string_append(text, errout);
		}

		content_text(content, text->str);
		g_string_free(text, TRUE);
	}

	bool failed = !ok || !g_spawn_check_wait_status(status, NULL);

	if (failed) {
		snprintf(err, err_sz, "computer_do failed");
		audit("computer_do", "chars=%zu force=%d failed", strlen(request), force);
	} else {
		audit("computer_do", "chars=%zu force=%d dry_run=%d",
		      strlen(request), force, opt_dry_run);
	}

	g_free(out);
	g_free(errout);
	g_free(layout_flag);
	g_free(delay_flag);
	g_free(hold_flag);
	g_ptr_array_free(argv, TRUE);
	g_free(exe);

	return content;
}
#endif

struct handler {
	const char *name;
	GString *(*fn)(jsmntok_t *t, int ntok, const char *json, int args,
		       char *err, size_t err_sz);
};

static const struct handler handlers[] = {
	{"computer_get_state", h_get_state},
	{"computer_screenshot", h_screenshot},
	{"computer_focus_window", h_focus_window},
	{"computer_type", h_type},
	{"computer_press", h_press},
	{"computer_key", h_key},
	{"computer_click", h_click},
	{"computer_scroll", h_scroll},
	{"computer_move", h_move},
#ifdef HAVE_TYPESAFE
	{"computer_do", h_do},
#endif
};

static bool tool_is_available(const struct mcp_tool *tool) {
	if (opt_read_only && tool->action)
		return false;

	return true;
}

/* ------------------------------------------------------------------- mcp */

static void handle_tools_list(const char *id_raw) {
	GString *s = g_string_new("{\"jsonrpc\":\"2.0\",\"id\":");
	bool first = true;

	g_string_append(s, id_raw ? id_raw : "null");
	g_string_append(s, ",\"result\":{\"tools\":[");

	for (size_t i = 0; i < sizeof(tools) / sizeof(tools[0]); i++) {
		if (!tool_is_available(&tools[i]))
			continue;

		if (!first)
			g_string_append_c(s, ',');
		first = false;

		g_string_append(s, "{\"name\":\"");
		json_escape_append(s, tools[i].name);
		g_string_append(s, "\",\"description\":\"");
		json_escape_append(s, tools[i].description);
		g_string_append(s, "\",\"inputSchema\":");
		g_string_append(s, tools[i].schema);
		g_string_append_c(s, '}');
	}

	g_string_append(s, "]}}");
	send_line(s->str);
	g_string_free(s, TRUE);
}

static void handle_tools_call(const char *line, jsmntok_t *toks, int ntok,
			      const char *id_raw, int params) {
	char name[64] = {0};
	int name_tok = mj_get(toks, ntok, line, params, "name");

	if (name_tok < 0 || mj_str(toks, ntok, line, name_tok, name, sizeof(name)) < 0) {
		send_protocol_error(id_raw, -32602, "missing tool name");
		return;
	}

	for (size_t i = 0; i < sizeof(tools) / sizeof(tools[0]); i++) {
		if (strcmp(tools[i].name, name) != 0)
			continue;

		if (!tool_is_available(&tools[i])) {
			GString *content = g_string_new(NULL);
			content_text(content, "this server is in read-only mode");
			send_tool_result(id_raw, content, true);
			g_string_free(content, TRUE);
			return;
		}

		int args = mj_get(toks, ntok, line, params, "arguments");
		char err[512] = {0};

		for (size_t h = 0; h < sizeof(handlers) / sizeof(handlers[0]); h++) {
			if (strcmp(handlers[h].name, name) != 0)
				continue;

			GString *content = handlers[h].fn(toks, ntok, line, args,
							  err, sizeof(err));

			if (!content) {
				content = g_string_new(NULL);
				if (!err[0])
					snprintf(err, sizeof(err), "invalid arguments");
				content_text(content, err);
				send_tool_result(id_raw, content, true);
				g_string_free(content, TRUE);
				return;
			}

			bool is_error = err[0] != '\0';

			if (is_error && !content->len)
				content_text(content, err);

			send_tool_result(id_raw, content, is_error);
			g_string_free(content, TRUE);
			return;
		}

		send_protocol_error(id_raw, -32602, "unknown tool");
		return;
	}

	send_protocol_error(id_raw, -32602, "unknown tool");
}

static void handle_line(char *line) {
	jsmntok_t toks[MCP_MAX_TOKENS];
	int ntok = mj_parse(line, toks, MCP_MAX_TOKENS);

	if (ntok <= 0 || toks[0].type != JSMN_OBJECT) {
		send_protocol_error("null", -32700, "parse error");
		return;
	}

	int method_tok = mj_get(toks, ntok, line, 0, "method");
	int id_tok = mj_get(toks, ntok, line, 0, "id");

	char id_raw[128] = {0};

	if (id_tok >= 0) {
		size_t len = (size_t)(toks[id_tok].end - toks[id_tok].start);

		if (len >= sizeof(id_raw))
			len = sizeof(id_raw) - 1;
		memcpy(id_raw, line + toks[id_tok].start, len);
		id_raw[len] = '\0';
	}

	if (method_tok < 0) {
		if (id_tok >= 0)
			send_protocol_error(id_raw, -32600, "invalid request");
		return;
	}

	char method[64] = {0};

	if (mj_str(toks, ntok, line, method_tok, method, sizeof(method)) < 0)
		return;

	/* Notifications carry no id and expect no response. */
	if (id_tok < 0)
		return;

	if (strcmp(method, "initialize") == 0) {
		GString *s = g_string_new("{\"jsonrpc\":\"2.0\",\"id\":");
		char version[32] = "2025-06-18";
		int params = mj_get(toks, ntok, line, 0, "params");

		if (params >= 0) {
			int vt = mj_get(toks, ntok, line, params, "protocolVersion");
			char requested[32] = {0};

			if (vt >= 0 &&
			    mj_str(toks, ntok, line, vt, requested, sizeof(requested)) == 0 &&
			    (strcmp(requested, "2025-06-18") == 0 ||
			     strcmp(requested, "2024-11-05") == 0))
				snprintf(version, sizeof(version), "%s", requested);
		}

		g_string_append(s, id_raw);
		g_string_append_printf(s,
			",\"result\":{\"protocolVersion\":\"%s\","
			"\"capabilities\":{\"tools\":{}},"
			"\"serverInfo\":{\"name\":\"ydotool\",\"version\":\"%s\"},"
			"\"instructions\":\"Controls the local desktop through ydotool. "
			"Actions go to the focused window: call computer_get_state and, "
			"when unsure, computer_screenshot before acting. Use expect_window "
			"to refuse when the wrong window is focused. Prefer the structured "
			"tools over computer_do.\"}}", version, VERSION);

		send_line(s->str);
		g_string_free(s, TRUE);
		return;
	}

	if (strcmp(method, "ping") == 0) {
		GString *s = g_string_new("{\"jsonrpc\":\"2.0\",\"id\":");

		g_string_append(s, id_raw);
		g_string_append(s, ",\"result\":{}}");
		send_line(s->str);
		g_string_free(s, TRUE);
		return;
	}

	if (strcmp(method, "tools/list") == 0) {
		handle_tools_list(id_raw);
		return;
	}

	if (strcmp(method, "tools/call") == 0) {
		int params = mj_get(toks, ntok, line, 0, "params");

		if (params < 0) {
			send_protocol_error(id_raw, -32602, "missing params");
			return;
		}

		handle_tools_call(line, toks, ntok, id_raw, params);
		return;
	}

	send_protocol_error(id_raw, -32601, "method not found");
}

/* ------------------------------------------------------------------ setup */

static int none_init(void) {
	return 0;
}

static size_t none_scan(struct window_info *wins, size_t max) {
	(void)wins;
	(void)max;
	return 0;
}

static int none_active(struct window_info *out) {
	(void)out;
	return -1;
}

static int none_wait(int timeout_ms, struct window_info *out) {
	(void)timeout_ms;
	(void)out;
	return -1;
}

static int none_screenshot(char **data, size_t *len, char *err, size_t err_sz) {
	(void)data;
	(void)len;
	snprintf(err, err_sz, "perception backend 'none': screenshots and windows unavailable");
	return -1;
}

static const struct perception_ops perception_none = {
	.name = "none",
	.init = none_init,
	.scan = none_scan,
	.active_window = none_active,
	.wait_active_change = none_wait,
	.screenshot = none_screenshot,
};

static gboolean handle_io(GIOChannel *source, GIOCondition cond, gpointer data) {
	(void)data;

	bool hup = (cond & (G_IO_HUP | G_IO_ERR)) != 0;

	for (;;) {
		GError *err = NULL;
		gchar *line = NULL;
		gsize len = 0;
		GIOStatus status = g_io_channel_read_line(source, &line, &len, NULL, &err);

		if (status == G_IO_STATUS_NORMAL) {
			if (len > MCP_MAX_LINE) {
				send_protocol_error("null", -32600, "message too large");
			} else {
				while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
					line[--len] = '\0';

				if (len > 0)
					handle_line(line);
			}

			g_free(line);
			if (err)
				g_error_free(err);
			continue;
		}

		if (status == G_IO_STATUS_AGAIN) {
			g_free(line);
			if (err)
				g_error_free(err);
			break;
		}

		/* EOF: the client closed the stream. */
		g_free(line);
		if (err)
			g_error_free(err);
		g_main_loop_quit(main_loop);
		return FALSE;
	}

	if (hup) {
		g_main_loop_quit(main_loop);
		return FALSE;
	}

	return TRUE;
}

static void show_help(void) {
	puts(
		"Usage: mcp [OPTION]...\n"
		"Run a Model Context Protocol computer-use server over stdio.\n"
		"\n"
		"Exposes ydotool input actions and, on GNOME, window/focus/screenshot\n"
		"perception to MCP hosts such as Claude Code, Codex, Cursor or opencode.\n"
		"\n"
		"Options:\n"
		"  -n, --dry-run                 Report planned actions without sending events\n"
		"      --read-only               Only expose perception tools\n"
		"      --require-focus           Refuse actions when the active window is unknown\n"
		"      --allow-dangerous         Allow ctrl+alt+delete, VT switches and sysrq\n"
		"      --rate-limit=N            Maximum actions per second (default: 20)\n"
		"      --perception=gnome|none   Perception backend (default: gnome)\n"
		"      --layout=NAME             Layout used by computer_type (default: us)\n"
		"      --key-delay=N             Delay between keys in ms (default: 20)\n"
		"      --key-hold=N              Key hold time in ms (default: 20)\n"
		"      --screenshot-timeout=N    Seconds to wait for screenshot consent (default: 60)\n"
		"  -l, --log-args                Log tool arguments to stderr (redacted by default)\n"
		"  -h, --help                    Display this help and exit\n"
		"\n"
		"stdout is reserved for MCP messages; logs go to stderr."
	);
}

int tool_mcp(int argc, char **argv) {
	if (argc < 1) {
		show_help();
		return 0;
	}

	while (1) {
		int c;
		static struct option long_options[] = {
			{"dry-run", no_argument, 0, 'n'},
			{"read-only", no_argument, 0, 1},
			{"require-focus", no_argument, 0, 2},
			{"allow-dangerous", no_argument, 0, 3},
			{"rate-limit", required_argument, 0, 4},
			{"perception", required_argument, 0, 5},
			{"layout", required_argument, 0, 6},
			{"key-delay", required_argument, 0, 7},
			{"key-hold", required_argument, 0, 8},
			{"screenshot-timeout", required_argument, 0, 9},
			{"log-args", no_argument, 0, 'l'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "nlh", long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'n':
			opt_dry_run = true;
			break;
		case 'l':
			opt_log_args = true;
			break;
		case 1:
			opt_read_only = true;
			break;
		case 2:
			opt_require_focus = true;
			break;
		case 3:
			opt_allow_dangerous = true;
			break;
		case 4:
			opt_rate_limit = (int)strtol(optarg, NULL, 10);
			break;
		case 5:
			if (strcmp(optarg, "gnome") == 0)
				perception = &perception_gnome;
			else if (strcmp(optarg, "none") == 0)
				perception = &perception_none;
			else {
				fprintf(stderr, "ydotool: mcp: unknown perception backend '%s'\n", optarg);
				return 2;
			}
			break;
		case 6:
			opt_layout = optarg;
			break;
		case 7:
			opt_key_delay_ms = (int)strtol(optarg, NULL, 10);
			break;
		case 8:
			opt_key_hold_ms = (int)strtol(optarg, NULL, 10);
			break;
		case 9:
			opt_screenshot_timeout_s = (int)strtol(optarg, NULL, 10);
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

	if (!keymap_get(opt_layout)) {
		fprintf(stderr, "ydotool: mcp: unknown layout '%s'. Available layouts: ",
			opt_layout);
		keymap_print_list(stderr);
		fprintf(stderr, "\n");
		return 2;
	}

	if (perception == &perception_gnome) {
		perception_gnome_set_screenshot_timeout(opt_screenshot_timeout_s);
		if (perception->init && perception->init() < 0)
			perception = &perception_none;
	}

	GIOChannel *in = g_io_channel_unix_new(STDIN_FILENO);

	g_io_channel_set_encoding(in, NULL, NULL);
	g_io_channel_set_flags(in, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_buffered(in, TRUE);
	g_io_add_watch(in, G_IO_IN | G_IO_HUP | G_IO_ERR, handle_io, NULL);

	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);

	return 0;
}