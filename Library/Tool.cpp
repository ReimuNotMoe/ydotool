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
#include "Utils.hpp"

#include "../Tools/Tools.hpp"

using namespace ydotool::Tool;

void ToolTemplate::Init(std::shared_ptr<ydotool::Instance> &__ydotool_instance) {
	ydotool_instance = __ydotool_instance;
	uInputContext = ydotool_instance->uInputContext.get();
}

void ToolManager::ScanPath(const std::string &__path) {
	try {
		Utils::dir_foreach(__path, [this](const std::string &path_base, struct dirent *ent) {
			if (ent->d_type != DT_REG && ent->d_type != DT_LNK)
				return 1;

			std::string fullpath = path_base + "/" + std::string(ent->d_name);
			TryDlOpen(fullpath);
			return 0;
		});
	} catch (...) {

	}
}

void ToolManager::TryDlOpen(const std::string &__path) {
	void *handle = dlopen(__path.c_str(), RTLD_LAZY);

	if (!handle) {
		std::cerr <<  "ydotool: dlopen failed: " << dlerror() << "\n";
		return;
	}

	auto tool_name = (const char *)dlsym(handle, "ydotool_tool_name");
	auto tool_fptr = dlsym(handle, "ydotool_tool_construct");

	if (!tool_name || !tool_fptr) {
		dlclose(handle);
		return;
	}

//	std::cerr << "ToolManager: debug: tool found: " << tool_name << " at " << __path << "\n";

	dl_handles[tool_name] = handle;
	init_funcs[tool_name] = tool_fptr;
}

ToolManager::ToolManager() {
	auto &i = init_funcs;


	i["click"] = (void *)&Tools::Click::construct;
	i["key"] = (void *)&Tools::Key::construct;
	i["mousemove"] = (void *)&Tools::MouseMove::construct;
	i["recorder"] = (void *)&Tools::Recorder::construct;
	i["type"] = (void *)&Tools::Type::construct;
}
