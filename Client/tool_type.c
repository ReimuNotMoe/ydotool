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
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>

#define MAX_HOTKEY_LENGTH 256
#define MAX_LAYOUT_NAME 64

static int opt_key_delay_ms = 20;
static int opt_key_hold_ms = 20;
static int opt_next_delay_ms = 0;
static int opt_debug = 0;
static int opt_four_digit = 0;
static char unicode_hotkey[MAX_HOTKEY_LENGTH] = "<Control><Shift>u";

static int escape_state = 0;
static char hex_str[3] = {0, 0, 0};

static int escape(char in) {
	switch (escape_state) {
		case 0:
			if (in == '\\') {
				escape_state = 1;
				return -1;
			} else {
				return in;
			}
		case 1:
			escape_state = 0;
			switch (in) {
				case 'n':
					return '\n';
				case 't':
					return '\t';
				case 'x':
					escape_state = 2;
					return -1;
				case '\\':
					return '\\';
				default:
					return -1;
			}
		case 2:
			escape_state = 3;
			hex_str[0] = in;
			return -1;
		case 3:
			escape_state = 0;
			hex_str[1] = in;
			return (int)strtol(hex_str, NULL, 16);
		default:
			abort();
	}
}

typedef struct {
	const char *name;
	uint16_t a_keycode;
	uint16_t b_keycode;
	uint16_t c_keycode;
	uint16_t d_keycode;
	uint16_t e_keycode;
	uint16_t f_keycode;
} layout_map_t;

typedef struct {
	uint16_t keycode;
	uint8_t modifiers;
} char_to_keycode_t;

static layout_map_t layout_mappings[] = {
	{"us", KEY_A, KEY_B, KEY_COMMA, KEY_D, KEY_E, KEY_F},
	{"us+altgr-intl", KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F},
	{"us+dvorak", KEY_A, KEY_N, KEY_I, KEY_H, KEY_D, KEY_Y},
	{"us+dvp", KEY_A, KEY_N, KEY_I, KEY_H, KEY_D, KEY_Y},
	{"us+dvorak-intl", KEY_A, KEY_N, KEY_I, KEY_H, KEY_D, KEY_Y},
	{"us+dvorak-programmer", KEY_A, KEY_N, KEY_I, KEY_H, KEY_D, KEY_Y},
	{"tr", KEY_A, KEY_B, KEY_COMMA, KEY_D, KEY_E, KEY_F},
	{NULL, 0, 0, 0, 0, 0, 0}
};

static char_to_keycode_t hex_digit_keycodes[16] = {
	{KEY_0, 0}, {KEY_1, 0}, {KEY_2, 0}, {KEY_3, 0}, {KEY_4, 0}, {KEY_5, 0}, {KEY_6, 0}, {KEY_7, 0},
	{KEY_8, 0}, {KEY_9, 0}, {KEY_A, 0}, {KEY_B, 0}, {KEY_C, 0}, {KEY_D, 0}, {KEY_E, 0}, {KEY_F, 0}
};

#define MOD_SHIFT  0x01
#define MOD_ALTGR  0x02
#define MOD_CAPS   0x04

#ifdef HAVE_LIBXKBCOMMON
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

typedef struct {
	xkb_keysym_t target;
	char letter;
	int xkb_keycode;
} key_search_t;

typedef struct {
	key_search_t *searches;
	int count;
} search_data_t;

static struct xkb_context *g_xkb_ctx = NULL;
static struct xkb_keymap *g_xkb_keymap = NULL;
static char g_current_layout[64] = "";
static char g_current_variant[64] = "";
static int g_use_direct_typing = 1;

static int get_keycode_for_codepoint(uint32_t codepoint, uint16_t *keycode, uint8_t *modifiers);
static void type_char_direct(uint16_t keycode, uint8_t modifiers);
static int try_type_char_direct(uint32_t codepoint);
static const char *keycode_to_name(uint16_t keycode);

static void lookup_xkb_keycode_for_keysym(struct xkb_keymap *keymap, xkb_keycode_t keycode, void *data) {
	search_data_t *search_data = (search_data_t *)data;
	const xkb_keysym_t *syms = NULL;
	xkb_keymap_key_get_syms_by_level(keymap, keycode, 0, 0, &syms);
	
	if (syms) {
		for (int i = 0; i < search_data->count; i++) {
			if (search_data->searches[i].xkb_keycode == 0 && syms[0] == search_data->searches[i].target) {
				search_data->searches[i].xkb_keycode = keycode;
			}
		}
	}
}


