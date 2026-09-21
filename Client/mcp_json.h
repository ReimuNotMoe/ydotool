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

/* Pull in jsmn declarations only; mcp_json.c carries the implementation. */
#define JSMN_HEADER
#include "jsmn.h"

/* Small typed accessors over jsmn tokens for MCP JSON-RPC handling. */

int mj_parse(const char *json, jsmntok_t *toks, int max);

/* Index of the token after i, skipping nested objects and arrays. */
int mj_skip(jsmntok_t *t, int ntok, int i);

/* Value token for key in the object at obj, or -1. */
int mj_get(jsmntok_t *t, int ntok, const char *json, int obj, const char *key);

int mj_str(jsmntok_t *t, int ntok, const char *json, int tok, char *out, size_t out_sz);
int mj_int(jsmntok_t *t, int ntok, const char *json, int tok, long *out);
int mj_bool(jsmntok_t *t, int ntok, const char *json, int tok, bool *out);

/* Element count of an object or array token. */
int mj_size(jsmntok_t *t, int ntok, int tok);