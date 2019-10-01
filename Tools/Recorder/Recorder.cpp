//
// Created by root on 9/2/19.
//

#include "Recorder.hpp"

using namespace ydotool::Tools;

const char ydotool_tool_name[] = "recorder";


static void ShowHelp(const char *argv_0){
	std::cerr << "Usage: " << argv_0 << " <output file> [devices]\n"
		  << "  --help                Show this help.\n"
		  << "  devices               Devices to record from. Default is all." << std::endl;
}

const char *Recorder::Name() {
	return ydotool_tool_name;
}


int Recorder::Exec(int argc, const char **argv) {
	std::cout << "argc = " << argc << "\n";

	for (int i=1; i<argc; i++) {
		std::cout << "argv["<<i<<"] = " << argv[i] << "\n";
	}

	std::vector<std::string> extra_args;

	try {

		po::options_description desc("");
		desc.add_options()
			("help", "Show this help")
			("extra-args", po::value(&extra_args));


		po::positional_options_description p;
		p.add("extra-args", -1);


		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(desc).
			positional(p).
			run(), vm);
		po::notify(vm);


		if (vm.count("help")) {
			ShowHelp(argv[0]);
			return -1;
		}

		if (extra_args.empty())
			throw std::invalid_argument("output file not specified");


	} catch (std::exception &e) {
		std::cerr <<  "ydotool: " << argv[0] << ": error: " << e.what() << std::endl;
		return 2;
	}

	auto& filepath = extra_args.front();

	fd_file = open(filepath.c_str(), O_WRONLY|O_CREAT, 0644);

	if (fd_file == -1) {
		std::cerr << "ydotool: " << argv[0] << ": error: failed to open " << filepath << ": "
		<< strerror(errno) << std::endl;
		return 2;
	}

	extra_args.erase(extra_args.begin());

	auto& device_list = extra_args;

	if (device_list.empty()) {
		device_list = find_all_devices();

		if (device_list.empty()) {
			std::cerr << "ydotool: " << argv[0] << ": error: no event device found in /dev/input/"
				  << std::endl;
			return 2;
		}
	}

	do_record(device_list);

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

			clock_gettime(CLOCK_MONOTONIC, &tm_now2);

			Utils::timespec_diff(&tm_now, &tm_now2, &tm_diff);

			tm_now = tm_now2;

			uint64_t time_hdr[2];
			time_hdr[0] = tm_diff.tv_sec;
			time_hdr[1] = tm_diff.tv_nsec;

			write(fd_file, time_hdr, 16);
			write(fd_file, &buf.event, sizeof(input_event));

			write(STDERR_FILENO, ".", 1);
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

