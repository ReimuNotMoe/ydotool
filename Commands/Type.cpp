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

static void ShowHelp(){
	fprintf(stderr, "Usage: type [--delay milliseconds] [--key-delay milliseconds] [--args N] [--file <filepath>] <things to type>\n"
		"  --help                    Show this help\n"
		"  --delay milliseconds      Delay time before start typing\n"
		"  --key-delay milliseconds  Delay time between keystrokes. Default 12ms\n"
		"  --args N                  How many arguments to expect in the exec command. This \n"
		"                            is useful for ending an exec and continuing with more \n"
		"                            ydotool commands\n"
		"  --file filepath           Specify a file, the contents of which will be be typed \n"
		"                            as if passed as an argument. The filepath may also be \n"
		"                            '-' to read from stdin\n");

}

static int TypeText(const std::string &text) {
	int pos = 0;

	for (auto &c : text) {

		int isUpper = 0;

		int key_code;

		auto itk = KeyTextStringTable.find(c);

		if (itk != KeyTextStringTable.end()) {
			key_code = itk->second;
		} else {
			auto itku = KeyTextStringTableUpper.find(c);
			if (itku != KeyTextStringTableUpper.end()) {
				isUpper = 1;
				key_code = itku->second;
			} else {
				return -(pos+1);
			}
		}

		int sleep_time;

		if (isUpper) {
			sleep_time = 250 * time_keydelay;
			myuInput->SendKey(KEY_LEFTSHIFT, 1);
			usleep(sleep_time);
		} else {
			sleep_time = 500 * time_keydelay;
		}

		myuInput->SendKey(key_code, 1);
		usleep(sleep_time);
		myuInput->SendKey(key_code, 0);
		usleep(sleep_time);


		if (isUpper) {
			myuInput->SendKey(KEY_LEFTSHIFT, 0);
			usleep(sleep_time);
		}

	}

	return pos;
}

int Command_Type(int argc, const char *argv[]) {

	printf("argc = %d\n", argc);

	for (int i=1; i<argc; i++) {
		printf("argv[%d] = %s \n", i, argv[i]);
	}

	int time_delay = 100;
	int args_count = -1;
	int text_start = -1;

	std::string file_path;
	std::vector<std::string> extra_args;

	try {

		po::options_description desc("");
		desc.add_options()
			("help", "Show this help")
			("delay", po::value<int>())
			("key-delay", po::value<int>())
			("args", po::value<int>())
			("file", po::value<std::string>())
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

		if (vm.count("key-delay")) {
			time_keydelay = vm["key-delay"].as<int>();
			std::cerr << "Key delay was set to "
				  << time_keydelay << " milliseconds.\n";
		}

		if (vm.count("args")) {
			args_count = vm["args"].as<int>();
			std::cerr << "Argument count was set to "
				  << args_count << ".\n";
		}

		if (vm.count("file")) {
			file_path = vm["file"].as<std::string>();
			std::cerr << "File path was set to "
				  << file_path << ".\n";
		} else {
			if (extra_args.empty()) {
				std::cerr << "What do you want to type?\n"
					"Use `ydotool type --help' for help.\n";

				return 1;
			}
		}


	} catch (std::exception &e) {
		fprintf(stderr, "ydotool: type: error: %s\n", e.what());
		return 2;
	}

	int fd = -1;

	if (!file_path.empty()) {
		fd = open(file_path.c_str(), O_RDONLY);

		if (fd == -1) {
			fprintf(stderr, "ydotool: type: error: failed to open %s: %s\n", file_path.c_str(), strerror(errno));
			return 2;
		}
	}

	InituInput();

	if (time_delay)
		usleep(time_delay * 1000);

	if (fd > 0) {
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
		for (auto &txt : extra_args) {
			TypeText(txt);
		}
	}

	return argc;

}