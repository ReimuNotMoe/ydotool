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

struct window_info {
	char app[128];
	char title[256];
	int x, y, w, h;
	int active;
};

/*
    Perception backend seam. The GNOME backend uses AT-SPI for windows and
    focus and the xdg-desktop-portal for screenshots. Other compositors can
    be added by implementing this interface; ydotool mcp degrades to
    actions-only when a backend cannot provide perception.
*/
struct perception_ops {
	const char *name;
	int (*init)(void);
	/* Returns the number of windows written, or 0 when unavailable. */
	size_t (*scan)(struct window_info *wins, size_t max);
	/* Returns 0 and fills out when the active window is known. */
	int (*active_window)(struct window_info *out);
	/* Blocks until the active window changes; 0 on change, -1 on timeout. */
	int (*wait_active_change)(int timeout_ms, struct window_info *out);
	/* Captures the full screen; caller frees *data with free(). */
	int (*screenshot)(char **data, size_t *len, char *err, size_t err_sz);
};

extern const struct perception_ops perception_gnome;

/* Seconds to wait for the desktop screenshot permission dialog. */
void perception_gnome_set_screenshot_timeout(int seconds);