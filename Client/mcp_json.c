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

#include "jsmn.h"
#include "mcp_json.h"

#include <stdlib.h>
#include <string.h>

int mj_parse(const char *json, jsmntok_t *toks, int max) {
	jsmn_parser p;

	jsmn_init(&p);
	return jsmn_parse(&p, json, strlen(json), toks, max);
}

int mj_skip(jsmntok_t *t, int ntok, int i) {
	int j = i + 1;

	if (i < 0 || i >= ntok)
		return ntok;

	if (t[i].type == JSMN_OBJECT) {
		for (int k = 0; k < t[i].size; k++) {
			j = mj_skip(t, ntok, j);
			j = mj_skip(t, ntok, j);
		}
	} else if (t[i].type == JSMN_ARRAY) {
		for (int k = 0; k < t[i].size; k++)
			j = mj_skip(t, ntok, j);
	}

	return j;
}

int mj_get(jsmntok_t *t, int ntok, const char *json, int obj, const char *key) {
	if (obj < 0 || obj >= ntok || t[obj].type != JSMN_OBJECT)
		return -1;

	int i = obj + 1;
	size_t key_len = strlen(key);

	for (int k = 0; k < t[obj].size; k++) {
		int keytok = i;
		int valtok = mj_skip(t, ntok, keytok);

		if (keytok >= ntok)
			return -1;

		if (t[keytok].type == JSMN_STRING &&
		    (size_t)(t[keytok].end - t[keytok].start) == key_len &&
		    memcmp(json + t[keytok].start, key, key_len) == 0)
			return valtok;

		i = mj_skip(t, ntok, valtok);
	}

	return -1;
}

int mj_str(jsmntok_t *t, int ntok, const char *json, int tok, char *out, size_t out_sz) {
	size_t len;

	if (tok < 0 || tok >= ntok ||
	    (t[tok].type != JSMN_STRING && t[tok].type != JSMN_PRIMITIVE))
		return -1;

	len = (size_t)(t[tok].end - t[tok].start);

	if (len + 1 > out_sz)
		return -1;

	memcpy(out, json + t[tok].start, len);
	out[len] = '\0';

	return 0;
}

int mj_int(jsmntok_t *t, int ntok, const char *json, int tok, long *out) {
	char buf[64];

	if (mj_str(t, ntok, json, tok, buf, sizeof(buf)) < 0)
		return -1;

	char *end = NULL;
	long v = strtol(buf, &end, 10);

	if (!end || *end)
		return -1;

	*out = v;
	return 0;
}

int mj_bool(jsmntok_t *t, int ntok, const char *json, int tok, bool *out) {
	char buf[16];

	if (mj_str(t, ntok, json, tok, buf, sizeof(buf)) < 0)
		return -1;

	if (strcmp(buf, "true") == 0) {
		*out = true;
		return 0;
	}
	if (strcmp(buf, "false") == 0) {
		*out = false;
		return 0;
	}

	return -1;
}

int mj_size(jsmntok_t *t, int ntok, int tok) {
	if (tok < 0 || tok >= ntok)
		return 0;
	return t[tok].size;
}