static int get_keycode_for_codepoint(uint32_t codepoint, uint16_t *keycode, uint8_t *modifiers) {
	if (!g_xkb_keymap) {
		return -1;
	}

	xkb_keysym_t keysym = xkb_utf32_to_keysym(codepoint);
	if (keysym == XKB_KEY_NoSymbol) {
		return -1;
	}

	xkb_keycode_t min_keycode = xkb_keymap_min_keycode(g_xkb_keymap);
	xkb_keycode_t max_keycode = xkb_keymap_max_keycode(g_xkb_keymap);

	for (xkb_keycode_t kc = min_keycode; kc <= max_keycode; kc++) {
		for (int level = 0; level < 4; level++) {
			const xkb_keysym_t *syms;
			int nsyms = xkb_keymap_key_get_syms_by_level(g_xkb_keymap, kc, 0, level, &syms);
			
			if (nsyms > 0 && syms[0] == keysym) {
				*keycode = kc - 8;
				*modifiers = 0;
				
				switch (level) {
					case 0:
						*modifiers = 0;
						break;
					case 1:
						*modifiers = MOD_SHIFT;
						break;
					case 2:
						*modifiers = MOD_ALTGR;
						break;
					case 3:
						*modifiers = MOD_SHIFT | MOD_ALTGR;
						break;
				}
				
				return 0;
			}
		}
	}

	return -1;
}

static int detect_keyboard_layout_libxkbcommon(const char *layout_name, const char *variant) {
	if (g_xkb_ctx) {
		xkb_context_unref(g_xkb_ctx);
	}
	if (g_xkb_keymap) {
		xkb_keymap_unref(g_xkb_keymap);
	}

	g_xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!g_xkb_ctx) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Failed to create xkb_context\n");
		}
		return -1;
	}

	strncpy(g_current_layout, layout_name, sizeof(g_current_layout) - 1);
	g_current_layout[sizeof(g_current_layout) - 1] = '\0';
	if (variant) {
		strncpy(g_current_variant, variant, sizeof(g_current_variant) - 1);
		g_current_variant[sizeof(g_current_variant) - 1] = '\0';
	} else {
		g_current_variant[0] = '\0';
	}

	struct xkb_rule_names names = {
		.layout = layout_name,
		.variant = variant
	};
	
	g_xkb_keymap = xkb_keymap_new_from_names(g_xkb_ctx, &names, 0);
	if (!g_xkb_keymap) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Failed to create keymap for %s+%s\n", 
			        layout_name, variant ? variant : "basic");
		}
		xkb_context_unref(g_xkb_ctx);
		g_xkb_ctx = NULL;
		return -1;
	}
	
	char hex_chars[] = "0123456789abcdef";
	int found_count = 0;
	
	if (opt_debug) {
		fprintf(stderr, "[DEBUG] libxkbcommon: Looking up all 16 hex digit keycodes for %s+%s\n",
		        layout_name, variant ? variant : "basic");
		fprintf(stderr, "[DEBUG] Applying libxkbcommon-detected keycodes:\n");
	}

	for (int i = 0; i < 16; i++) {
		uint16_t keycode;
		uint8_t modifiers;
		
		if (get_keycode_for_codepoint(hex_chars[i], &keycode, &modifiers) == 0) {
			hex_digit_keycodes[i].keycode = keycode;
			hex_digit_keycodes[i].modifiers = modifiers;
			found_count++;
			if (opt_debug) {
				const char *mod_str = "";
				if (modifiers & MOD_SHIFT) mod_str = " + Shift";
				fprintf(stderr, "[DEBUG]   %c -> keycode %d%s\n",
				        hex_chars[i], keycode, mod_str);
			}
		}
	}

	if (found_count == 16) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] libxkbcommon: Found all 16/16 hex digit keycodes\n");
		}
		return 0;
	} else {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] libxkbcommon: Only found %d/16 hex digit keycodes\n", found_count);
		}
		return -1;
	}
        
}

static void type_char_direct(uint16_t keycode, uint8_t modifiers) {
	uint16_t mod_keys[3];
	int mod_count = 0;
	
	if (modifiers & MOD_SHIFT) {
		mod_keys[mod_count++] = KEY_LEFTSHIFT;
	}
	if (modifiers & MOD_ALTGR) {
		mod_keys[mod_count++] = KEY_RIGHTALT;
	}
	
	for (int i = 0; i < mod_count; i++) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG]   Pressing modifier: %s\n", keycode_to_name(mod_keys[i]));
		}
		uinput_emit(EV_KEY, mod_keys[i], 1, 0);
	}
	
	if (opt_debug) {
		fprintf(stderr, "[DEBUG]   Typing character directly: keycode %d (%s)\n", keycode, keycode_to_name(keycode));
	}
	
	uinput_emit(EV_KEY, keycode, 1, 1);
	usleep(opt_key_hold_ms * 1000);
	uinput_emit(EV_KEY, keycode, 0, 1);
	
	for (int i = mod_count - 1; i >= 0; i--) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG]   Releasing modifier: %s\n", keycode_to_name(mod_keys[i]));
		}
		uinput_emit(EV_KEY, mod_keys[i], 0, 1);
	}
	
	if (opt_debug) {
		fprintf(stderr, "[DEBUG]   Direct typing complete\n");
	}
}

