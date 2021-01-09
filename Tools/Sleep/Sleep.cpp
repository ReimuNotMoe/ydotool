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

#include "Sleep.hpp"


static const char ydotool_tool_name[] = "sleep";

using namespace ydotool::Tools;

const char *Sleep::name() {
	return ydotool_tool_name;
}


static void ShowHelp(){
	std::cerr << "Usage: sleep <ms>\n";
}

int Sleep::run(int argc, char **argv) {
	if (argc != 2) {
		ShowHelp();
	}

	usleep(1000 * strtoul(argv[1], nullptr, 10));

	return 0;
}

