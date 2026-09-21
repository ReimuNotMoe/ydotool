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

#include "perception.h"

#include <atspi/atspi.h>
#include <gio/gio.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int screenshot_timeout_s = 60;

void perception_gnome_set_screenshot_timeout(int seconds) {
	if (seconds > 0)
		screenshot_timeout_s = seconds;
}

static struct window_info active_win;
static int active_known;
static int active_seq;

static void fill_window(AtspiAccessible *w, struct window_info *wi) {
	memset(wi, 0, sizeof(*wi));

	char *title = atspi_accessible_get_name(w, NULL);
	snprintf(wi->title, sizeof(wi->title), "%s", title ? title : "");
	g_free(title);

	AtspiAccessible *parent = atspi_accessible_get_parent(w, NULL);
	if (parent) {
		char *app = atspi_accessible_get_name(parent, NULL);
		snprintf(wi->app, sizeof(wi->app), "%s", app ? app : "?");
		g_free(app);
		g_object_unref(parent);
	} else {
		snprintf(wi->app, sizeof(wi->app), "?");
	}

	AtspiComponent *comp = atspi_accessible_get_component(w);
	if (comp) {
		AtspiRect *r = atspi_component_get_extents(comp, ATSPI_COORD_TYPE_SCREEN, NULL);
		if (r) {
			wi->x = r->x;
			wi->y = r->y;
			wi->w = r->width;
			wi->h = r->height;
			g_free(r);
		}
		g_object_unref(comp);
	}
}

static void on_state_event(AtspiEvent *event, void *user_data) {
	(void)user_data;

	if (!event || !event->source || event->detail1 != 1)
		return;

	char *role = atspi_accessible_get_role_name(event->source, NULL);
	if (!role)
		return;

	if (strcmp(role, "frame") == 0) {
		fill_window(event->source, &active_win);
		active_win.active = 1;
		active_known = 1;
		active_seq++;
	}

	g_free(role);
}

static size_t scan_impl(struct window_info *wins, size_t max) {
	AtspiAccessible *desktop = atspi_get_desktop(0);
	size_t count = 0;

	if (!desktop)
		return 0;

	int apps = atspi_accessible_get_child_count(desktop, NULL);

	for (int i = 0; i < apps && count < max; i++) {
		AtspiAccessible *app = atspi_accessible_get_child_at_index(desktop, i, NULL);
		if (!app)
			continue;

		char *app_name = atspi_accessible_get_name(app, NULL);
		int wins_n = atspi_accessible_get_child_count(app, NULL);

		for (int j = 0; j < wins_n && count < max; j++) {
			AtspiAccessible *w = atspi_accessible_get_child_at_index(app, j, NULL);
			if (!w)
				continue;

			char *role = atspi_accessible_get_role_name(w, NULL);

			if (role && strcmp(role, "frame") == 0) {
				struct window_info *wi = &wins[count];

				fill_window(w, wi);
				snprintf(wi->app, sizeof(wi->app), "%s", app_name ? app_name : "?");

				AtspiStateSet *ss = atspi_accessible_get_state_set(w);
				wi->active = ss && atspi_state_set_contains(ss, ATSPI_STATE_ACTIVE);
				count++;
			}

			g_free(role);
			g_object_unref(w);
		}

		g_free(app_name);
		g_object_unref(app);
	}

	return count;
}

static int active_impl(struct window_info *out) {
	if (active_known) {
		*out = active_win;
		return 0;
	}

	struct window_info wins[64];
	size_t n = scan_impl(wins, 64);
	size_t actives = 0;
	struct window_info found;

	for (size_t i = 0; i < n; i++) {
		if (wins[i].active) {
			found = wins[i];
			actives++;
		}
	}

	if (actives == 1) {
		active_win = found;
		active_known = 1;
		*out = found;
		return 0;
	}

	return -1;
}

static int wait_impl(int timeout_ms, struct window_info *out) {
	int start = active_seq;
	gint64 deadline = g_get_monotonic_time() + (gint64)timeout_ms * 1000;

	while (active_seq == start) {
		if (g_get_monotonic_time() >= deadline)
			return -1;

		g_main_context_iteration(NULL, FALSE);
		g_usleep(2000);
	}

	if (out)
		*out = active_win;

	return 0;
}

struct shot_ctx {
	gboolean done;
	guint response;
	char *uri;
	const char *handle;
};