static int try_type_char_direct(uint32_t codepoint) {
	uint16_t keycode;
	uint8_t modifiers;
	
	if (!g_use_direct_typing || !g_xkb_keymap) {
		return -1;
	}
	
	if (get_keycode_for_codepoint(codepoint, &keycode, &modifiers) == 0) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Found direct key mapping for U+%04X: keycode=%d modifiers=0x%02x\n", 
			        codepoint, keycode, modifiers);
		}
		type_char_direct(keycode, modifiers);
		return 0;
	}
	
	return -1;
}

#else
static int g_use_direct_typing = 0;

static int try_type_char_direct(uint32_t codepoint) {
	return -1;
}
#endif

static const char *keycode_to_name(uint16_t keycode) {
	switch (keycode) {
		case KEY_LEFTCTRL: return "KEY_LEFTCTRL";
		case KEY_LEFTSHIFT: return "KEY_LEFTSHIFT";
		case KEY_LEFTALT: return "KEY_LEFTALT";
		case KEY_RIGHTCTRL: return "KEY_RIGHTCTRL";
		case KEY_RIGHTSHIFT: return "KEY_RIGHTSHIFT";
		case KEY_RIGHTALT: return "KEY_RIGHTALT";
		case KEY_U: return "KEY_U";
		case KEY_A: return "KEY_A";
		case KEY_B: return "KEY_B";
		case KEY_C: return "KEY_C";
		case KEY_D: return "KEY_D";
		case KEY_E: return "KEY_E";
		case KEY_F: return "KEY_F";
		case KEY_1: case KEY_2: case KEY_3: case KEY_4: case KEY_5:
		case KEY_6: case KEY_7: case KEY_8: case KEY_9: case KEY_0: {
			static char buf[16];
			snprintf(buf, sizeof(buf), "KEY_%d", keycode - KEY_1 + 1);
			return buf;
		}
		case KEY_ENTER: return "KEY_ENTER";
		case KEY_KP0: case KEY_KP1: case KEY_KP2: case KEY_KP3: case KEY_KP4:
		case KEY_KP5: case KEY_KP6: case KEY_KP7: case KEY_KP8: case KEY_KP9: {
			static char kpbuf[16];
			snprintf(kpbuf, sizeof(kpbuf), "KEY_KP%d", keycode - KEY_KP0);
			return kpbuf;
		}
		case KEY_Q: return "KEY_Q";
		case KEY_W: return "KEY_W";
		case KEY_R: return "KEY_R";
		case KEY_T: return "KEY_T";
		case KEY_Y: return "KEY_Y";
		case KEY_I: return "KEY_I";
		case KEY_O: return "KEY_O";
		case KEY_P: return "KEY_P";
		case KEY_S: return "KEY_S";
		case KEY_G: return "KEY_G";
		case KEY_H: return "KEY_H";
		case KEY_J: return "KEY_J";
		case KEY_K: return "KEY_K";
		case KEY_L: return "KEY_L";
		case KEY_Z: return "KEY_Z";
		case KEY_X: return "KEY_X";
		case KEY_V: return "KEY_V";
		case KEY_N: return "KEY_N";
		case KEY_M: return "KEY_M";
		case KEY_COMPOSE: return "KEY_COMPOSE";
		default: return "UNKNOWN";
	}
}

static uint16_t parse_key_modifier(const char *name) {
	if (strcasecmp(name, "Control") == 0 || strcasecmp(name, "Ctrl") == 0) {
		return KEY_LEFTCTRL;
	} else if (strcasecmp(name, "Shift") == 0) {
		return KEY_LEFTSHIFT;
	} else if (strcasecmp(name, "Alt") == 0) {
		return KEY_LEFTALT;
	} else if (strcasecmp(name, "Super") == 0 || strcasecmp(name, "Meta") == 0) {
		return KEY_LEFTMETA;
	}
	return 0;
}

