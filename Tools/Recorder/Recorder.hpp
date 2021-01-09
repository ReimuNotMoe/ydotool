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
#include "../../Library/Utils.hpp"

namespace ydotool {
	namespace Tools {
		class Recorder : public Tool::ToolTemplate {
		public:
			struct file_header {
				char magic[4];
				uint32_t crc32;
				uint64_t size;
				uint64_t feature_mask;
			} __attribute__((__packed__));

			struct data_chunk {
				uint64_t delay[2];
				uint16_t ev_type;
				uint16_t ev_code;
				int32_t ev_value;
			} __attribute__((__packed__));

		private:
			int fd_epoll = -1;
//			int fd_file = -1;

		public:
			const char *name() override;

			void do_record(const std::vector<std::string> &__devices);

			void do_replay();

			void do_display();

			std::vector<std::string> find_all_devices();

			int run(int argc, char **argv) override;

			static void *construct() {
				return (void *)(new Recorder());
			}
		};
	}
}