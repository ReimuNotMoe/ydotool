//
// Created by root on 9/2/19.
//

#include "Recorder.hpp"

using namespace ydotool;
using namespace Tools;

const char ydotool_tool_name[] = "recorder";


static void ShowHelp(const char *argv_0){
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

const char *Recorder::Name() {
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


int Recorder::Exec(int argc, const char **argv) {
	std::vector<std::string> extra_args;

	int delay = 5000;
	int duration = 0;
	int mode = 0;

	try {

		po::options_description desc("");
		desc.add_options()
			("help", "Show this help")
			("record", "")
			("replay", "")
			("display", "")
			("delay", po::value<int>())
			("duration", po::value<int>())
			("extra-args", po::value(&extra_args));


		po::positional_options_description p;
		p.add("extra-args", -1);


		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(desc).
			positional(p).
			run(), vm);
		po::notify(vm);

		if (vm.count("delay")) {
			delay = vm["delay"].as<int>();
		}

		if (vm.count("duration")) {
			duration = vm["duration"].as<int>();
		}

		if (vm.count("help")) {
			ShowHelp(argv[0]);
			return -1;
		}

		if (vm.count("record")) {
			mode = 1;
		}

		if (vm.count("replay")) {
			mode = 2;
		}

		if (vm.count("display")) {
			mode = 3;
		}

		if (!mode)
			throw std::invalid_argument("mode not specified");

		if (extra_args.empty())
			throw std::invalid_argument("file not specified");


	} catch (std::exception &e) {
		std::cerr <<  "ydotool: " << argv[0] << ": error: " << e.what() << std::endl;
		std::cerr << "Use --help for help.\n";

		return 2;
	}

	auto& filepath = extra_args.front();

	if (mode == 1)
		fd_file = open(filepath.c_str(), O_WRONLY|O_CREAT, 0644);
	else
		fd_file = open(filepath.c_str(), O_RDONLY);

	if (fd_file == -1) {
		std::cerr << "ydotool: " << argv[0] << ": error: failed to open " << filepath << ": "
			  << strerror(errno) << std::endl;
		return 2;
	}

	std::cerr << "Delay was set to "
		  << delay << " milliseconds.\n";



	if (mode == 1) {
		extra_args.erase(extra_args.begin());

		auto &device_list = extra_args;

		if (device_list.empty()) {
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

		if (delay)
			usleep(delay * 1000);

		do_record(device_list);
	} else if (mode == 2) {
		if (delay)
			usleep(delay * 1000);

		do_replay();
	} else if (mode == 3) {
		do_display();
	}
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

		uInputContext->Emit(dat->ev_type, dat->ev_code, dat->ev_value);

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

		assert(!epoll_ctl(fd_epoll, EPOLL_CTL_ADD, evDev->FD, &eev));

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
			auto buf = eev->Read();

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





