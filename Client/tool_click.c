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

static void show_help() {
	puts(
		"Usage: click [OPTION]... [BUTTONS]...\n"
		"Click mouse buttons.\n"
		"\n"
		"Options:\n"
		"  -r, --repeat=N             Repeat entire sequence N times\n"
		"  -D, --next-delay=N         Delay N milliseconds between input events (up/down, "
		"                               a complete click means doubled time)\n"
		"  -h, --help                 Display this help and exit\n"
		"\n"
		"How to specify buttons:\n"
		"  Now all mouse buttons are represented using hexadecimal numeric values, with an optional\n"
		"bit mask to specify if mouse up/down needs to be omitted.\n"
		"  0x00 - LEFT\n"
		"  0x01 - RIGHT\n"
		"  0x02 - MIDDLE\n"
		"  0x03 - SIDE\n"
		"  0x04 - EXTR\n"
		"  0x05 - FORWARD\n"
		"  0x06 - BACK\n"
		"  0x07 - TASK\n"
		"  0x40 - Mouse down\n"
		"  0x80 - Mouse up\n"
		"  Examples:\n"
		"    0x00: chooses left button, but does nothing (you can use this to implement extra sleeps)\n"
		"    0xC0: left button click (down then up)\n"
		"    0x41: right button down\n"
		"    0x82: middle button up\n"
		"  The '0x' prefix can be omitted if you want.\n"
	);
}

int tool_click(int argc, char **argv) {
	if (argc < 2) {
		show_help();
		return 0;
	}

	int repeats = 1;
	int next_delay_ms = 25;

	while (1) {
		int c;

		static struct option long_options[] = {
			{"repeat", required_argument, 0, 'r'},
			{"next-delay", required_argument, 0, 'D'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hr:D:",
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
			case 'r':
				repeats = strtol(optarg, NULL, 10);
				break;

			case 'D':
				next_delay_ms = strtol(optarg, NULL, 10);
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
		int optind_save = optind;

		for (int i=0; i<repeats; i++) {
			optind = optind_save;
			while (optind < argc) {
				int key = strtol(argv[optind++], NULL, 16);
				int keycode = (key & 0xf) | 0x110;

				if (key & 0x40) {
					uinput_emit(EV_KEY, keycode, 1, 1);
					usleep(next_delay_ms * 1000);
				}

				if (key & 0x80) {
					uinput_emit(EV_KEY, keycode, 0, 1);
					usleep(next_delay_ms * 1000);
				}

				if ((key & 0xc0) == 0) {
					usleep(next_delay_ms * 1000);
				}

				printf("%x %x\n", key, keycode);
			}
		}
	} else {
		show_help();
	}

	return 0;
}
