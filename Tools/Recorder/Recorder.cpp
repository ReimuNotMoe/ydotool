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

#include "Recorder.hpp"

using namespace ydotool;
using namespace Tools;

const char ydotool_tool_name[] = "recorder";

const char *Recorder::name() {
	return ydotool_tool_name;
}

static int fd_file = -1;

static std::vector<uint8_t> record_buffer;
static Recorder::file_header header;

static void generate_header() {
	auto &m = header.magic;
	m[0] = 'Y';
	m[1] = 'D';
	m[2] = 'T';
	m[3] = 'L';

	header.feature_mask = 0;
	header.size = record_buffer.size();
	header.crc32 = Utils::crc32(record_buffer.data(), record_buffer.size());
}

static void stop_handler(int whatever) {
	std::cout << "Saving file...\n";
	generate_header();

	write(fd_file, &header, sizeof(header));
	write(fd_file, record_buffer.data(), record_buffer.size());

	close(fd_file);

	std::cout << "Done.\n";

	exit(0);
}

static void ShowHelp(char *argv_0){
	std::cerr << "Usage: " << argv_0 << " [--delay <ms] [--duration <ms>] [--record <output file> [devices]] [--replay <input file>]\n"
		  << "  --help                Show this help.\n"
		  << "  --record                \n"
		  << "  devices               Devices to record from. Default is all, including non-keyboard devices.\n"
		  << "  --replay                \n"
		  << "  --display               \n"
		  << "  --delay ms            Delay time before start recording/replaying. Default 5000ms.\n"
		  << "  --duration ms         Record duration. Otherwise use SIGINT to stop recording.\n"
		     "\n"
		     "The record file can't be replayed on an architecture with different endianness." << std::endl;
}

int Recorder::run(int argc, char **argv) {
	cxxopts::Options options("key", "key");

	options.add_options()
		("h,help", "Show this help")
		("record", "Record", cxxopts::value<bool>())
		("replay", "Replay", cxxopts::value<bool>())
		("display", "Display", cxxopts::value<bool>())
		("duration", "Record duration. If unspecified, use Ctrl-C to stop recording", cxxopts::value<int>()->default_value("0"), "ms")
		("devices", "Devices, separated by comma, to record from. Default is all devices", cxxopts::value<std::vector<std::string>>()->default_value(""), "path")
		("file", "File to record to / replay from", cxxopts::value<std::string>())
		;

	int duration = 0;
	int mode = 0;

	std::vector<std::string> device_list;
	std::string filepath;

	try {
		auto cmd = options.parse(argc, argv);

		if (cmd.count("help")) {
			std::cout << options.help();
			return 0;
		}

		filepath = cmd["file"].as<std::string>();
		duration = cmd["duration"].as<int>();
		device_list = cmd["devices"].as<std::vector<std::string>>();

		if (cmd.count("record")) {
			mode = 1;
		}
		if (cmd.count("replay")) {
			mode = 2;
		}
		if (cmd.count("display")) {
			mode = 3;
		}

		if (!mode)
			throw std::invalid_argument("mode not specified");

		if (filepath.empty())
			throw std::invalid_argument("file not specified");

	} catch (std::exception &e) {
		std::cout << "Ooooops! " << e.what() << "\n";
		std::cout << options.help();
		return 0;
	}


	if (mode == 1)
		fd_file = open(filepath.c_str(), O_WRONLY|O_CREAT, 0644);
	else
		fd_file = open(filepath.c_str(), O_RDONLY);

	if (fd_file == -1) {
		std::cerr << "ydotool: " << argv[0] << ": error: failed to open " << filepath << ": "
			  << strerror(errno) << std::endl;
		return 2;
	}

	if (mode == 1) {
		if (device_list.empty() || (device_list.size() == 1 && device_list[0] == "")) {
			device_list = find_all_devices();

			if (device_list.empty()) {
				std::cerr << "ydotool: " << argv[0] << ": error: no event device found in /dev/input/"
					  << std::endl;
				return 2;
			}
		}

		signal(SIGINT, stop_handler);

		if (duration)
			std::thread([duration]() {
				std::cerr << "Duration was set to "
					  << duration << " milliseconds.\n";
				usleep(duration * 1000);
				kill(getpid(), SIGINT);
			}).detach();

		do_record(device_list);
	} else if (mode == 2) {
		do_replay();
	} else if (mode == 3) {
		do_display();
	}
	
	return 0;
}


