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

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <limits.h>

#include <getopt.h>

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/un.h>

#include <sys/epoll.h>

#include <linux/uinput.h>

#include "../Client/ydotool.h"

#ifndef VERSION
#define VERSION "unknown"
#endif

#define SOCKET_PATH_LEN		108

static char opt_socket_path[SOCKET_PATH_LEN] = "/tmp/.ydotool_socket";
static char opt_socket_perm[16] = "0600";
static char opt_socket_own[16] = "";

static void show_help() {
	puts(
		"Usage: ydotoold [OPTION]...\n"
		"The ydotool Daemon.\n"
		"\n"
		"Options:\n"
		"  -p, --socket-path=PATH     Custom socket path\n"
		"  -P, --socket-perm=PERM     Socket permission (default 0600)\n"
		"  -o, --socket-own=UID:GID   Socket ownership\n"
		"  -m, --mouse-off            Disable mouse (EV_REL)\n"
		"  -k, --keyboard-off         Disable keyboard (EV_KEY)\n"
		"  -T, --touch-on             Enable touchscreen (EV_ABS)\n"
		"  -h, --help                 Display this help and exit\n"
		"  -V, --version              Show version information\n"
	);
}

static void show_version() {
	puts(VERSION);
}

enum ydotool_uinput_setup_options {
	ENABLE_KEY = (1 << 0),
	ENABLE_REL = (1 << 1),
	ENABLE_ABS = (1 << 2),
};

static void uinput_setup(int fd, enum ydotool_uinput_setup_options setup_opt) {

	if (setup_opt & ENABLE_KEY) {
		if (ioctl(fd, UI_SET_EVBIT, EV_KEY)) {
			fprintf(stderr, "UI_SET_EVBIT %s failed\n", "EV_KEY");
		}

		for (size_t i=0; i<sizeof(ydotool_key_list)/sizeof(int); i++) {
			if (ioctl(fd, UI_SET_KEYBIT, ydotool_key_list[i])) {
				fprintf(stderr, "UI_SET_KEYBIT %d failed\n", i);

			}
		}
	}

	if (setup_opt & ENABLE_REL) {
		if (ioctl(fd, UI_SET_EVBIT, EV_REL)) {
			fprintf(stderr, "UI_SET_EVBIT %s failed\n", "EV_REL");
		}

		static const int rel_list[] = {REL_X, REL_Y, REL_Z, REL_WHEEL, REL_HWHEEL};

		for (int i=0; i<sizeof(rel_list)/sizeof(int); i++) {
			if (ioctl(fd, UI_SET_RELBIT, rel_list[i])) {
				fprintf(stderr, "UI_SET_RELBIT %d failed\n", i);

			}
		}
	}

	if (setup_opt & ENABLE_ABS) {
		if (ioctl(fd, UI_SET_EVBIT, EV_ABS)) {
			fprintf(stderr, "UI_SET_EVBIT %s failed\n", "EV_ABS");
		}

		static const int abs_list[] = {ABS_X, ABS_Y, ABS_MT_SLOT, ABS_MT_TRACKING_ID,
					       ABS_MT_POSITION_X, ABS_MT_POSITION_Y, ABS_PRESSURE, ABS_MT_PRESSURE};

		for (int i = 0; i < sizeof(abs_list) / sizeof(int); i++) {
			if (ioctl(fd, UI_SET_ABSBIT, abs_list[i])) {
				fprintf(stderr, "UI_SET_ABSBIT %d failed\n", i);

			}
		}
	}

	static const struct uinput_setup usetup = {
		.name = "ydotoold virtual device",
		.id = {
			.bustype = BUS_VIRTUAL,
			.vendor = 0x2333,
			.product = 0x6666,
			.version = 1
		}
	};

	if (ioctl(fd, UI_DEV_SETUP, &usetup)) {
		perror("UI_DEV_SETUP ioctl failed");
		exit(2);
	}

	if (ioctl(fd, UI_DEV_CREATE)) {
		perror("UI_DEV_CREATE ioctl failed");
		exit(2);
	}

}

