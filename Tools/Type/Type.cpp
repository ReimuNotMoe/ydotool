/*
    This file is part of ydotool.
    Copyright (C) 2018-2021 Reimu NotMoe <reimu@sudomaker.com>

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

#include "Type.hpp"

using namespace evdevPlus;

using namespace ydotool::Tools;

const char ydotool_tool_name[] = "type";

static int time_keydelay = 12;

int Type::TypeText(const std::string &text) {
	int pos = 0;

	for (auto &c : text) {

		int isUpper = 0;

		int key_code;

		auto itk = Table_LowerKeys.find(c);

		if (itk != Table_LowerKeys.end()) {
			key_code = itk->second;
		} else {
			auto itku = Table_UpperKeys.find(c);
			if (itku != Table_UpperKeys.end()) {
				isUpper = 1;
				key_code = itku->second;
			} else {
				return -(pos+1);
			}
		}

		int sleep_time;

		if (isUpper) {
			sleep_time = 250 * time_keydelay;
			uInputContext->send_key(KEY_LEFTSHIFT, 1);
			usleep(sleep_time);
		} else {
			sleep_time = 500 * time_keydelay;
		}

		uInputContext->send_key(key_code, 1);
		usleep(sleep_time);
		uInputContext->send_key(key_code, 0);
		usleep(sleep_time);


		if (isUpper) {
			uInputContext->send_key(KEY_LEFTSHIFT, 0);
			usleep(sleep_time);
		}

	}

	return pos;
}

const char *Type::name() {
	return ydotool_tool_name;
}

int Type::run(int argc, char **argv) {
	cxxopts::Options options("type", "type");

	options.add_options()
		("h,help", "Show this help")
		("key-delay", "Delay time between keystrokes", cxxopts::value<int>()->default_value("12"), "ms")
		("next-delay", "Delay time before typing next string on cmdline", cxxopts::value<int>()->default_value("50"), "ms")
		("file", "Specify a file, the contents of which will be be typed as if passed as an argument. The filepath may also be '-' to read from stdin", cxxopts::value<std::string>()->default_value(""), "path")

		("texts", "[Positional] Texts to type", cxxopts::value<std::string>())
		;

	options.parse_positional({"texts"});
	options.positional_help("<texts...>");
	options.show_positional_help();

	int next_delay;
	std::string file_path;
	std::vector<std::string> texts;

	try {
		auto cmd = options.parse(argc, argv);

		file_path = cmd["file"].as<std::string>();

		if (cmd.count("help") || (!cmd.count("texts") && file_path.empty())) {
			std::cout << options.help();
			return 0;
		}

		time_keydelay = cmd["key-delay"].as<int>();
		next_delay = cmd["next-delay"].as<int>();


		if (cmd.count("texts")) {
			texts.push_back(cmd["texts"].as<std::string>());
			texts.insert(texts.end(), cmd.unmatched().begin(), cmd.unmatched().end());
		}

	} catch (std::exception &e) {
		std::cout << "Ooooops! " << e.what() << "\n";
		std::cout << options.help();
		return 0;
	}

	int fd = -1;

	if (!file_path.empty()) {
		if (file_path == "-") {
			fd = STDIN_FILENO;
			fprintf(stderr, "ydotool: type: reading from stdin\n");
		} else {
			fd = open(file_path.c_str(), O_RDONLY);

			if (fd == -1) {
				fprintf(stderr, "ydotool: type: error: failed to open %s: %s\n", file_path.c_str(),
					strerror(errno));
				return 2;
			}
		}
	}

	if (fd >= 0) {
		std::string buf;
		buf.resize(128);

		ssize_t rc;
		while ((rc = read(fd, &buf[0], 128))) {
			if (rc > 0) {
				buf.resize(rc);
				TypeText(buf);
				buf.resize(128);
			} else if (rc < 0) {
				fprintf(stderr, "ydotool: type: error: read %s failed: %s\n", file_path.c_str(), strerror(errno));
				return 2;
			}
		}

		close(fd);
	} else {
		for (int i=0; i<texts.size(); i++) {
			auto &txt = texts[i];
			TypeText(txt);
			if (next_delay && i<(texts.size()-1))
				usleep(next_delay * 1000);
		}
	}

	return 0;
}


