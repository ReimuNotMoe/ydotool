/*
    This file is part of ydotool.
    Copyright (C) 2018-2019 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "Click.hpp"


static const char ydotool_tool_name[] = "click";

using namespace ydotool::Tools;

const char *Click::Name() {
	return ydotool_tool_name;
}


static void ShowHelp(){
	std::cerr << "Usage: click [--delay <ms>] <button>\n"
		<< "  --help                Show this help.\n"
		<< "  --delay ms            Delay time before start clicking. Default 100ms.\n"
		<< "  button                1: left 2: right 3: middle" << std::endl;
}

int Click::Exec(int argc, const char **argv) {
//	std::cout << "argc = " << argc << "\n";
//
//	for (int i=1; i<argc; i++) {
//		std::cout << "argv[" << i << "] = " << argv[i] << "\n";
//	}

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
			ShowHelp();
			return -1;
		}

		if (vm.count("delay")) {
			time_delay = vm["delay"].as<int>();
			std::cerr << "Delay was set to "
				  << time_delay << " milliseconds.\n";
		}

		if (extra_args.size() != 1) {
			std::cerr << "Which mouse button do you want to click?\n"
				     "Use `ydotool " << argv[0] << " --help' for help.\n";

			return 1;
		}

	} catch (std::exception &e) {
		std::cerr << "ydotool: click: error: " << e.what() << std::endl;
		return 2;
	}

	if (time_delay)
		usleep(time_delay * 1000);

	int keycode = BTN_LEFT;

	switch (strtol(extra_args[0].c_str(), NULL, 10)) {
		case 2:
			keycode = BTN_RIGHT;
			break;
		case 3:
			keycode = BTN_MIDDLE;
			break;
		default:
			break;
	}

	uInputContext->SendKey(keycode, 1);
	uInputContext->SendKey(keycode, 0);

	return argc;
}

