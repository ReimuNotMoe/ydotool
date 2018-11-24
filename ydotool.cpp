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

static void ShowHelp() {
	fprintf(stderr, "Usage: ydotool <cmd> <args>\n"
		"Available commands:\n");

	for (auto &it : CommandTable) {
		fprintf(stderr, "  %s\n", it.first.c_str());
	}

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
		fprintf(stderr, "ydotool: Unknown command: %s\n"
			"Run 'ydotool help' if you want a command list\n", argv[1]);
		exit(1);
	}

	auto command = (int (*)(int, const char *[]))it_cmd->second;

	fprintf(stderr, "ydotool: Executing `%s' at %p\n", argv[1], command);

	int rc = command(argc-1, &argv[1]);


}