/*
    This file is part of ydotool.
    Copyright (C) 2018 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "../Commands.hpp"


using namespace uInputPlus;

static int time_keydelay = 12;

static void ShowHelp(const char *argv_0){
	fprintf(stderr, "Usage: %s [--delay <ms>] <x> <y>\n"
		"  --help                Show this help.\n"
		"  --delay ms            Delay time before start moving. Default 100ms.\n", argv_0);
}


int Command_MouseMove(int argc, const char *argv[]) {

	printf("argc = %d\n", argc);

	for (int i=1; i<argc; i++) {
		printf("argv[%d] = %s \n", i, argv[i]);
	}

	int time_delay = 100;

	std::vector<std::string> extra_args;

	try {

		po::options_description desc("");
		desc.add_options()
			("help", "Show this help")
			("delay", po::value<int>())
			("extra-args", po::value(&extra_args));


		po::positional_options_description p;
		p.add("extra-args", -1);


		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(desc).
			positional(p).
			run(), vm);
		po::notify(vm);


		if (vm.count("help")) {
			ShowHelp(argv[0]);
			return -1;
		}

		if (vm.count("delay")) {
			time_delay = vm["delay"].as<int>();
			std::cerr << "Delay was set to "
				  << time_delay << " milliseconds.\n";
		}

		if (extra_args.size() != 2) {
			std::cerr << "Which coordinate do you want to move to?\n"
				"Use `ydotool " << argv[0] << " --help' for help.\n";

			return 1;
		}

	} catch (std::exception &e) {
		fprintf(stderr, "ydotool: %s: error: %s\n", argv[0], e.what());
		return 2;
	}

	InituInput();

	if (time_delay)
		usleep(time_delay * 1000);

	auto x = (int32_t)strtol(extra_args[0].c_str(), nullptr, 10);
	auto y = (int32_t)strtol(extra_args[1].c_str(), nullptr, 10);

	if (!strchr(argv[0], '_')) {
		myuInput->RelativeMove({-INT32_MAX, -INT32_MAX});
	}

	myuInput->RelativeMove({x, y});

	return argc;
}