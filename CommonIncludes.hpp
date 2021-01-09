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

#include <iostream>
#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <deque>

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cassert>
#include <ctime>

#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/un.h>

#include <sys/epoll.h>

#include <IODash.hpp>
#include <uInput.hpp>
#include <evdevPlus.hpp>
#include <cxxopts.hpp>
