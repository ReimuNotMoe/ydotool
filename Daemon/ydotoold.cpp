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

#include "ydotoold.hpp"

using namespace IODash;
using namespace uInputPlus;
using namespace ydotool;

int main(int argc, char **argv) {
	cxxopts::Options options("ydotoold", "ydotool daemon");

	options.add_options()
		("h,help", "Show help")
		("socket-path", "Socket path", cxxopts::value<std::string>()->default_value("/tmp/.ydotool_socket"))
		("socket-perm", "Socket permission", cxxopts::value<std::string>()->default_value("0600"))
		;

	std::string cfg_socket_path, cfg_socket_perm;

	try {
		auto cmd = options.parse(argc, argv);

		if (cmd.count("help")) {
			std::cout << options.help();
			return 0;
		}

		cfg_socket_path = cmd["socket-path"].as<std::string>();
		cfg_socket_perm = cmd["socket-perm"].as<std::string>();

	} catch (std::exception &e) {
		std::cout << "Ooooops! " << e.what() << "\n";
		std::cout << options.help();
		return 0;
	}

	Utils::uinput_availability_test();

	uInputSetup uinput_setup({"ydotoold virtual device"});
	uInputPlus::uInput uinput(uinput_setup);

	unlink(cfg_socket_path.c_str());

	Socket<AddressFamily::Unix, SocketType::Datagram> socket_;
	socket_.create();
	socket_.bind(SocketAddress<AddressFamily::Unix>(cfg_socket_path));

	chmod(cfg_socket_path.c_str(), strtoul(cfg_socket_perm.c_str(), nullptr, 8));

	std::cerr << "ydotoold: " << "listening on socket " << cfg_socket_path << "\n";

	EventLoop<EventBackend::EPoll> event_loop;

	event_loop.add(socket_, EventType::In);

	std::vector<uint8_t> recv_buf(1536);

	event_loop.on_events([&](auto& evloop_ctx, File& file, EventType ev_type, auto& userdata){
		auto rc_recv = socket_cast<AddressFamily::Unix, SocketType::Datagram>(file).recv(recv_buf.data(), recv_buf.size());

		if (rc_recv > 0) {
			for (int i=0; i<rc_recv; i+=sizeof(uInputRawData)) {
				auto *buf = reinterpret_cast<uInputRawData *>(recv_buf.data() + i);
				uinput.emit(buf->type, buf->code, buf->value);
			}
		}
	});

	event_loop.run();

	return 0;
}