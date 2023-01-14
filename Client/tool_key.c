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
#include <string.h>

static void show_help() {
	puts(
		"Usage: key [OPTION]... [KEYCODES]...\n"
		"Emit key events.\n"
		"\n"
		"Options:\n"
		"  -d, --key-delay=N          Delay N milliseconds between key events\n"
		"  -h, --help                 Display this help and exit\n"
		"\n"
		"Since there's no way to know how many keyboard layouts are there in the world,\n"
		"we're using raw keycodes now.\n"
		"\n"
		"Syntax: <keycode>:<pressed>\n"
		"e.g. 28:1 28:0 means pressing on the Enter button on a standard US keyboard.\n"
		"     (where :1 for pressed means the key is down and then :0 means the key is released)"
		"     42:1 38:1 38:0 24:1 24:0 38:1 38:0 42:0 - \"LOL\"\n"
		"\n"
		"Non-interpretable values, such as 0, aaa, l0l, will only cause a delay.\n"
		"\n"
		"See `/usr/include/linux/input-event-codes.h' for available key codes (KEY_*).\n"
	);
}


int tool_key(int argc, char **argv) {
	if (argc < 2) {
		show_help();
		return 0;
	}

	int key_delay = 12;

	while (1) {
		int c;

		static struct option long_options[] = {
			{"key-delay", required_argument, 0, 'd'},
			{"file", required_argument, 0, 'f'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hd:",
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
			case 'D':
				key_delay = strtol(optarg, NULL, 10);
				break;

			case 'd':
				key_delay = strtol(optarg, NULL, 10);
				break;

			case 'h':
				show_help();
				exit(0);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				break;

			default:
				abort();
		}
	}


	if (optind < argc) {
		while (optind < argc) {
			char *pstr = argv[optind++];
			uint16_t kc;
			if (strchr(pstr, 'x')) {
				kc = strtol(pstr, NULL, 16);
			} else {
				kc = strtol(pstr, NULL, 10);
			}

			size_t slen = strlen(pstr);
			if (slen) {
				char cen = pstr[slen-1];

				if (cen == '0') {
					uinput_emit(EV_KEY, kc, 0, 1);
				} else {
					uinput_emit(EV_KEY, kc, 1, 1);
				}
			}

			usleep(key_delay * 1000);
		}
	} else {
		show_help();
	}

	return 0;
}
