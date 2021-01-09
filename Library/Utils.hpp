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
	class Utils {
	public:
		static void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result);
		static void dir_foreach(const std::string& path, const std::function<int(const std::string&, struct dirent *)>& callback, bool recursive=false);
		static uint32_t crc32(const void *buf, size_t len);
		static void uinput_availability_test();
	};
}