static uint16_t parse_key_name(const char *name) {
	if (strcasecmp(name, "Multi_key") == 0 || strcasecmp(name, "Compose") == 0) {
		return KEY_COMPOSE;
	} else if (strlen(name) == 1) {
		char c = name[0];
		switch (c) {
			case 'a': case 'A': return KEY_A;
			case 'b': case 'B': return KEY_B;
			case 'c': case 'C': return KEY_C;
			case 'd': case 'D': return KEY_D;
			case 'e': case 'E': return KEY_E;
			case 'f': case 'F': return KEY_F;
			case 'g': case 'G': return KEY_G;
			case 'h': case 'H': return KEY_H;
			case 'i': case 'I': return KEY_I;
			case 'j': case 'J': return KEY_J;
			case 'k': case 'K': return KEY_K;
			case 'l': case 'L': return KEY_L;
			case 'm': case 'M': return KEY_M;
			case 'n': case 'N': return KEY_N;
			case 'o': case 'O': return KEY_O;
			case 'p': case 'P': return KEY_P;
			case 'q': case 'Q': return KEY_Q;
			case 'r': case 'R': return KEY_R;
			case 's': case 'S': return KEY_S;
			case 't': case 'T': return KEY_T;
			case 'u': case 'U': return KEY_U;
			case 'v': case 'V': return KEY_V;
			case 'w': case 'W': return KEY_W;
			case 'x': case 'X': return KEY_X;
			case 'y': case 'Y': return KEY_Y;
			case 'z': case 'Z': return KEY_Z;
		}
	}
	return 0;
}

static void type_key(uint16_t keycode) {
	if (opt_debug) {
		fprintf(stderr, "[DEBUG]   Typing key: %d (%s)\n", keycode, keycode_to_name(keycode));
		if (!is_keycode_allowed(keycode)) {
			fprintf(stderr, "ydotool: type: warning: keycode %d may not be allowed by ydotoold\n", keycode);
		}
	}
	uinput_emit(EV_KEY, keycode, 1, 1);
	usleep(opt_key_hold_ms * 1000);
	uinput_emit(EV_KEY, keycode, 0, 1);
}

static void type_hex_digit(uint8_t digit) {
	if (digit < 16) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG]   Typing hex digit: %x (keycode: %d - %s, modifiers: 0x%02x)\n", 
					digit, hex_digit_keycodes[digit].keycode, 
					keycode_to_name(hex_digit_keycodes[digit].keycode),
					hex_digit_keycodes[digit].modifiers);
		}
		
		if (hex_digit_keycodes[digit].modifiers & MOD_SHIFT) {
			uinput_emit(EV_KEY, KEY_LEFTSHIFT, 1, 0);
			if (opt_debug) {
				fprintf(stderr, "[DEBUG]   Pressing modifier: KEY_LEFTSHIFT\n");
			}
		}
		
		type_key(hex_digit_keycodes[digit].keycode);
		
		if (hex_digit_keycodes[digit].modifiers & MOD_SHIFT) {
			uinput_emit(EV_KEY, KEY_LEFTSHIFT, 0, 1);
			if (opt_debug) {
				fprintf(stderr, "[DEBUG]   Releasing modifier: KEY_LEFTSHIFT\n");
			}
		}
	}
}

static void type_unicode_codepoint(uint32_t codepoint) {
	uint8_t hex_digits[8];
	int digit_count = 0;

	while (codepoint > 0) {
		hex_digits[digit_count++] = codepoint & 0xF;
		codepoint >>= 4;
	}

	if (digit_count == 0) {
		digit_count = 1;
		hex_digits[0] = 0;
	}

	if (opt_debug) {
		fprintf(stderr, "[DEBUG] Step 3: Typing Unicode codepoint U+%04X\n", 
				hex_digits[3] << 12 | hex_digits[2] << 8 | hex_digits[1] << 4 | hex_digits[0]);
		fprintf(stderr, "[DEBUG]   Total hex digits: %d\n", digit_count);
	}

	if (opt_four_digit && digit_count < 4) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG]   Padding to 4 digits\n");
		}
		while (digit_count < 4) {
			hex_digits[digit_count++] = 0;
		}
	}

	for (int i = digit_count - 1; i >= 0; i--) {
		type_hex_digit(hex_digits[i]);
		usleep(opt_key_delay_ms * 1000);
	}

	type_key(KEY_ENTER);
	usleep(opt_key_delay_ms * 1000);
}

