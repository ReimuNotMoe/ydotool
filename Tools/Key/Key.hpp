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

#include "../../Library/Tool.hpp"

namespace ydotool {
	namespace Tools {
		class Key : public Tool::ToolTemplate {
		private:
			int mode = 0, time_keydelay = 12, time_repdelay, repeats, next_delay;

		public:
			const char *name() override;

			int run(int argc, char **argv) override;

			int EmitKeyCodes(long key_delay, const std::vector<std::vector<int>> &list_keycodes);

			static void *construct() {
				return (void *)(new Key());
			}
		};
	}
}