int main(int argc, char **argv) {

	char *env_xrd = getenv("XDG_RUNTIME_DIR");

	if (env_xrd) {
		snprintf(opt_socket_path, SOCKET_PATH_LEN-1, "%s/.ydotool_socket", env_xrd);
	}

	enum ydotool_uinput_setup_options opt_ui_setup = ENABLE_REL | ENABLE_KEY;

	while (1) {
		int c;

		static struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"version", no_argument, 0, 'V'},
			{"socket-perm", required_argument, 0, 'P'},
			{"socket-own", required_argument, 0, 'o'},
			{"socket-path", required_argument, 0, 'p'},
			{"mouse-off", no_argument, 0, 'm'},
			{"keyboard-off", no_argument, 0, 'k'},
			{"touch-on", no_argument, 0, 'T'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hVp:P:o:mkT",
				 long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;
			case 'p':
				strncpy(opt_socket_path, optarg, sizeof(opt_socket_path)-1);
				break;

			case 'P':
				strncpy(opt_socket_perm, optarg, sizeof(opt_socket_perm)-1);
				break;

			case 'o':
				strncpy(opt_socket_own, optarg, sizeof(opt_socket_perm)-1);
				break;

			case 'm':
				opt_ui_setup &= ~ENABLE_REL;
				break;

			case 'k':
				opt_ui_setup &= ~ENABLE_KEY;
				break;

			case 'T':
				opt_ui_setup |= ENABLE_ABS;
				break;

			case 'h':
				show_help();
				exit(0);
				break;

			case 'V':
				show_version();
				exit(0);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				break;

			default:
				exit(2);
		}
	}

	if (getuid() || getegid()) {
		puts("You're advised to run this program as root, or YMMV.");
	}

	int fd_ui = open("/dev/uinput", O_WRONLY);

	if (fd_ui < 0) {
		perror("failed to open uinput device");
		exit(2);
	}

	printf("Socket path: %s\n", opt_socket_path);

	struct stat sbuf;

	if (stat(opt_socket_path, &sbuf) == 0) {

		int fd_sot = socket(AF_UNIX, SOCK_DGRAM, 0);

		if (fd_sot < 0) {
			perror("failed to create socket for daemon collision detection");
			exit(2);
		}

		struct sockaddr_un sa = {
			.sun_family = AF_UNIX
		};

		strncpy(sa.sun_path, opt_socket_path, sizeof(sa.sun_path)-1);

		if (connect(fd_sot, (const struct sockaddr *) &sa, sizeof(sa))) {
			close(fd_sot);

			puts("Removing old stale socket");

			if (unlink(opt_socket_path)) {
				perror("failed remove old stale socket");
				exit(2);
			}
		} else {
			puts("error: Another ydotoold is running with the same socket.");
			exit(2);
		}
	}

	int fd_so = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (fd_so < 0) {
		perror("failed to create socket");
		exit(2);
	}

	struct sockaddr_un sa = {
		.sun_family = AF_UNIX
	};

	strncpy(sa.sun_path, opt_socket_path, sizeof(sa.sun_path)-1);

	if (bind(fd_so, (const struct sockaddr *) &sa, sizeof(sa))) {
		perror("failed to bind socket");
		exit(2);
	}


	if (chmod(opt_socket_path, strtol(opt_socket_perm, NULL, 8))) {
		perror("failed to change socket permission");
		exit(2);
	}

	printf("Socket permission: %s\n", opt_socket_perm);

	if (opt_socket_own[0]) {
		char *gid_pos = strchr(opt_socket_own, ':');

		if (!gid_pos) {
			puts("invalid ownership specification");
			exit(2);
		}

		gid_pos++;

		uid_t uid = strtol(opt_socket_own, NULL, 10);
		gid_t gid = strtol(gid_pos, NULL, 10);

		if (chown(opt_socket_path, uid, gid)) {
			perror("failed to change socket ownership");
			exit(2);
		}

		printf("Socket ownership: UID=%d, GID=%d\n", uid, gid);
	}

	uinput_setup(fd_ui, opt_ui_setup);

	sleep(1);

	const char *xinput_path = "/usr/bin/xinput";

	if (getenv("DISPLAY")) {
		if (stat(xinput_path, &sbuf) == 0) {
			pid_t npid = vfork();

			if (npid == 0) {
				execl(xinput_path, "xinput", "--set-prop", "pointer:ydotoold virtual device", "libinput Accel Profile Enabled", "0,", "1", NULL);
				perror("failed to run xinput command");
				_exit(2);
			} else if (npid == -1) {
				perror("failed to fork");
			}
		} else {
			printf("xinput command not found in `%s', not disabling mouser pointer acceleration", xinput_path);
		}
	}

	puts("READY");

	struct input_event uev;

	while (1) {
		if (recv(fd_so, &uev, sizeof(uev), 0) == sizeof(uev)) {
			write(fd_ui, &uev, sizeof(uev));
		}
	}
}