static void parse_and_send_unicode_hotkey(void) {
	char hotkey[MAX_HOTKEY_LENGTH];
	strncpy(hotkey, unicode_hotkey, sizeof(hotkey) - 1);
	hotkey[sizeof(hotkey) - 1] = '\0';

	uint16_t modifiers[16] = {0};
	uint16_t main_key = 0;
	int mod_count = 0;

	char *token = strtok(hotkey, "<>");
	while (token != NULL) {
		if (mod_count < 16) {
			uint16_t mod = parse_key_modifier(token);
			uint16_t key = parse_key_name(token);

			if (mod != 0) {
				modifiers[mod_count++] = mod;
			} else if (key != 0) {
				main_key = key;
			}
		}
		token = strtok(NULL, "<>");
	}

	if (opt_debug) {
		fprintf(stderr, "[DEBUG] Step 1: Sending Unicode input hotkey sequence\n");
		fprintf(stderr, "[DEBUG]   Using hotkey from IBUS config: %s\n", unicode_hotkey);
		if (!is_keycode_allowed(main_key)) {
			fprintf(stderr, "ydotool: type: error: Invalid keycode %d (%s) from IBUS config\n",
					main_key, keycode_to_name(main_key));
			exit(1);
		}
		for (int i = 0; i < mod_count; i++) {
			if (!is_keycode_allowed(modifiers[i])) {
				fprintf(stderr, "ydotool: type: error: Invalid keycode %d (%s) from IBUS config\n",
						modifiers[i], keycode_to_name(modifiers[i]));
				exit(1);
			}
		}
	}

	for (int i = 0; i < mod_count; i++) {
		if (modifiers[i] != 0) {
			if (opt_debug) {
				fprintf(stderr, "[DEBUG]   Pressing modifier: %s\n", keycode_to_name(modifiers[i]));
			}
			uinput_emit(EV_KEY, modifiers[i], 1, 0);
		}
	}

	if (main_key != 0) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG]   Pressing main key: %s\n", keycode_to_name(main_key));
		}
		uinput_emit(EV_KEY, main_key, 1, 1);
		usleep(opt_key_hold_ms * 1000);
		if (opt_debug) {
			fprintf(stderr, "[DEBUG]   Releasing main key: %s\n", keycode_to_name(main_key));
		}
		uinput_emit(EV_KEY, main_key, 0, 1);
	}

	for (int i = mod_count - 1; i >= 0; i--) {
		if (modifiers[i] != 0) {
			if (opt_debug) {
				fprintf(stderr, "[DEBUG]   Releasing modifier: %s\n", keycode_to_name(modifiers[i]));
			}
			uinput_emit(EV_KEY, modifiers[i], 0, 1);
		}
	}

	if (opt_debug) {
		fprintf(stderr, "[DEBUG] Step 2: Waiting before typing hex digits (%dms)\n", opt_key_delay_ms);
	}

	usleep(opt_key_delay_ms * 1000);
}

static void read_ibus_unicode_hotkey(void) {
	FILE *fp = popen("gsettings get org.freedesktop.ibus.panel.emoji unicode-hotkey 2>/dev/null", "r");
	if (fp) {
		char buffer[MAX_HOTKEY_LENGTH];
		if (fgets(buffer, sizeof(buffer), fp)) {
			char *start = strchr(buffer, '\'');
			if (start) {
				start++;
				char *end = strchr(start, '\'');
				if (end) {
					*end = '\0';
					strncpy(unicode_hotkey, start, sizeof(unicode_hotkey) - 1);
					unicode_hotkey[sizeof(unicode_hotkey) - 1] = '\0';
					if (opt_debug) {
						fprintf(stderr, "[DEBUG] IBUS Unicode hotkey detected: %s\n", unicode_hotkey);
					}
				}
			}
		}
		pclose(fp);
	} else if (opt_debug) {
		fprintf(stderr, "[DEBUG] IBUS not available, using default hotkey: %s\n", unicode_hotkey);
	}
}

