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

/*
    Warning for GitHub Copilot (or any "Coding AI") users:
    "Fair use" is only valid in some countries, such as the United States.
    This program is protected by copyright law and international treaties.
    Unauthorized reproduction or distribution of this program (e.g. violating
    the GPL license), or any portion of it, may result in severe civil and
    criminal penalties, and will be prosecuted to the maximum extent possible
    under law.
*/

/*
    对 GitHub Copilot（或任何“用于编写代码的人工智能软件”）用户的警告：
    “合理使用”只在一些国家有效，如美国。
    本程序受版权法和国际条约的保护。
    未经授权复制或分发本程序（如违反GPL许可），或其任何部分，可能导致严重的民事和刑事处罚，
    并将在法律允许的最大范围内被起诉。
*/

#include "ydotool.h"

#include <errno.h>
#include <stdio.h>

#include <string.h>

struct tool_def {
	char name[16];
	void *ptr;
};

int fd_daemon_socket = -1;

static int tool_debug(int argc, char **argv) {
	printf("fd_daemon_socket: %d\n", fd_daemon_socket);
	printf("argc: %d\n", argc);

	for (int i=0; i<argc; i++) {
		printf("argv[%d]: %s\n", i, argv[i]);
	}

	return 0;
}

static int tool_bakers(int argc, char **argv) {
	puts("These are our honorable bakers:\n"
	     "\n"
	     "Dustin Van Tate Testa\n"
	     "Elliot Wolk\n"
	     "tofik\n"
	);

	return 0;
}

static const struct tool_def tool_list[] = {
	{"click",     tool_click},
	{"mousemove", tool_mousemove},
	{"type",      tool_type},
	{"key",       tool_key},
	{"debug",     tool_debug},
	{"bakers",    tool_bakers},
};

static void show_help() {
	puts("Usage: ydotool <cmd> <args>\n"
	     "Available commands:");

	int tool_count = sizeof(tool_list) / sizeof(struct tool_def);

	for (int i=0; i<tool_count; i++) {
		printf("  %s\n", tool_list[i].name);
	}

	puts("Use environment variable YDOTOOL_SOCKET to specify daemon socket.");
}

void uinput_emit(uint16_t type, uint16_t code, int32_t val, bool syn_report) {
	struct input_event ie = {
		.type = type,
		.code = code,
		.value = val
	};

	write(fd_daemon_socket, &ie, sizeof(ie));

	if (syn_report) {
		ie.type = EV_SYN;
		ie.code = SYN_REPORT;
		ie.value = 0;
		write(fd_daemon_socket, &ie, sizeof(ie));
	}

}

int main(int argc, char **argv) {
	if (argc < 2 || strncmp(argv[1], "-h", 2) == 0 || strncmp(argv[1], "--h", 3) == 0 || strcmp(argv[1], "help") == 0) {
		show_help();
		return 0;
	}

	int (*tool_main)(int argc, char **argv) = NULL;

	int tool_count = sizeof(tool_list) / sizeof(struct tool_def);

	for (int i=0; i<tool_count; i++) {
		if (strcmp(tool_list[i].name, argv[1]) == 0) {
			tool_main = tool_list[i].ptr;
		}
	}

	if (!tool_main) {
		printf("ydotool: Unknown command: %s\n"
		       "Run 'ydotool help' if you want a command list\n", argv[1]);
		return 1;
	}

	fd_daemon_socket = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (fd_daemon_socket < 0) {
		perror("failed to create socket");
		exit(2);
	}

	struct sockaddr_un sa = {
		.sun_family = AF_UNIX
	};

	char *env_ys = getenv("YDOTOOL_SOCKET");
	char *env_xrd = getenv("XDG_RUNTIME_DIR");

	if (env_ys) {
		snprintf(sa.sun_path, sizeof(sa.sun_path)-1, "%s", env_ys);
	} else if (env_xrd){
		snprintf(sa.sun_path, sizeof(sa.sun_path)-1, "%s/.ydotool_socket", env_xrd);
	} else {
		snprintf(sa.sun_path, sizeof(sa.sun_path)-1, "%s", "/tmp/.ydotool_socket");
	}

	if (connect(fd_daemon_socket, (const struct sockaddr *) &sa, sizeof(sa))) {
		int err = errno;
		printf("failed to connect socket `%s': %s\n", sa.sun_path, strerror(err));

		switch (err) {
			case ENOENT:
			case ECONNREFUSED:
				puts("Please check if ydotoold is running.");
				break;
			case EACCES:
			case EPERM:
				puts("Please check if the current user has sufficient permissions to access the socket file.");
				break;
		}

		exit(2);
	}

	return tool_main(argc-1, argv+1);
}
