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

#include "Click.hpp"


static const char ydotool_tool_name[] = "click";

using namespace ydotool::Tools;

const char *Click::name() {
	return ydotool_tool_name;
}

int Click::run(int argc, char **argv) {
	cxxopts::Options options("click", "click");

	options.add_options()
		("h,help", "Show this help")
		("down", "Only mousedown", cxxopts::value<bool>())
		("up", "Only mouseup", cxxopts::value<bool>())
		("repeat", "times of running/repeating", cxxopts::value<int>()->default_value("1"), "times")
		("next-delay", "Delay time before clicking next button", cxxopts::value<int>()->default_value("50"), "ms")
		("buttons", "[Positional] Buttons to press (left, right, middle)", cxxopts::value<std::vector<std::string>>())
		;


	options.parse_positional({"buttons"});
	options.positional_help("<buttons...>");
	options.show_positional_help();

	int mode = 0, repeat = 1, next_delay=25;//next_delay min. 25ms

	std::vector<std::string> btns;

	try {
		auto cmd = options.parse(argc, argv);

		if (cmd.count("help") || !cmd.count("buttons")) {
			std::cout << options.help();
			return 0;
		}

		if (cmd.count("up")) {
			mode = 2;
		} else if (cmd.count("down")) {
			mode = 1;
		}

		repeat = cmd["repeat"].as<int>();
		next_delay = cmd["next-delay"].as<int>();
        
		btns = cmd["buttons"].as<std::vector<std::string>>();
		btns.insert(btns.end(), cmd.unmatched().begin(), cmd.unmatched().end());


	} catch (std::exception &e) {
		std::cout << "Ooooops! " << e.what() << "\n";
		std::cout << options.help();
		return 0;
	}
    
    for (int j=1; j<=repeat; ++j) {
        for (size_t i=0; i<btns.size(); i++) {
            auto &btn = btns[i];

            int keycode = BTN_LEFT;

            if (btn == "left")
                keycode = BTN_LEFT;
            else if (btn == "right")
                keycode = BTN_RIGHT;
            else if (btn == "middle")
                keycode = BTN_MIDDLE;

            if (mode != 2)
                uInputContext->send_key(keycode, 1);

            if (mode != 1)
                uInputContext->send_key(keycode, 0);

            //if (next_delay && i<(btns.size()-1))
            if (next_delay)
                usleep(next_delay * 1000);
        }
    }

	return 0;
}

