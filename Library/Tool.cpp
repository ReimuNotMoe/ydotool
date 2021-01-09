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

#include "Tool.hpp"
#include "Utils.hpp"

#include "../Tools/Tools.hpp"

using namespace ydotool::Tool;

void ToolTemplate::init(uInputPlus::uInput* ui_ctx) {
	uInputContext = ui_ctx;
}

void ToolManager::scan_path(const std::string &__path) {
	try {
		Utils::dir_foreach(__path, [this](const std::string &path_base, struct dirent *ent) {
			if (ent->d_type != DT_REG && ent->d_type != DT_LNK)
				return 1;

			std::string fullpath = path_base + "/" + std::string(ent->d_name);
			try_dlopen(fullpath);
			return 0;
		});
	} catch (...) {

	}
}

void ToolManager::try_dlopen(const std::string &__path) {
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

	tools[tool_name] = {handle, tool_fptr, nullptr};
}

ToolManager::ToolManager(uInputPlus::uInput* ui_ctx) {
	uInputContext = ui_ctx;

	auto &t = tools;

	t["click"] = {nullptr, (void *)&Tools::Click::construct, nullptr};
	t["key"] = {nullptr, (void *)&Tools::Key::construct, nullptr};
	t["mousemove"] = {nullptr, (void *)&Tools::MouseMove::construct, nullptr};
	t["sleep"] = {nullptr, (void *)&Tools::Sleep::construct, nullptr};
	t["recorder"] = {nullptr, (void *)&Tools::Recorder::construct, nullptr};
	t["type"] = {nullptr, (void *)&Tools::Type::construct, nullptr};
}

int ToolManager::run_tool(int argc, char **argv) {
	std::string tool_name = argv[0];

	auto it_cmd = tools.find(tool_name);

	if (it_cmd == tools.end()) {
		std::cerr <<  "ydotool: Unknown tool: " << tool_name << "\n"
			  << "Run 'ydotool help' if you want a tools list" << std::endl;
		throw std::logic_error("Unknown tool");
	}

	if (!std::get<2>(it_cmd->second)) {
		auto tool_constructor = (void *(*)()) std::get<1>(it_cmd->second);
		std::get<2>(it_cmd->second) = static_cast<Tool::ToolTemplate *>(tool_constructor());
		static_cast<Tool::ToolTemplate *>(std::get<2>(it_cmd->second))->init(uInputContext);
	}

	auto *this_tool = static_cast<Tool::ToolTemplate *>(std::get<2>(it_cmd->second));

	return this_tool->run(argc, argv);
}
