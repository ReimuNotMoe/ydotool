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

#include "Key.hpp"

using namespace evdevPlus;
using namespace ydotool::Tools;

const char ydotool_tool_name[] = "key";


std::vector<std::string> ExplodeString(const std::string &str, char delim) {
	std::vector<std::string> result;
	std::istringstream iss(str);

	for (std::string token; std::getline(iss, token, delim); )
		result.push_back(std::move(token));

	return result;
}

static std::vector<std::string> SplitKeys(const std::string &keys) {
	if (!strchr(keys.c_str(), '+')) {
		return {keys};
	}

	return ExplodeString(keys, '+');
}

static std::vector<int> KeyStroke2Code(const std::string &ks) {
	auto list_keystrokes = SplitKeys(ks);

	std::vector<int> list_keycodes;

	for (auto &it : list_keystrokes) {
		for (auto &itc : it) {
			if (islower(itc))
				itc = toupper(itc);
		}

		auto t_kms = Table_ModifierKeys.find(it);
		if (t_kms != Table_ModifierKeys.end()) {
			list_keycodes.push_back(t_kms->second);
			continue;
		}

		auto t_kcs = Table_KeyCodes.find(it);
		if (t_kcs != Table_KeyCodes.end()) {
			list_keycodes.push_back(t_kcs->second);
			continue;
		}

		auto t_ks = Table_FunctionKeys.find(it);
		if (t_ks != Table_FunctionKeys.end()) {
			list_keycodes.push_back(t_ks->second);
		} else {
			auto t_kts = Table_LowerKeys.find(tolower(it[0]));

			if (t_kts != Table_LowerKeys.end()) {
				list_keycodes.push_back(t_kts->second);
			} else {
				throw std::invalid_argument("no matching keycode");
			}
		}
	}

	return list_keycodes;
}

const char *Key::name() {
	return ydotool_tool_name;
}


int Key::EmitKeyCodes(long key_delay, const std::vector<std::vector<int>> &list_keycodes) {
	auto sleep_time = (uint)(key_delay * 1000 / (list_keycodes.size() * 2));

	for (int i=0; i<list_keycodes.size(); i++) {
		auto &it = list_keycodes[i];

		if (mode == 0 || mode == 1) {
			for (auto &it_m : it) {
				uInputContext->send_key(it_m, 1);
				usleep(sleep_time);
			}
		}

		if (mode == 0) {
			for (auto i = it.size(); i-- > 0;) {
				uInputContext->send_key(it[i], 0);
				usleep(sleep_time);
			}
		}

		if (mode == 2) {
			for (auto &it_m : it) {
				uInputContext->send_key(it_m, 0);
				usleep(sleep_time);
			}
		}

		if (next_delay && i<(list_keycodes.size()-1))
			usleep(next_delay * 1000);
	}

	return 0;
}

static void ShowHelp(){
	std::cerr << "Usage: key [--delay <ms>] [--key-delay <ms>] [--repeat <times>] [--repeat-delay <ms>] <key sequence> ...\n"
		  << "  --help                Show this help.\n"
		  << "  --down                Only keydown\n"
		  << "  --up                  Only keyup\n"
		  << "  --delay ms            Delay time before start pressing keys. Default 100ms.\n"
		  << "  --key-delay ms        Delay time between keystrokes. Default 12ms.\n"
		  << "  --repeat times        Times to repeat the key sequence.\n"
		  << "  --repeat-delay ms     Delay time between repetitions. Default 0ms.\n"
		  << "  --persist-delay ms    Keep virtual device alive for <ms> ms. Should be used in conjunction with --down or --up\n"
		  << "\n"
		  << "Each key sequence can be any number of modifiers and keys, separated by plus (+)\n"
		  << "For example: alt+r Alt+F4 CTRL+alt+f3 aLT+1+2+3 ctrl+Backspace \n"
		  << "\n"
		  << "Since we are emulating keyboard input, combination like Shift+# is invalid.\n"
		  << "Because typing a `#' involves pressing Shift and 3." << std::endl;
}

static void ShowHelpExtra() {
	std::cerr << "\n"
		  << "Each key sequence can be any number of modifiers and keys, separated by plus (+)\n"
		  << "For example: alt+r Alt+F4 CTRL+alt+f3 aLT+1+2+3 ctrl+Backspace \n"
		  << "\n"
		  << "Since we are emulating keyboard input, combination like Shift+# is invalid.\n"
		  << "Because typing a `#' involves pressing Shift and 3." << std::endl;
}

int Key::run(int argc, char **argv) {
	cxxopts::Options options("key", "key");

	options.add_options()
		("h,help", "Show this help")
		("down", "Only keydown", cxxopts::value<bool>())
		("up", "Only keyup", cxxopts::value<bool>())
		("key-delay", "Delay time between keystrokes", cxxopts::value<int>()->default_value("12"), "ms")
		("repeat", "Times to repeat the sequence", cxxopts::value<int>()->default_value("1"), "times")
		("repeat-delay", "Delay time between repetitions", cxxopts::value<int>()->default_value("20"), "ms")
		("next-delay", "Delay time before pressing next key", cxxopts::value<int>()->default_value("50"), "ms")
		("keys", "[Positional] Keys to press", cxxopts::value<std::string>())
		;

	options.parse_positional({"keys"});
	options.positional_help("<keys...>");
	options.show_positional_help();

	std::vector<std::string> keys;

	try {
		auto cmd = options.parse(argc, argv);

		if (cmd.count("help") || !cmd.count("keys")) {
			std::cout << options.help();
			ShowHelpExtra();
			return 0;
		}

		time_keydelay = cmd["key-delay"].as<int>();
		repeats = cmd["repeat"].as<int>();
		time_repdelay = cmd["repeat-delay"].as<int>();
		next_delay = cmd["next-delay"].as<int>();

		keys.push_back(cmd["keys"].as<std::string>());
		keys.insert(keys.end(), cmd.unmatched().begin(), cmd.unmatched().end());


		if (cmd.count("up")) {
			mode = 2;
		} else if (cmd.count("down")) {
			mode = 1;
		}

	} catch (std::exception &e) {
		std::cout << "Ooooops! " << e.what() << "\n";
		std::cout << options.help();
		ShowHelpExtra();
		return 0;
	}


		std::cerr << "Next delay was set to "
			  << next_delay << " milliseconds.\n";

		std::cerr << "Key delay was set to "
			  << time_keydelay << " milliseconds.\n";

		std::cerr << "Repeat was set to "
			  << repeats << " times.\n";

		std::cerr << "Repeat delay was set to "
			  << time_repdelay << " milliseconds.\n";


	std::vector<std::vector<int>> keycodes;

	for (auto &ks : keys) {
		auto thiskc = KeyStroke2Code(ks);

		keycodes.emplace_back(thiskc);
	}

	while (repeats--) {
		EmitKeyCodes(time_keydelay, keycodes);
		if (repeats > 0) {
			usleep(time_repdelay * 1000);
		}
	}

	return 0;
}


