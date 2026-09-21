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
#include "keymap.h"

#include <string.h>

static int opt_key_delay_ms = 20;
static int opt_key_hold_ms = 20;
static int opt_next_delay_ms = 0;

struct type_parser {
	int esc;		/* 0 normal, 1 after '\', 2 first hex digit, 3 second hex digit */
	char hex[2];
	uint32_t u8_cp;
	int u8_need;
	int key_delay_ms;
	int key_hold_ms;
};

static void show_help() {
	puts(
		"Usage: type [OPTION]... [STRINGS]...\n"
		"Type strings.\n"
		"\n"
		"Options:");

	printf(
		"  -d, --key-delay=N          Delay N milliseconds between keys (the delay between every key down/up pair) (default: %d)\n", opt_key_delay_ms
	);

	printf(
		"  -H, --key-hold=N           Hold each key for N milliseconds (the delay between key down and up) (default: %d)\n", opt_key_hold_ms
	);

	printf(
		"  -D, --next-delay=N         Delay N milliseconds between command line strings (default: %d)\n", opt_next_delay_ms
	);

	printf(
		"  -l, --layout=NAME          Keyboard layout to type with (default: us)\n"
		"                               Available layouts: "
	);

	keymap_print_list(stdout);

	puts(
		"\n"
		"  -f, --file=PATH            Specify a file, the contents of which will be be typed as if passed as an argument.\n"
		"                               The filepath may also be '-' to read from stdin\n"
		"  -e, --escape=BOOL          Escape enable (1) or disable (0)\n"
		"  -h, --help                 Display this help and exit\n"
		"\n"
		"Escape is enabled by default when typing command line arguments, and disabled by default when typing from file and stdin.\n"
		"Recognized escapes: \\n, \\t, \\\\ and \\xHH. Unknown escapes are an error.\n"
		"Input is decoded as UTF-8. Characters the selected layout cannot produce without dead keys are skipped with a warning."
	);
}

static void type_parser_init(struct type_parser *p, int key_delay_ms, int key_hold_ms) {
	memset(p, 0, sizeof(*p));
	p->key_delay_ms = key_delay_ms;
	p->key_hold_ms = key_hold_ms;
}

static void type_codepoint(struct type_parser *p, const struct keymap *km, uint32_t cp) {
	struct keydef kd;

	if (cp == '\n') {
		kd.code = KEY_ENTER;
		kd.mods = 0;
	} else if (cp == '\t') {
		kd.code = KEY_TAB;
		kd.mods = 0;
	} else if (!keymap_lookup(km, cp, &kd)) {
		fprintf(stderr, "ydotool: type: warning: U+%04X is not available on layout '%s'\n",
			cp, km->name);
		return;
	}

	if (kd.mods & KM_SHIFT)
		uinput_emit(EV_KEY, KEY_LEFTSHIFT, 1, 1);
	if (kd.mods & KM_ALTGR)
		uinput_emit(EV_KEY, KEY_RIGHTALT, 1, 1);

	uinput_emit(EV_KEY, kd.code, 1, 1);

	usleep(p->key_hold_ms * 1000);

	uinput_emit(EV_KEY, kd.code, 0, 1);

	if (kd.mods & KM_ALTGR)
		uinput_emit(EV_KEY, KEY_RIGHTALT, 0, 1);
	if (kd.mods & KM_SHIFT)
		uinput_emit(EV_KEY, KEY_LEFTSHIFT, 0, 1);

	usleep(p->key_delay_ms * 1000);
}

