/*
    This file is part of ydotool.
    Copyright (C) 2018-2019 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "ydotool.hpp"

using namespace ydotool;
using namespace uInputPlus;

uInput *myuInput = nullptr;

Tool::ToolManager tool_mgr;

static void ShowHelp() {
	std::cerr << "Usage: ydotool <cmd> <args>\n"
		"Available commands:\n";

	for (auto &it : tool_mgr.init_funcs) {
		std::cerr << "  " << it.first << std::endl;
	}

}

int InituInput() {
	if (!myuInput) {
		uInputSetup us({"ydotool virtual device"});

		myuInput = new uInput({us});
	}

	return 0;
}

std::vector<std::string> explode(const std::string& str, char delim) {
	std::vector<std::string> result;
	std::istringstream iss(str);

	for (std::string token; std::getline(iss, token, delim); ) {
		result.push_back(std::move(token));
	}

	return result;
}


int connect_socket(const char *path_socket) {
	int fd_client = socket(AF_UNIX, SOCK_STREAM, 0);

	if (fd_client == -1) {
		std::cerr << "ydotool: " << "failed to create socket: " << strerror(errno) << "\n";
		return -2;
	}

	sockaddr_un addr{0};
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path_socket, sizeof(addr.sun_path)-1);

	if (connect(fd_client, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)))
		return -1;

	return fd_client;
}

int socket_callback(uint16_t type, uint16_t code, int32_t val, void *userp) {
	uInputRawData buf{0};
	buf.type = type;
	buf.code = code;
	buf.value = val;

	int fd = (intptr_t)userp;

	send(fd, &buf, sizeof(buf), 0);

	return 0;
}

const char default_library_path[] = "/usr/local/lib/ydotool:/usr/lib/ydotool:/usr/lib/x86_64-linux-gnu/ydotool:/usr/lib/i386-linux-gnu/ydotool";

int main(int argc, const char **argv) {
	const char *library_path = default_library_path;

//	std::cerr << "ydotool: library search path: " << library_path << "\n";

//	for (auto &it : explode(library_path, ':')) {
//		tool_mgr.ScanPath(it);
//	}

	if (argc < 2) {
		ShowHelp();
		exit(1);
	}

	if (strncmp(argv[1], "-h", 2) == 0 || strncmp(argv[1], "--h", 3) == 0 || strcmp(argv[1], "help") == 0) {
		ShowHelp();
		exit(1);
	}

	auto it_cmd = tool_mgr.init_funcs.find(argv[1]);

	if (it_cmd == tool_mgr.init_funcs.end()) {
		std::cerr <<  "ydotool: Unknown tool: " << argv[1] << "\n"
			<< "Run 'ydotool help' if you want a tools list" << std::endl;
		exit(1);
	}

	auto instance = std::make_shared<Instance>();

	const char path_socket[] = "/tmp/.ydotool_socket";
	int fd_client = connect_socket(path_socket);

	if (fd_client > 0) {
		std::cerr << "ydotool: notice: Using ydotoold backend\n";
		instance->uInputContext = std::make_unique<uInput>();
		instance->uInputContext->Init(&socket_callback, (void *)(intptr_t)fd_client);
	} else {
		std::cerr << "ydotool: notice: ydotoold backend unavailable (may have latency+delay issues)\n";
		instance->Init();
	}

	auto tool_constructor = (void *(*)())it_cmd->second;
	auto *this_tool = static_cast<Tool::ToolTemplate *>(tool_constructor());

//	std::cerr <<  "ydotool: debug: tool `" << this_tool->Name() << "' constructed at " << std::hex << this_tool << std::dec << std::endl;

	this_tool->Init(instance);

	int rc = this_tool->Exec(argc-1, &argv[1]);

}