static void on_portal_response(GDBusConnection *conn, const gchar *sender, const gchar *path,
			       const gchar *iface, const gchar *signal, GVariant *params,
			       gpointer user_data) {
	(void)conn; (void)sender; (void)iface; (void)signal;

	struct shot_ctx *ctx = user_data;

	if (ctx->handle && strcmp(path, ctx->handle) != 0)
		return;

	guint response = 0;
	GVariant *results = NULL;
	g_variant_get(params, "(u@a{sv})", &response, &results);

	ctx->response = response;

	if (results) {
		GVariant *uri = g_variant_lookup_value(results, "uri", G_VARIANT_TYPE_STRING);

		if (uri) {
			ctx->uri = g_variant_dup_string(uri, NULL);
			g_variant_unref(uri);
		}

		g_variant_unref(results);
	}

	ctx->done = TRUE;
}

static int screenshot_impl(char **data, size_t *len, char *err, size_t err_sz) {
	GError *gerr = NULL;
	int rc = -1;
	struct shot_ctx ctx = {0};
	GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &gerr);

	if (!conn) {
		snprintf(err, err_sz, "cannot connect to the session bus: %s",
			 gerr ? gerr->message : "unknown error");
		return -1;
	}

	guint sub = g_dbus_connection_signal_subscribe(conn,
		"org.freedesktop.portal.Desktop", "org.freedesktop.portal.Request",
		"Response", NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
		on_portal_response, &ctx, NULL);

	GVariantBuilder opts;
	g_variant_builder_init(&opts, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&opts, "{sv}", "interactive", g_variant_new_boolean(FALSE));
	g_variant_builder_add(&opts, "{sv}", "modal", g_variant_new_boolean(TRUE));

	GVariant *res = g_dbus_connection_call_sync(conn,
		"org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
		"org.freedesktop.portal.Screenshot", "Screenshot",
		g_variant_new("(sa{sv})", "", &opts), G_VARIANT_TYPE("(o)"),
		G_DBUS_CALL_FLAGS_NONE, 15000, NULL, &gerr);

	if (!res) {
		snprintf(err, err_sz, "screenshot portal unavailable: %s",
			 gerr ? gerr->message : "unknown error");
		goto out;
	}

	const char *handle = NULL;
	g_variant_get(res, "(o)", &handle);
	ctx.handle = handle;

	gint64 deadline = g_get_monotonic_time() + (gint64)screenshot_timeout_s * G_TIME_SPAN_SECOND;

	while (!ctx.done && g_get_monotonic_time() < deadline) {
		g_main_context_iteration(NULL, FALSE);
		g_usleep(2000);
	}

	if (!ctx.done) {
		snprintf(err, err_sz, "timed out waiting for the screenshot permission dialog");
	} else if (ctx.response != 0) {
		snprintf(err, err_sz, "screenshot request was denied or cancelled");
	} else if (!ctx.uri) {
		snprintf(err, err_sz, "screenshot portal returned no file");
	} else {
		char *path = g_filename_from_uri(ctx.uri, NULL, NULL);
		gchar *contents = NULL;
		gsize length = 0;

		if (!path || !g_file_get_contents(path, &contents, &length, &gerr)) {
			snprintf(err, err_sz, "cannot read the screenshot file: %s",
				 gerr ? gerr->message : "unknown error");
		} else {
			unlink(path);
			*data = contents;
			*len = (size_t)length;
			rc = 0;
		}

		g_free(path);
	}

	g_variant_unref(res);
out:
	g_dbus_connection_signal_unsubscribe(conn, sub);
	g_object_unref(conn);
	g_free(ctx.uri);

	return rc;
}

static int init_impl(void) {
	if (atspi_init() != 0) {
		fprintf(stderr, "ydotool: mcp: warning: accessibility bus unavailable, "
				"window and focus tools disabled\n");
		return -1;
	}

	AtspiEventListener *listener = atspi_event_listener_new(on_state_event, NULL, NULL);

	if (listener) {
		atspi_event_listener_register(listener, "object:state-changed:active", NULL);
		atspi_event_listener_register(listener, "object:state-changed:focused", NULL);
	}

	struct window_info wins[64];
	size_t n = scan_impl(wins, 64);
	size_t actives = 0;

	for (size_t i = 0; i < n; i++) {
		if (wins[i].active) {
			active_win = wins[i];
			actives++;
		}
	}

	if (actives == 1)
		active_known = 1;

	fprintf(stderr, "ydotool: mcp: perception: %zu windows, active %s\n",
		n, active_known ? active_win.title : "unknown");

	return 0;
}

const struct perception_ops perception_gnome = {
	.name = "gnome",
	.init = init_impl,
	.scan = scan_impl,
	.active_window = active_impl,
	.wait_active_change = wait_impl,
	.screenshot = screenshot_impl,
};