static void apply_hardcoded_layout_mapping(const char *layout_name) {
	for (int i = 0; layout_mappings[i].name != NULL; i++) {
		if (strcmp(layout_name, layout_mappings[i].name) == 0) {
			hex_digit_keycodes[10].keycode = layout_mappings[i].a_keycode;
			hex_digit_keycodes[11].keycode = layout_mappings[i].b_keycode;
			hex_digit_keycodes[12].keycode = layout_mappings[i].c_keycode;
			hex_digit_keycodes[13].keycode = layout_mappings[i].d_keycode;
			hex_digit_keycodes[14].keycode = layout_mappings[i].e_keycode;
			hex_digit_keycodes[15].keycode = layout_mappings[i].f_keycode;

			if (opt_debug) {
				fprintf(stderr, "[DEBUG] Applied hardcoded layout mapping for %s:\n", layout_name);
				fprintf(stderr, "[DEBUG]   A->%d (KEY_%d), B->%d (KEY_%d), C->%d (KEY_%d)\n",
					hex_digit_keycodes[10].keycode, hex_digit_keycodes[10].keycode,
					hex_digit_keycodes[11].keycode, hex_digit_keycodes[11].keycode,
					hex_digit_keycodes[12].keycode, hex_digit_keycodes[12].keycode);
				fprintf(stderr, "[DEBUG]   D->%d (KEY_%d), E->%d (KEY_%d), F->%d (KEY_%d)\n",
					hex_digit_keycodes[13].keycode, hex_digit_keycodes[13].keycode,
					hex_digit_keycodes[14].keycode, hex_digit_keycodes[14].keycode,
					hex_digit_keycodes[15].keycode, hex_digit_keycodes[15].keycode);
			}
			return;
		}
	}

	if (opt_debug) {
		fprintf(stderr, "[DEBUG] Layout '%s' not in hardcoded table, using US default\n", layout_name);
	}
}

static void detect_keyboard_layout(void) {
	char buffer[MAX_HOTKEY_LENGTH];
	FILE *fp = popen("gsettings get org.gnome.desktop.input-sources mru-sources 2>/dev/null", "r");

	if (!fp) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Could not detect keyboard layout, using US default\n");
		}
		return;
	}

	if (!fgets(buffer, sizeof(buffer), fp)) {
		pclose(fp);
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Could not read keyboard layout, using US default\n");
		}
		return;
	}
	pclose(fp);

	char *layout_name = NULL;
	char *variant = NULL;

	char *p = buffer;
	while (*p && !layout_name) {
		char *start1 = strchr(p, '\'');
		if (!start1) break;
		start1++;

		char *end1 = strchr(start1, '\'');
		if (!end1) break;
		*end1 = '\0';

		if (strcmp(start1, "xkb") == 0) {
			char *start2 = strchr(end1 + 1, '\'');
			if (start2) {
				start2++;
				char *end2 = strchr(start2, '\'');
				if (end2) {
					*end2 = '\0';
					layout_name = start2;
					break;
				}
			}
		}

		p = end1 + 1;
	}

	if (!layout_name) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Could not parse keyboard layout from: %s\n", buffer);
			fprintf(stderr, "[DEBUG] Using US default\n");
		}
		return;
	}

	if (opt_debug) {
		fprintf(stderr, "[DEBUG] Detected keyboard layout: %s\n", layout_name);
	}

#ifdef HAVE_LIBXKBCOMMON
	char *plus = strchr(layout_name, '+');
	if (plus) {
		*plus = '\0';
		variant = plus + 1;
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Layout variant: %s, variant: %s\n", layout_name, variant);
		}
	} else {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Layout: %s (no variant)\n", layout_name);
		}
	}

	if (detect_keyboard_layout_libxkbcommon(layout_name, variant) == 0) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Successfully looked up layout using libxkbcommon\n");
		}
		return;
	}

	if (opt_debug) {
		fprintf(stderr, "[DEBUG] libxkbcommon lookup failed, trying hardcoded table\n");
	}
#endif

	apply_hardcoded_layout_mapping(layout_name);
}

