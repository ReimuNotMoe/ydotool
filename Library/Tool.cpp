/*
    This file is part of ydotool.
    Copyright (C) 2018-2019 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "Tool.hpp"

using namespace ydotool::Tool;

void ToolTemplate::Init(std::shared_ptr<ydotool::Instance> &__ydotool_instance) {
	ydotool_instance = __ydotool_instance;
	uInputContext = ydotool_instance->uInputContext.get();
}

void ToolManager::ScanPath(const std::string &__path) {
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(__path.c_str()))) {
		/* print all the files and directories within directory */
		while ((ent = readdir(dir))) {

			printf("%s\n", ent->d_name);
			std::string fullpath = __path + "/" + std::string(ent->d_name);
			TryDlOpen(fullpath);
		}
		closedir(dir);
	} else {
		/* could not open directory */
//		perror("");
//		return 2;
	}
}

void ToolManager::TryDlOpen(const std::string &__path) {
	void *handle = dlopen(__path.c_str(), RTLD_LAZY);

	if (!handle)
		return;

	auto tool_name = (const char *)dlsym(handle, "ydotool_tool_name");
	auto tool_fptr = dlsym(handle, "ydotool_tool_construct");

	if (!tool_name || !tool_fptr) {
		dlclose(handle);
		return;
	}

	std::cerr << "tool found: " << tool_name << " at " << __path << "\n";

	dl_handles[tool_name] = handle;
	init_funcs[tool_name] = tool_fptr;
}
