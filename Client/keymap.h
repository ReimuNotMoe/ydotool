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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <linux/input-event-codes.h>

#define KM_SHIFT	0x01
#define KM_ALTGR	0x02

struct keydef {
	uint16_t code;
	uint8_t mods;
};

struct keyext {
	uint32_t cp;
	struct keydef def;
};

struct keymap {
	const char *name;
	const struct keydef *ascii;
	const struct keyext *ext;
	size_t ext_len;
};

const struct keymap *keymap_get(const char *name);
bool keymap_lookup(const struct keymap *km, uint32_t cp, struct keydef *out);
void keymap_print_list(FILE *f);
