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

#include "typesafe.h"

#define JSMN_STATIC
#include "jsmn.h"

#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION "unknown"
#endif

#define TS_MAX_TOKENS 2048
#define TS_MAX_ATTEMPTS 3

struct resp_buf {
	char *data;
	size_t len;
	size_t cap;
};

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
	struct resp_buf *b = userdata;
	size_t n = size * nmemb;
	size_t room = b->cap - b->len - 1;

	if (n > room)
		n = room;

	memcpy(b->data + b->len, ptr, n);
	b->len += n;
	b->data[b->len] = '\0';

	return size * nmemb;
}

int typesafe_ask(const char *endpoint, const char *body, char *resp, size_t resp_sz) {
	const char *key = getenv("TYPESAFE_API_KEY");

	if (!key || !*key) {
		fprintf(stderr, "ydotool: do: error: TYPESAFE_API_KEY is not set\n");
		return -1;
	}

	char auth[512];
	snprintf(auth, sizeof(auth), "Authorization: Bearer %s", key);

	CURL *curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "ydotool: do: error: failed to initialize HTTP client\n");
		return -1;
	}

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, auth);

	long http_code = 0;
	int rc = -1;

	for (int attempt = 0; attempt < TS_MAX_ATTEMPTS; attempt++) {
		struct resp_buf b = { resp, 0, resp_sz };
		resp[0] = '\0';

		curl_easy_setopt(curl, CURLOPT_URL, endpoint);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "ydotool/" VERSION);

		CURLcode crc = curl_easy_perform(curl);

		if (crc != CURLE_OK) {
			fprintf(stderr, "ydotool: do: error: request failed: %s\n",
				curl_easy_strerror(crc));
			break;
		}

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		if (http_code != 429 && http_code != 529)
			break;

		if (attempt + 1 < TS_MAX_ATTEMPTS) {
			fprintf(stderr, "ydotool: do: warning: TypeSafe busy (HTTP %ld), retrying\n",
				http_code);
			sleep(1 << attempt);
		}
	}

	if (http_code == 200) {
		rc = 0;
	} else if (http_code != 0) {
		fprintf(stderr, "ydotool: do: error: TypeSafe returned HTTP %ld\n%s\n",
			http_code, resp);
	}

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	return rc;
}

int typesafe_json_escape(const char *in, char *out, size_t out_sz) {
	size_t o = 0;

	for (const unsigned char *p = (const unsigned char *)in; *p; p++) {
		char tmp[8];
		const char *rep;
		size_t len;

		switch (*p) {
		case '"':  rep = "\\\""; len = 2; break;
		case '\\': rep = "\\\\"; len = 2; break;
		case '\n': rep = "\\n";  len = 2; break;
		case '\r': rep = "\\r";  len = 2; break;
		case '\t': rep = "\\t";  len = 2; break;
		default:
			if (*p < 0x20) {
				snprintf(tmp, sizeof(tmp), "\\u%04x", *p);
				rep = tmp;
				len = strlen(tmp);
			} else {
				tmp[0] = (char)*p;
				tmp[1] = '\0';
				rep = tmp;
				len = 1;
			}
		}

		if (o + len + 1 > out_sz)
			return -1;

		memcpy(out + o, rep, len);
		o += len;
	}

	out[o] = '\0';
	return 0;
}

static int tok_skip(jsmntok_t *t, int ntok, int i) {
	int j = i + 1;

	if (i < 0 || i >= ntok)
		return ntok;

	if (t[i].type == JSMN_OBJECT) {
		for (int k = 0; k < t[i].size; k++) {
			j = tok_skip(t, ntok, j);
			j = tok_skip(t, ntok, j);
		}
	} else if (t[i].type == JSMN_ARRAY) {
		for (int k = 0; k < t[i].size; k++)
			j = tok_skip(t, ntok, j);
	}

	return j;
}

static int tok_find(jsmntok_t *t, int ntok, int obj, const char *js, const char *key) {
	if (obj < 0 || obj >= ntok || t[obj].type != JSMN_OBJECT)
		return -1;

	int i = obj + 1;
	size_t key_len = strlen(key);

	for (int k = 0; k < t[obj].size; k++) {
		int keytok = i;
		int valtok = tok_skip(t, ntok, keytok);

		if (keytok >= ntok || valtok > ntok)
			return -1;

		if (t[keytok].type == JSMN_STRING &&
		    (size_t)(t[keytok].end - t[keytok].start) == key_len &&
		    memcmp(js + t[keytok].start, key, key_len) == 0)
			return valtok;

		i = tok_skip(t, ntok, valtok);
	}

	return -1;
}

static int tok_str(jsmntok_t *t, const char *js, int i, char *out, size_t out_sz) {
	size_t len;

	if (i < 0 || (t[i].type != JSMN_STRING && t[i].type != JSMN_PRIMITIVE))
		return -1;

	len = (size_t)(t[i].end - t[i].start);

	if (len + 1 > out_sz)
		return -1;

	memcpy(out, js + t[i].start, len);
	out[len] = '\0';

	return 0;
}

static int parse_answers(const char *json, jsmntok_t *toks, int *ntok_out) {
	jsmn_parser p;
	size_t len = strlen(json);

	jsmn_init(&p);
	int ntok = jsmn_parse(&p, json, len, toks, TS_MAX_TOKENS);

	if (ntok <= 0)
		return -1;

	*ntok_out = ntok;
	return 0;
}

int typesafe_choice(const char *json, const char *id, char *out, size_t out_sz,
		    double *confidence) {
	jsmntok_t toks[TS_MAX_TOKENS];
	int ntok;

	if (parse_answers(json, toks, &ntok) < 0)
		return -1;

	int answers = tok_find(toks, ntok, 0, json, "answers");
	if (answers < 0)
		return -1;

	int ans = tok_find(toks, ntok, answers, json, id);
	if (ans < 0)
		return -1;

	if (tok_str(toks, json, tok_find(toks, ntok, ans, json, "choice"), out, out_sz) < 0)
		return -1;

	if (confidence) {
		char buf[64];
		int c = tok_find(toks, ntok, ans, json, "confidence");

		if (tok_str(toks, json, c, buf, sizeof(buf)) == 0)
			*confidence = strtod(buf, NULL);
		else
			*confidence = -1.0;
	}

	return 0;
}

int typesafe_noul(const char *json, const char *id, double *noul) {
	jsmntok_t toks[TS_MAX_TOKENS];
	int ntok;

	if (parse_answers(json, toks, &ntok) < 0)
		return -1;

	int answers = tok_find(toks, ntok, 0, json, "answers");
	if (answers < 0)
		return -1;

	int ans = tok_find(toks, ntok, answers, json, id);
	if (ans < 0)
		return -1;

	char buf[64];
	int v = tok_find(toks, ntok, ans, json, "noul");

	if (tok_str(toks, json, v, buf, sizeof(buf)) < 0)
		return -1;

	*noul = strtod(buf, NULL);
	return 0;
}
