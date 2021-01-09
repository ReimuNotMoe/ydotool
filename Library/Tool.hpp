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

#pragma once

#include "../CommonIncludes.hpp"

namespace ydotool {
	namespace Tool {
		class ToolTemplate {
		public:
			uInputPlus::uInput* uInputContext = nullptr;

			ToolTemplate() = default;
			~ToolTemplate() = default;

			void init(uInputPlus::uInput* ui_ctx);
			virtual const char *name() = 0;
			virtual int run(int argc, char **argv) = 0;
		};

		class ToolManager {
		private:
			uInputPlus::uInput* uInputContext = nullptr;
		public:
			explicit ToolManager(uInputPlus::uInput* ui_ctx);

			std::unordered_map<std::string, std::tuple<void *, void *, void *>> tools; //dl_handles, init_funcs, constructed

			void try_dlopen(const std::string& __path);
			void scan_path(const std::string& __path);

			int run_tool(int argc, char **argv);
		};

	}
}
