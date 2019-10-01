//
// Created by root on 9/2/19.
//

#ifndef YDOTOOL_TOOL_RECORD_HPP
#define YDOTOOL_TOOL_RECORD_HPP

#include "../../Library/Tool.hpp"
#include "../../Library/Utils.hpp"

using namespace ydotool;
using namespace uInputPlus;
namespace po = boost::program_options;

extern "C" {
extern const char ydotool_tool_name[];
}

class Recorder : public Tool::ToolTemplate {
public:
	struct file_header {
		char magic[4];
		uint32_t crc32;
		uint64_t feature_mask;
	} __attribute__((__packed__));

private:
	int fd_epoll = -1;
	int fd_file = -1;

public:
	const char *Name() override {
		return ydotool_tool_name;
	}

	void do_record(const std::vector<std::string>& __devices);

	std::vector<std::string> find_all_devices();

	int Exec(int argc, const char **argv) override;
};

#endif //YDOTOOL_TOOL_RECORD_HPP
