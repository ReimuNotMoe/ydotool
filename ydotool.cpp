/*
    This file is part of ydotool.
    Copyright (C) 2018 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "CommonIncludes.hpp"
#include "Commands.hpp"

using namespace uInputPlus;

uInput *myuInput = nullptr;

static void ShowHelp() {
	std::cerr << "Usage: ydotool <cmd> <args>\n"
		"Available commands:\n";

	for (auto &it : CommandTable) {
		std::cerr << it.first << std::endl;
	}

}

int InituInput() {
	if (!myuInput) {
		uInputSetup us({"ydotool virtual device"});

		myuInput = new uInput({us});
	}

	return 0;
}

int main(int argc, const char **argv) {

	InitCommands();

	if (argc < 2) {
		ShowHelp();
		exit(1);
	}

	if (strncmp(argv[1], "-h", 2) == 0 || strncmp(argv[1], "--h", 3) == 0 || strcmp(argv[1], "help") == 0) {
		ShowHelp();
		exit(1);
	}

	auto it_cmd = CommandTable.find(argv[1]);

	if (it_cmd == CommandTable.end()) {
		std::cerr <<  "ydotool: Unknown command: " << argv[0] << "\n"
			<< "Run 'ydotool help' if you want a command list" << std::endl;
		exit(1);
	}

	auto command = (int (*)(int, const char *[]))it_cmd->second;

	std::cerr <<  "ydotool: Executing `" << argv[1] << "' at " << std::hex << &command << std::endl;

	int rc = command(argc-1, &argv[1]);


}
