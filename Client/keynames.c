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

#include "keynames.h"

#include <ctype.h>
#include <string.h>

#include <linux/input-event-codes.h>

struct keyname {
	const char *name;
	uint16_t code;
};

static const struct keyname keynames[] = {
	{"esc", KEY_ESC},
	{"escape", KEY_ESC},
	{"tab", KEY_TAB},
	{"enter", KEY_ENTER},
	{"return", KEY_ENTER},
	{"space", KEY_SPACE},
	{"spacebar", KEY_SPACE},
	{"backspace", KEY_BACKSPACE},
	{"bspace", KEY_BACKSPACE},
	{"delete", KEY_DELETE},
	{"del", KEY_DELETE},
	{"insert", KEY_INSERT},
	{"ins", KEY_INSERT},
	{"home", KEY_HOME},
	{"end", KEY_END},
	{"pageup", KEY_PAGEUP},
	{"pgup", KEY_PAGEUP},
	{"prior", KEY_PAGEUP},
	{"pagedown", KEY_PAGEDOWN},
	{"pgdn", KEY_PAGEDOWN},
	{"next", KEY_PAGEDOWN},
	{"up", KEY_UP},
	{"down", KEY_DOWN},
	{"left", KEY_LEFT},
	{"right", KEY_RIGHT},
	{"capslock", KEY_CAPSLOCK},
	{"numlock", KEY_NUMLOCK},
	{"scrolllock", KEY_SCROLLLOCK},
	{"printscreen", KEY_SYSRQ},
	{"prtsc", KEY_SYSRQ},
	{"sysrq", KEY_SYSRQ},
	{"pause", KEY_PAUSE},
	{"menu", KEY_MENU},
	{"compose", KEY_COMPOSE},
	{"power", KEY_POWER},
	{"sleep", KEY_SLEEP},
	{"wakeup", KEY_WAKEUP},
	{"ctrl", KEY_LEFTCTRL},
	{"control", KEY_LEFTCTRL},
	{"leftctrl", KEY_LEFTCTRL},
	{"lctrl", KEY_LEFTCTRL},
	{"rightctrl", KEY_RIGHTCTRL},
	{"rctrl", KEY_RIGHTCTRL},
	{"shift", KEY_LEFTSHIFT},
	{"leftshift", KEY_LEFTSHIFT},
	{"lshift", KEY_LEFTSHIFT},
	{"rightshift", KEY_RIGHTSHIFT},
	{"rshift", KEY_RIGHTSHIFT},
	{"alt", KEY_LEFTALT},
	{"leftalt", KEY_LEFTALT},
	{"lalt", KEY_LEFTALT},
	{"rightalt", KEY_RIGHTALT},
	{"ralt", KEY_RIGHTALT},
	{"altgr", KEY_RIGHTALT},
	{"meta", KEY_LEFTMETA},
	{"super", KEY_LEFTMETA},
	{"win", KEY_LEFTMETA},
	{"windows", KEY_LEFTMETA},
	{"leftmeta", KEY_LEFTMETA},
	{"lmeta", KEY_LEFTMETA},
	{"rightmeta", KEY_RIGHTMETA},
	{"rmeta", KEY_RIGHTMETA},
	{"minus", KEY_MINUS},
	{"hyphen", KEY_MINUS},
	{"equal", KEY_EQUAL},
	{"equals", KEY_EQUAL},
	{"grave", KEY_GRAVE},
	{"backtick", KEY_GRAVE},
	{"tilde", KEY_GRAVE},
	{"semicolon", KEY_SEMICOLON},
	{"apostrophe", KEY_APOSTROPHE},
	{"quote", KEY_APOSTROPHE},
	{"comma", KEY_COMMA},
	{"dot", KEY_DOT},
	{"period", KEY_DOT},
	{"slash", KEY_SLASH},
	{"backslash", KEY_BACKSLASH},
	{"leftbrace", KEY_LEFTBRACE},
	{"leftbracket", KEY_LEFTBRACE},
	{"rightbrace", KEY_RIGHTBRACE},
	{"rightbracket", KEY_RIGHTBRACE},
	{"a", KEY_A}, {"b", KEY_B}, {"c", KEY_C}, {"d", KEY_D},
	{"e", KEY_E}, {"f", KEY_F}, {"g", KEY_G}, {"h", KEY_H},
	{"i", KEY_I}, {"j", KEY_J}, {"k", KEY_K}, {"l", KEY_L},
	{"m", KEY_M}, {"n", KEY_N}, {"o", KEY_O}, {"p", KEY_P},
	{"q", KEY_Q}, {"r", KEY_R}, {"s", KEY_S}, {"t", KEY_T},
	{"u", KEY_U}, {"v", KEY_V}, {"w", KEY_W}, {"x", KEY_X},
	{"y", KEY_Y}, {"z", KEY_Z},
	{"0", KEY_0}, {"1", KEY_1}, {"2", KEY_2}, {"3", KEY_3},
	{"4", KEY_4}, {"5", KEY_5}, {"6", KEY_6}, {"7", KEY_7},
	{"8", KEY_8}, {"9", KEY_9},
	{"f1", KEY_F1}, {"f2", KEY_F2}, {"f3", KEY_F3}, {"f4", KEY_F4},
	{"f5", KEY_F5}, {"f6", KEY_F6}, {"f7", KEY_F7}, {"f8", KEY_F8},
	{"f9", KEY_F9}, {"f10", KEY_F10}, {"f11", KEY_F11}, {"f12", KEY_F12},
	{"f13", KEY_F13}, {"f14", KEY_F14}, {"f15", KEY_F15}, {"f16", KEY_F16},
	{"f17", KEY_F17}, {"f18", KEY_F18}, {"f19", KEY_F19}, {"f20", KEY_F20},
	{"f21", KEY_F21}, {"f22", KEY_F22}, {"f23", KEY_F23}, {"f24", KEY_F24},
	{"kp0", KEY_KP0}, {"kp1", KEY_KP1}, {"kp2", KEY_KP2}, {"kp3", KEY_KP3},
	{"kp4", KEY_KP4}, {"kp5", KEY_KP5}, {"kp6", KEY_KP6}, {"kp7", KEY_KP7},
	{"kp8", KEY_KP8}, {"kp9", KEY_KP9},
	{"kpenter", KEY_KPENTER},
	{"kpplus", KEY_KPPLUS},
	{"kpminus", KEY_KPMINUS},
	{"kpasterisk", KEY_KPASTERISK},
	{"kpslash", KEY_KPSLASH},
	{"kpdot", KEY_KPDOT},
	{"volumeup", KEY_VOLUMEUP},
	{"volumedown", KEY_VOLUMEDOWN},
	{"mute", KEY_MUTE},
	{"playpause", KEY_PLAYPAUSE},
	{"nextsong", KEY_NEXTSONG},
	{"previoussong", KEY_PREVIOUSSONG},
	{"stopcd", KEY_STOPCD},
};

int keyname_lookup(const char *name) {
	char buf[32];
	size_t len = strlen(name);

	if (len >= sizeof(buf))
		return -1;

	for (size_t i = 0; i <= len; i++)
		buf[i] = (char)tolower((unsigned char)name[i]);

	const char *p = buf;
	if (strncmp(p, "key_", 4) == 0)
		p += 4;

	for (size_t i = 0; i < sizeof(keynames) / sizeof(keynames[0]); i++) {
		if (strcmp(keynames[i].name, p) == 0)
			return keynames[i].code;
	}

	return -1;
}
