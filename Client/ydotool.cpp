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

#include "ydotool.hpp"

using namespace IODash;
using namespace uInputPlus;
using namespace ydotool;

static void ShowHelp(Tool::ToolManager *tool_mgr) {
	std::cerr << "Usage: ydotool <opts> <cmd> <args>\n"
		"Options:\n"
		"  --socket-path\tPath to daemon socket, default: /tmp.ydotool_socket\n\n"
		"Available commands:\n";

	for (auto &it : tool_mgr->tools) {
		std::cerr << "  " << it.first << std::endl;
	}

}

int main(int argc, char **argv) {

	cxxopts::Options options("ydotool", "ydotool client");

	options
		.allow_unrecognised_options()
		.add_options()
		("socket-path", "Socket path", cxxopts::value<std::string>()->default_value("/tmp/.ydotool_socket"))
		;

	std::string cfg_socket_path;
	std::unique_ptr<uInput> uinput_;
	Tool::ToolManager tool_mgr(uinput_.get());
	std::vector<char *> commands;

	try {
		auto cmd = options.parse(argc, argv);
		for (auto unmatched: cmd.unmatched()) {
			commands.push_back(strdup(unmatched.c_str()));
		}

		cfg_socket_path = cmd["socket-path"].as<std::string>();

	} catch (std::exception &e) {
		std::cout << "Ooooops! " << e.what() << "\n";
		std::cout << options.help();
		return 0;
	}

	if (commands.size() < 1) {
		ShowHelp(&tool_mgr);
		exit(1);
	}

	if (strncmp(commands[0], "-h", 2) == 0 || strncmp(commands[0], "--h", 3) == 0 || strcmp(commands[0], "help") == 0) {
		ShowHelp(&tool_mgr);
		exit(1);
	}

	Socket<AddressFamily::Unix, SocketType::Datagram> socket_;

	try {
		socket_.create();
		if (!socket_.connect(cfg_socket_path)) {
			throw std::system_error(errno, std::system_category(), "failed to connect to socket");
		}
		uinput_ = std::make_unique<uInput>([&](uint16_t type, uint16_t code, int32_t val){
				uInputRawData buf{type, code, val};
				socket_.send(&buf, sizeof(buf));
				});
		std::cerr << "ydotool: notice: Using ydotoold backend\n";
	} catch (std::exception &e) {
		std::cerr << "ydotool: notice: unable to access ydotoold backend: " << e.what() << "\n";
		std::cerr << "ydotool: you may experience device recognition delay issues\n";
		Utils::uinput_availability_test();
		uinput_ = std::make_unique<uInput>(uInputSetup({"ydotool virtual device"}));
	}



	std::vector<char *> splitted_argv;

	for (int i=0; i < commands.size(); i++) {
		if (0 == strcmp(commands[i], ",")) {
			if (!splitted_argv.empty()) {
				tool_mgr.run_tool(splitted_argv.size(), splitted_argv.data());
				splitted_argv.clear();
			}
		} else {
			splitted_argv.emplace_back(commands[i]);
		}
	}
	if (!splitted_argv.empty()) {
		tool_mgr.run_tool(splitted_argv.size(), splitted_argv.data());
	}

	return 0;
}
