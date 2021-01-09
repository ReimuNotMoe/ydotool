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

#include "MouseMove.hpp"

using namespace ydotool::Tools;

const char ydotool_tool_name[] = "mousemove";


const char *MouseMove::name() {
	return ydotool_tool_name;
}

int MouseMove::run(int argc, char **argv) {
	cxxopts::Options options("mousemove", "mousemove");

	options.add_options()
		("h,help", "Show this help")
		("absolute", "Use absolute position", cxxopts::value<bool>())
		("x", "[Positional] X", cxxopts::value<int>()->default_value("0"))
		("y", "[Positional] Y", cxxopts::value<int>()->default_value("0"))
		;

	options.parse_positional({"x", "y"});
	options.positional_help("<x> <y>");
	options.show_positional_help();

	int x, y;
	bool absolute = false;

	try {
		auto cmd = options.parse(argc, argv);

		if (cmd.count("help") || !cmd.count("x") || !cmd.count("y")) {
			std::cout << options.help();
			return 0;
		}

		if (cmd.count("absolute"))
			absolute = true;

		x = cmd["x"].as<int>();
		y = cmd["y"].as<int>();
	} catch (std::exception &e) {
		std::cout << "Ooooops! " << e.what() << "\n";
		std::cout << options.help();
		return 0;
	}



	if (absolute) {
		uInputContext->send_pos_relative({-INT32_MAX, -INT32_MAX});
	}

	uInputContext->send_pos_relative({x, y});

	return 0;
}

