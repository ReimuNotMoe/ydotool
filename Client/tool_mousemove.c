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
		"Usage: mousemove [OPTION]... -- <xpos> <ypos>\n"
		"Move mouse pointer.\n"
		"\n"
		"Options:\n"
		"  -a, --absolute             Use absolute position\n"
		"  -h, --help                 Display this help and exit\n"
	);
}

int tool_mousemove(int argc, char **argv) {
	if (argc < 2) {
		show_help();
		return 0;
	}

	int is_abs = 0;

	while (1) {
		int c;

		static struct option long_options[] = {
			{"absolute", no_argument, 0, 'a'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "ha",
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
			case 'a':
				is_abs = 1;
				break;

			case 'h':
				show_help();
				return 0;

			case '?':
				/* getopt_long already printed an error message. */
				break;

			default:
				abort();
		}
	}

	int i = 0;
	int32_t arg[2] = {0, 0};

	if (optind < argc) {
		while (optind < argc) {
			arg[i] = strtol(argv[optind++], NULL, 10);
			i++;
		}
	}

	if (i == 2) {
		if (is_abs) {
			uinput_emit(EV_REL, REL_X, INT32_MIN, 0);
			uinput_emit(EV_REL, REL_Y, INT32_MIN, 1);
		}

		uinput_emit(EV_REL, REL_X, arg[0], 0);
		uinput_emit(EV_REL, REL_Y, arg[1], 1);
	} else {
		show_help();
		return 1;
	}

	return 0;
}
