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
#include "type_text.h"
#include <string.h>

static int opt_key_delay_ms = 20;
static int opt_key_hold_ms = 20;
static int opt_next_delay_ms = 0;

enum {
	OPT_XKB_RULES = 256,
	OPT_XKB_MODEL,
	OPT_XKB_LAYOUT,
	OPT_XKB_VARIANT,
	OPT_XKB_OPTIONS
};

struct escape_state {
	int state;
	char hex_str[3];
};

struct type_state {
	struct ydotool_type_resolver resolver;
	struct ydotool_type_utf8_decoder decoder;
	struct ydotool_type_options options;
};

static void show_help() {
	puts(
		"Usage: type [OPTION]... [STRINGS]...\n"
		"Type UTF-8 strings.\n"
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

	puts(
		"  -f, --file=PATH            Specify a file, the contents of which will be be typed as if passed as an argument.\n"
		"                               The filepath may also be '-' to read from stdin\n"
		"  -e, --escape=BOOL          Escape enable (1) or disable (0)\n"
		"      --xkb-rules=RULES      XKB rules to use when resolving text\n"
		"      --xkb-model=MODEL      XKB model to use when resolving text\n"
		"      --xkb-layout=LAYOUT    XKB layout to use when resolving text\n"
		"      --xkb-variant=VARIANT  XKB variant to use when resolving text\n"
		"      --xkb-options=OPTIONS  XKB options to use when resolving text\n"
		"  -h, --help                 Display this help and exit\n"
		"\n"
		"Escape is enabled by default when typing command line arguments, and disabled by default when typing from file and stdin.\n"
		"Unicode characters outside ASCII are resolved through libxkbcommon using the XKB_DEFAULT_* keymap settings.\n"
		"When any --xkb-* option is set, all characters, including ASCII, are resolved through XKB."
	);
}

static void type_emit(uint16_t code, int32_t value, void *userdata) {
	(void)userdata;
	uinput_emit(EV_KEY, code, value, 1);
}

static int escape(struct escape_state *escape_state, unsigned char in) {
	switch (escape_state->state) {
		case 0:
			if (in == '\\') {
				escape_state->state = 1;
				return -1;
			} else {
				return in;
			}
		case 1:
			escape_state->state = 0;
			switch (in) {
				case 'n':
					return '\n';
				case 't':
					return '\t';
				case 'x':
					escape_state->state = 2;
					return -1;
				case '\\':
					return '\\';
				default:
					return -1;
			}
		case 2:
			escape_state->state = 3;
			escape_state->hex_str[0] = in;
			return -1;
		case 3:
			escape_state->state = 0;
			escape_state->hex_str[1] = in;
			return (int)strtol(escape_state->hex_str, NULL, 16);
		default:
			abort();
	}
}

static void print_type_error(int rc, uint32_t codepoint) {
	switch (rc) {
		case YDOTOOL_TYPE_INVALID_UTF8:
			fprintf(stderr, "ydotool: type: error: invalid UTF-8 input\n");
			break;
		case YDOTOOL_TYPE_TRUNCATED_UTF8:
			fprintf(stderr, "ydotool: type: error: truncated UTF-8 input\n");
			break;
		case YDOTOOL_TYPE_NOT_TYPEABLE:
			fprintf(stderr, "ydotool: type: error: U+%04X is not typeable with the current XKB keymap\n", codepoint);
			break;
		case YDOTOOL_TYPE_UNSUPPORTED_MODIFIERS:
			fprintf(stderr, "ydotool: type: error: U+%04X requires modifiers that ydotool cannot emit\n", codepoint);
			break;
		case YDOTOOL_TYPE_KEYMAP_ERROR:
			fprintf(stderr, "ydotool: type: error: failed to initialize XKB keymap\n");
			break;
		default:
			fprintf(stderr, "ydotool: type: error: failed to type U+%04X\n", codepoint);
			break;
	}
}

static int type_byte(struct type_state *state, unsigned char byte, bool delay) {
	uint32_t codepoint = 0;
	bool complete = false;
	int rc = ydotool_type_utf8_feed(&state->decoder, byte, &codepoint, &complete);

	if (rc != YDOTOOL_TYPE_OK) {
		print_type_error(rc, 0);
		return 2;
	}

	if (!complete) {
		return 0;
	}

	rc = ydotool_type_emit_codepoint(&state->resolver, codepoint, &state->options,
					 delay, type_emit, NULL);
	if (rc != YDOTOOL_TYPE_OK) {
		print_type_error(rc, codepoint);
		return 2;
	}

	return 0;
}

static int type_finish(struct type_state *state) {
	int rc = ydotool_type_utf8_finish(&state->decoder);
	if (rc != YDOTOOL_TYPE_OK) {
		print_type_error(rc, 0);
		return 2;
	}

	return 0;
}

int tool_type(int argc, char **argv) {
	if (argc < 2) {
		show_help();
		return 0;
	}



	const char *file_path = NULL;

	int enable_escape = -1;
	bool force_xkb = false;
	struct ydotool_xkb_names xkb_names = {0};
	struct type_state type_state;
	ydotool_type_resolver_init(&type_state.resolver);
	ydotool_type_utf8_init(&type_state.decoder);

	while (1) {
		int c;

		static struct option long_options[] = {
			{"key-delay", required_argument, 0, 'd'},
			{"next-delay", required_argument, 0, 'D'},
			{"key-hold", required_argument, 0, 'H'},
			{"escape", required_argument, 0, 'e'},
			{"file", required_argument, 0, 'f'},
			{"help", no_argument, 0, 'h'},
			{"xkb-rules", required_argument, 0, OPT_XKB_RULES},
			{"xkb-model", required_argument, 0, OPT_XKB_MODEL},
			{"xkb-layout", required_argument, 0, OPT_XKB_LAYOUT},
			{"xkb-variant", required_argument, 0, OPT_XKB_VARIANT},
			{"xkb-options", required_argument, 0, OPT_XKB_OPTIONS},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hd:D:H:f:e:",
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

			case OPT_XKB_RULES:
				xkb_names.rules = optarg;
				force_xkb = true;
				break;

			case OPT_XKB_MODEL:
				xkb_names.model = optarg;
				force_xkb = true;
				break;

			case OPT_XKB_LAYOUT:
				xkb_names.layout = optarg;
				force_xkb = true;
				break;

			case OPT_XKB_VARIANT:
				xkb_names.variant = optarg;
				force_xkb = true;
				break;

			case OPT_XKB_OPTIONS:
				xkb_names.options = optarg;
				force_xkb = true;
				break;

			case '?':
				/* getopt_long already printed an error message. */
				break;

			default:
				abort();
		}
	}

	type_state.options.key_hold_ms = opt_key_hold_ms;
	type_state.options.key_delay_ms = opt_key_delay_ms;
	ydotool_type_resolver_set_xkb(&type_state.resolver, &xkb_names, force_xkb);

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
			ydotool_type_resolver_destroy(&type_state.resolver);
			return 2;
		}

		char buf[128];
		struct escape_state escape_state = {0, {0, 0, 0}};

		ssize_t rc;
		while ((rc = read(fd, buf, sizeof(buf)))) {
			if (rc > 0) {
				for (int i = 0; i<rc; i++) {
					int c = enable_escape ? escape(&escape_state, (unsigned char)buf[i]) : (unsigned char)buf[i];
					if (c != -1) {
						int trc = type_byte(&type_state, (unsigned char)c, true);
						if (trc) {
							ydotool_type_resolver_destroy(&type_state.resolver);
							return trc;
						}
					}
				}
			} else if (rc < 0) {
				fprintf(stderr, "ydotool: type: error: read %s failed: %s\n", file_path, strerror(errno));
				ydotool_type_resolver_destroy(&type_state.resolver);
				return 2;
			}
		}

		int trc = type_finish(&type_state);
		ydotool_type_resolver_destroy(&type_state.resolver);
		if (trc) {
			return trc;
		}
	} else {
		if (enable_escape == -1) {
			enable_escape = 1;
		}

		if (optind < argc) {
			while (optind < argc) {
				char *pstr = argv[optind++];
				struct escape_state escape_state = {0, {0, 0, 0}};

//				printf("pstr: %s\n", pstr);

				for (int i = 0; ; i++) {
					int c = enable_escape ? escape(&escape_state, (unsigned char)pstr[i]) : (unsigned char)pstr[i];
					char next = pstr[i+1];

					if (c == 0) {
						break;
					} else if (c != -1) {
						int trc = type_byte(&type_state, (unsigned char)c, next);
						if (trc) {
							ydotool_type_resolver_destroy(&type_state.resolver);
							return trc;
						}
					}
				}

				int trc = type_finish(&type_state);
				if (trc) {
					ydotool_type_resolver_destroy(&type_state.resolver);
					return trc;
				}

				if (argv[optind])
					usleep(opt_next_delay_ms * 1000);
			}
		} else {
			show_help();
		}

	}

	ydotool_type_resolver_destroy(&type_state.resolver);
	return 0;
}