static int hexval(unsigned char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

/* Returns the codepoint, -1 when more bytes are needed, or -2 on invalid input. */
static int utf8_feed(struct type_parser *p, unsigned char b) {
	if (p->u8_need == 0) {
		if (b < 0x80)
			return (int)b;
		if ((b & 0xE0) == 0xC0) {
			p->u8_cp = b & 0x1F;
			p->u8_need = 1;
		} else if ((b & 0xF0) == 0xE0) {
			p->u8_cp = b & 0x0F;
			p->u8_need = 2;
		} else if ((b & 0xF8) == 0xF0) {
			p->u8_cp = b & 0x07;
			p->u8_need = 3;
		} else {
			return -2;
		}
		return -1;
	}

	if ((b & 0xC0) != 0x80) {
		p->u8_need = 0;
		return -2;
	}

	p->u8_cp = (p->u8_cp << 6) | (b & 0x3F);

	if (--p->u8_need == 0)
		return (int)p->u8_cp;

	return -1;
}

/* Feed one input byte. Returns 0 on success, -1 on a malformed escape. */
static int type_parser_feed(struct type_parser *p, const struct keymap *km,
			    unsigned char b, bool escape) {
	if (escape) {
		switch (p->esc) {
		case 0:
			if (b == '\\') {
				p->esc = 1;
				return 0;
			}
			break;
		case 1:
			p->esc = 0;
			switch (b) {
			case 'n':
				type_codepoint(p, km, '\n');
				return 0;
			case 't':
				type_codepoint(p, km, '\t');
				return 0;
			case '\\':
				type_codepoint(p, km, '\\');
				return 0;
			case 'x':
				p->esc = 2;
				return 0;
			default:
				fprintf(stderr, "ydotool: type: error: unknown escape sequence '\\%c'\n", b);
				return -1;
			}
		case 2:
			if (hexval(b) < 0) {
				fprintf(stderr, "ydotool: type: error: '\\x' must be followed by two hex digits\n");
				return -1;
			}
			p->hex[0] = (char)b;
			p->esc = 3;
			return 0;
		case 3:
			if (hexval(b) < 0) {
				fprintf(stderr, "ydotool: type: error: '\\x' must be followed by two hex digits\n");
				return -1;
			}
			p->hex[1] = (char)b;
			p->esc = 0;
			type_codepoint(p, km, (uint32_t)strtol(p->hex, NULL, 16));
			return 0;
		}
	}

	int cp = utf8_feed(p, b);

	if (cp == -2) {
		fprintf(stderr, "ydotool: type: warning: invalid UTF-8 byte 0x%02x skipped\n", b);
		return 0;
	}
	if (cp < 0)
		return 0;

	type_codepoint(p, km, (uint32_t)cp);
	return 0;
}

int type_string(const char *s, bool escape, const struct keymap *km,
		int key_delay_ms, int key_hold_ms) {
	struct type_parser p;

	type_parser_init(&p, key_delay_ms, key_hold_ms);

	for (const unsigned char *q = (const unsigned char *)s; *q; q++) {
		if (type_parser_feed(&p, km, *q, escape) < 0)
			return -1;
	}

	if (p.esc != 0) {
		fprintf(stderr, "ydotool: type: error: incomplete escape sequence at end of input\n");
		return -1;
	}

	return 0;
}

int tool_type(int argc, char **argv) {
	if (argc < 2) {
		show_help();
		return 0;
	}

	const char *file_path = NULL;
	const char *layout_name = "us";

	int enable_escape = -1;

	while (1) {
		int c;

		static struct option long_options[] = {
			{"key-delay", required_argument, 0, 'd'},
			{"next-delay", required_argument, 0, 'D'},
			{"key-hold", required_argument, 0, 'H'},
			{"layout", required_argument, 0, 'l'},
			{"escape", required_argument, 0, 'e'},
			{"file", required_argument, 0, 'f'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hd:D:H:l:f:e:",
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
			case 'd':
				opt_key_delay_ms = strtol(optarg, NULL, 10);
				break;

			case 'D':
				opt_next_delay_ms = strtol(optarg, NULL, 10);
				break;

			case 'H':
				opt_key_hold_ms = strtol(optarg, NULL, 10);
				break;

			case 'l':
				layout_name = optarg;
				break;

			case 'f':
				file_path = optarg;
				break;

			case 'h':
				show_help();
				exit(0);
				break;

			case 'e':
				enable_escape = strtol(optarg, NULL, 10);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				break;

			default:
				abort();
		}
	}

	const struct keymap *km = keymap_get(layout_name);
	if (!km) {
		fprintf(stderr, "ydotool: type: error: unknown layout '%s'. Available layouts: ", layout_name);
		keymap_print_list(stderr);
		fprintf(stderr, "\n");
		return 2;
	}

	if (file_path) {
		if (enable_escape == -1) {
			enable_escape = 0;
		}

		int fd = (strcmp(file_path, "-") == 0)
			 ? STDIN_FILENO
			 : open(file_path, O_RDONLY);

		if (fd == -1) {
			fprintf(stderr, "ydotool: type: error: failed to open %s: %s\n", file_path,
				strerror(errno));
			return 2;
		}

		char buf[128];
		struct type_parser p;

		type_parser_init(&p, opt_key_delay_ms, opt_key_hold_ms);

		ssize_t rc;
		while ((rc = read(fd, buf, sizeof(buf)))) {
			if (rc > 0) {
				for (ssize_t i = 0; i<rc; i++) {
					if (type_parser_feed(&p, km, (unsigned char)buf[i], enable_escape) < 0)
						return 2;
				}
			} else if (rc < 0) {
				fprintf(stderr, "ydotool: type: error: read %s failed: %s\n", file_path, strerror(errno));
				return 2;
			}
		}

		if (p.esc != 0) {
			fprintf(stderr, "ydotool: type: error: incomplete escape sequence at end of input\n");
			return 2;
		}
	} else {
		if (enable_escape == -1) {
			enable_escape = 1;
		}

		if (optind < argc) {
			while (optind < argc) {
				if (type_string(argv[optind], enable_escape, km,
						opt_key_delay_ms, opt_key_hold_ms) < 0)
					return 2;

				optind++;

				if (argv[optind])
					usleep(opt_next_delay_ms * 1000);
			}
		} else {
			show_help();
		}

	}

	return 0;
}
