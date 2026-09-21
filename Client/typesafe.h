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

#include <stddef.h>

/*
    Minimal TypeSafe System One client.

    The API key is read from the TYPESAFE_API_KEY environment variable and is
    never logged. See https://docs.typesafe.ai/api for the API contract.
*/

/* POST a request body to the System One endpoint. Returns 0 on HTTP 200. */
int typesafe_ask(const char *endpoint, const char *body, char *resp, size_t resp_sz);

/* Escape a UTF-8 string for embedding in a JSON string literal. */
int typesafe_json_escape(const char *in, char *out, size_t out_sz);

/* Read a Choice answer. confidence may be NULL. Returns 0 when found. */
int typesafe_choice(const char *json, const char *id, char *out, size_t out_sz,
		    double *confidence);

/* Read a Noul answer. Returns 0 when found. */
int typesafe_noul(const char *json, const char *id, double *noul);