void Recorder::do_replay() {
	struct stat statat;

	fstat(fd_file, &statat);

	if (statat.st_size < sizeof(file_header)+sizeof(data_chunk)) {
		fprintf(stderr, "File too small\n");
		abort();
	}

	auto filedata = (uint8_t *)mmap(nullptr, statat.st_size, PROT_READ, MAP_SHARED, fd_file, 0);

	assert(filedata);

	auto file_hdr = (file_header *)filedata;
	auto file_end = filedata + statat.st_size;
	auto data_start = (filedata + sizeof(file_header));

	auto cur_pos = data_start;

	auto size_cur = file_end - data_start;

	if (size_cur == file_hdr->size) {
		fprintf(stderr, "Size match\n");
	} else {
		fprintf(stderr, "Size mismatch: %zu != %zu\n", size_cur, file_hdr->size);
		abort();
	}

	auto crc32_cur = Utils::crc32(data_start, size_cur);

	if (crc32_cur == file_hdr->crc32) {
		fprintf(stderr, "CRC32 match\n");
	} else {
		fprintf(stderr, "CRC32 mismatch: %08x != %08x\n", crc32_cur, file_hdr->crc32);
		abort();
	}

	std::cerr << "Started replaying\n";

	while (cur_pos < file_end) {
		auto dat = (data_chunk *)cur_pos;
		usleep(dat->delay[0] * 1000000 + dat->delay[1] / 1000);

		uInputContext->emit(dat->ev_type, dat->ev_code, dat->ev_value);

		cur_pos += sizeof(data_chunk);
	}
}

void Recorder::do_display() {
	struct stat statat;

	fstat(fd_file, &statat);

	if (statat.st_size < sizeof(file_header)+sizeof(data_chunk)) {
		fprintf(stderr, "File too small\n");
		abort();
	}

	auto filedata = (uint8_t *)mmap(nullptr, statat.st_size, PROT_READ, MAP_SHARED, fd_file, 0);

	assert(filedata);

	auto file_hdr = (file_header *)filedata;
	auto file_end = filedata + statat.st_size;
	auto data_start = (filedata + sizeof(file_header));

	auto cur_pos = data_start;

	auto size_cur = file_end - data_start;
	auto crc32_cur = Utils::crc32(data_start, size_cur);


	printf("CRC32: 0x%08x / 0x%08x\n", crc32_cur, file_hdr->crc32);
	printf("Data length: %zu / %zu (%zu events)\n", file_hdr->size, size_cur, file_hdr->size / sizeof(data_chunk));
	puts("============================================");


	while (cur_pos < file_end) {
		auto dat = (data_chunk *)cur_pos;
		printf("Offset: 0x%lx\n", (uint8_t *)(dat)-filedata);
		printf("Delay: %" PRIu64 ".%09" PRIu64 " second\n", dat->delay[0], dat->delay[1]);
		printf("Event: %u, %u, %u\n", dat->ev_type, dat->ev_code, dat->ev_value);
		puts("-");
		cur_pos += sizeof(data_chunk);
	}
}

void Recorder::do_record(const std::vector<std::string> &__devices) {

	fd_epoll = epoll_create(42);
	assert(fd_epoll > 0);


	for (auto &it : __devices) {
		std::cerr << "Using device: " << it << "\n";

		auto evDev = new evdevPlus::EventDevice(it);

		epoll_event eev{0};
		eev.data.ptr = evDev;
		eev.events = EPOLLIN;

		assert(!epoll_ctl(fd_epoll, EPOLL_CTL_ADD, evDev->fd(), &eev));

	}

	timespec tm_now, tm_now2, tm_diff;
	clock_gettime(CLOCK_MONOTONIC, &tm_now);

	epoll_event events[16];

	std::cerr << "Started recording\n";

	while (int rc = epoll_wait(fd_epoll, events, 16, -1)) {

		assert(rc > 0);

		for (int i=0; i<rc; i++) {
			auto &it = events[i];
			auto eev = (evdevPlus::EventDevice *)it.data.ptr;
			auto buf = eev->read();

			data_chunk dat;

			clock_gettime(CLOCK_MONOTONIC, &tm_now2);

			Utils::timespec_diff(&tm_now, &tm_now2, &tm_diff);

			tm_now = tm_now2;

			uint64_t time_hdr[2];
			dat.delay[0] = tm_diff.tv_sec;
			dat.delay[1] = tm_diff.tv_nsec;

			dat.ev_type = buf.Type;
			dat.ev_code = buf.Code;
			dat.ev_value = buf.Value;

			record_buffer.insert(record_buffer.end(), (uint8_t *)&dat, (uint8_t *)&dat+sizeof(data_chunk));

//			write(STDERR_FILENO, ".", 1);
		}
	}

}

std::vector<std::string> Recorder::find_all_devices() {
	std::vector<std::string> ret;

	try {
		Utils::dir_foreach("/dev/input", [&](const std::string &path_base, struct dirent *ent) {
			if (ent->d_type != DT_CHR || ent->d_name[0] != 'e')
				return 1;

			std::string fullpath = path_base + "/" + std::string(ent->d_name);
			ret.push_back(std::move(fullpath));

			return 0;
		});
	} catch (...) {

	}

	return ret;
}





