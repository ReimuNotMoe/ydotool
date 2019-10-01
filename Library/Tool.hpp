/*
    This file is part of ydotool.
    Copyright (C) 2018-2019 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef YDOTOOL_LIB_COMMANDS_HPP
#define YDOTOOL_LIB_COMMANDS_HPP

#include "../CommonIncludes.hpp"
#include "Instance.hpp"

namespace ydotool {
	namespace Tool {

		class ToolTemplate {
		public:
			std::shared_ptr<ydotool::Instance> ydotool_instance;
			uInput* uInputContext = nullptr;

			ToolTemplate() = default;
			~ToolTemplate() = default;

			void Init(std::shared_ptr<ydotool::Instance>& __ydotool_instance);
			virtual const char *Name() = 0;
			virtual int Exec(int argc, const char **argv) = 0;
		};

		class ToolManager {
		public:
			ToolManager();

			std::unordered_map<std::string, void *> dl_handles;
			std::unordered_map<std::string, void *> init_funcs;

			void TryDlOpen(const std::string& __path);
			void ScanPath(const std::string& __path);
		};

	}
}

#endif //YDOTOOL_LIB_COMMANDS_HPP