static uint32_t utf8_to_codepoint(const char *str, int *bytes_read) {
	unsigned char c0 = str[0];

	if (c0 < 0x80) {
		*bytes_read = 1;
		return c0;
	} else if ((c0 & 0xE0) == 0xC0) {
		if (str[1] == '\0') {
			*bytes_read = 1;
			return 0xFFFD;
		}
		*bytes_read = 2;
		return ((c0 & 0x1F) << 6) | (str[1] & 0x3F);
	} else if ((c0 & 0xF0) == 0xE0) {
		if (str[1] == '\0' || str[2] == '\0') {
			*bytes_read = 1;
			return 0xFFFD;
		}
		*bytes_read = 3;
		return ((c0 & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
	} else if ((c0 & 0xF8) == 0xF0) {
		if (str[1] == '\0' || str[2] == '\0' || str[3] == '\0') {
			*bytes_read = 1;
			return 0xFFFD;
		}
		*bytes_read = 4;
		return ((c0 & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
	}

	*bytes_read = 1;
	return 0xFFFD;
}

static void show_help(void) {
	puts(
		"Usage: type [OPTION]... [STRINGS]...\n"
		"Type strings using Unicode input method or direct keystrokes.\n"
		"\n"
		"Options:");
	printf(
		"  -d, --key-delay=N          Delay N milliseconds between keys (default: %d)\n", opt_key_delay_ms
		);
	printf(
		"  -H, --key-hold=N           Hold each key for N milliseconds (default: %d)\n", opt_key_hold_ms
		);
	printf(
		"  -D, --next-delay=N         Delay N milliseconds between command line strings (default: %d)\n", opt_next_delay_ms
		);
	puts(
		"  -f, --file=PATH            Specify a file, the contents of which will be be typed as if passed as an argument.\n"
		"                               The filepath may also be '-' to read from stdin\n"
		"  -e, --escape=BOOL          Escape enable (1) or disable (0)\n"
		"  -4, --four-digit           Pad hex output to exactly 4 digits (BMP only)\n"
		"  -v, --debug                Show debug output with step-by-step information\n"
		"  -u, --unicode-only         Force Unicode input mode (disable direct keystrokes)\n"
		"  -h, --help                 Display this help and exit\n"
		"\n"
		"Escape is enabled by default when typing command line arguments, and disabled by default when typing from file and stdin.\n"
		"Detects encoding from environment variables (LC_ALL, LC_CTYPE, LANG).\n"
		"Supports UTF-8 encoded strings including emojis and Unicode characters.\n"
		"\n"
		"Direct Keystroke Typing:\n"
		"  Automatically uses direct keystrokes for characters available on current layout.\n"
		"  Falls back to Unicode input for characters not directly accessible.\n"
		"  Supports ALL keyboard layouts detected by libxkbcommon.\n"
		"  Handles special cases like Turkish (ı/İ, i/I) and Programmer Dvorak.\n"
		"  Uses Shift for uppercase characters and AltGr for third-level symbols.\n"
		"\n"
		"Keyboard Layout Detection:\n"
		"  Automatically detects current keyboard layout from Gnome settings.\n"
		"  Adapts hex digit keycodes (A-F) to match the active layout.\n"
		"  Supports: us, us+altgr-intl, us+dvorak, us+dvp, us+dvorak-intl, us+dvorak-programmer, tr\n"
		"  Falls back to US layout for unsupported layouts.\n"
		"\n"
		"Unicode Input Configuration:\n"
		"  Uses IBUS Unicode input method if available.\n"
		"  Reads shortcut from: gsettings get org.freedesktop.ibus.panel.emoji unicode-hotkey\n"
		"  Falls back to Ctrl+Shift+u if IBUS is not configured.\n"
		"  Validates all keycodes against ydotoold allowed list.\n"
		"\n"
		"Note: Unicode input format varies by system. Most Linux systems support\n"
		"both holding Ctrl+Shift+u while typing hex digits, and releasing after,\n"
		"OR pressing Ctrl+Shift+u, releasing, then typing hex digits and pressing Enter.\n"
		"This implementation uses the second method for better compatibility."
		);
}

static void type_codepoint(uint32_t codepoint, bool delay) {
	if (codepoint == 0xFFFD) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Invalid UTF-8, skipping\n");
		}
		return;
	}

	if (try_type_char_direct(codepoint) != 0) {
		if (opt_debug) {
			fprintf(stderr, "[DEBUG] Direct typing not available for U+%04X, using Unicode input\n", codepoint);
		}
		parse_and_send_unicode_hotkey();
		type_unicode_codepoint(codepoint);
	}

	if (delay) {
		usleep(opt_key_delay_ms * 1000);
	}
}

int tool_type(int argc, char **argv) {
	if (argc < 2) {
		show_help();
		return 0;
	}

	const char *file_path = NULL;
	int enable_escape = -1;

	while (1) {
		int c;

		static struct option long_options[] = {
			{"key-delay", required_argument, 0, 'd'},
			{"next-delay", required_argument, 0, 'D'},
			{"key-hold", required_argument, 0, 'H'},
			{"four-digit", no_argument, 0, '4'},
			{"unicode-only", no_argument, 0, 'u'},
			{"debug", no_argument, 0, 'v'},
			{"escape", required_argument, 0, 'e'},
			{"file", required_argument, 0, 'f'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};

		int option_index = 0;
		c = getopt_long(argc, argv, "hd:D:H:4uvf:e:", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'd':
				opt_key_delay_ms = strtol(optarg, NULL, 10);
				if (opt_debug) fprintf(stderr, "[DEBUG] Key delay set to %dms\n", opt_key_delay_ms);
				break;

			case 'D':
				opt_next_delay_ms = strtol(optarg, NULL, 10);
				if (opt_debug) fprintf(stderr, "[DEBUG] Next delay set to %dms\n", opt_next_delay_ms);
				break;

			case 'H':
				opt_key_hold_ms = strtol(optarg, NULL, 10);
				if (opt_debug) fprintf(stderr, "[DEBUG] Key hold set to %dms\n", opt_key_hold_ms);
				break;

			case 'f':
				file_path = optarg;
				break;

			case 'e':
				enable_escape = strtol(optarg, NULL, 10);
				break;

			case '4':
				opt_four_digit = 1;
				if (opt_debug) fprintf(stderr, "[DEBUG] Four-digit padding enabled\n");
				break;

			case 'u':
				g_use_direct_typing = 0;
				if (opt_debug) fprintf(stderr, "[DEBUG] Direct typing disabled, using Unicode-only mode\n");
				break;

			case 'v':
				opt_debug = 1;
				fprintf(stderr, "[DEBUG] Debug mode enabled\n");
				break;

			case 'h':
				show_help();
				exit(0);

			case '?':
				break;

			default:
				abort();
		}
	}

	setlocale(LC_ALL, "");

	read_ibus_unicode_hotkey();
	detect_keyboard_layout();

	if (file_path) {
		if (enable_escape == -1) {
			enable_escape = 0;
		}

		int fd = (strcmp(file_path, "-") == 0)
			 ? STDIN_FILENO
			 : open(file_path, O_RDONLY);

		if (fd == -1) {
			fprintf(stderr, "ydotool: type: error: failed to open %s: %s\n", file_path,
				strerror(errno));
			return 2;
		}

		char buf[4096];
		ssize_t rc;

		while ((rc = read(fd, buf, sizeof(buf)))) {
			if (rc > 0) {
				int pos = 0;
				while (pos < rc) {
					if (enable_escape) {
						int c = escape(buf[pos]);
                                                pos++;
                                                
						if (c == -1) {
							continue;
						}

						if (c == 0) {
							break;
						}
                                                
                                                if (buf[pos-1]!=c) {
                                                  type_codepoint(c, pos-1 < rc);
                                                } else {
                                                  int bytes_read;
                                                  uint32_t codepoint = utf8_to_codepoint(&buf[pos-1], &bytes_read);
                                                  type_codepoint(codepoint, pos-1 < rc);
                                                  pos += bytes_read-1;
                                                }
					} else {
						int bytes_read;
						uint32_t codepoint = utf8_to_codepoint(&buf[pos], &bytes_read);
						pos += bytes_read;
						type_codepoint(codepoint, pos-1 < rc);
					}
				}
			} else if (rc < 0) {
				fprintf(stderr, "ydotool: type: error: read %s failed: %s\n", file_path, strerror(errno));
				close(fd);
				return 2;
			}
		}

		if (fd != STDIN_FILENO) {
			close(fd);
		}
	} else {
		if (enable_escape == -1) {
			enable_escape = 1;
		}

		if (optind < argc) {
			int char_index = 0;
			while (optind < argc) {
				char *pstr = argv[optind++];
				int pos = 0;

				if (opt_debug) {
					fprintf(stderr, "\n[DEBUG] Processing argument %d: \"%s\"\n", char_index++, pstr);
					fprintf(stderr, "[DEBUG] String length: %zu bytes\n", strlen(pstr));
				}

				while (pstr[pos] != '\0') {
					if (enable_escape) {
						int c = escape(pstr[pos]);
                                                pos++;
                                                
						if (c == -1) {
							continue;
						}

						if (c == 0) {
							break;
						}

                                                if (pstr[pos-1]!=c) {
                                                  type_codepoint(c, pstr[pos] != '\0');
                                                } else {
                                                  int bytes_read;
                                                  uint32_t codepoint = utf8_to_codepoint(&pstr[pos-1], &bytes_read);
                                                  type_codepoint(codepoint, pstr[pos] != '\0');
                                                  pos += bytes_read-1;
                                                }
					} else {
						int bytes_read;
						uint32_t codepoint = utf8_to_codepoint(&pstr[pos], &bytes_read);
						pos += bytes_read;
						type_codepoint(codepoint, pstr[pos] != '\0');
					}
				}

				if (argv[optind] && opt_debug) {
					fprintf(stderr, "\n[DEBUG] Waiting before next argument (%dms)\n", opt_next_delay_ms);
				}
				if (argv[optind]) {
					usleep(opt_next_delay_ms * 1000);
				}
			}
		} else {
			show_help();
		}
	}

	if (opt_debug) {
		fprintf(stderr, "\n[DEBUG] All characters processed successfully\n");
	}

	return 0;
}
