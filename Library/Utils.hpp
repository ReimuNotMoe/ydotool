//
// Created by root on 9/2/19.
//

#ifndef YDOTOOL_LIB_UTILS_HPP
#define YDOTOOL_LIB_UTILS_HPP

#include "../CommonIncludes.hpp"

namespace ydotool {
	class Utils {
	public:
		static void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result);
		static void dir_foreach(const std::string& path, const std::function<int(const std::string&, struct dirent *)>& callback, bool recursive=false);
		static uint32_t crc32(const void *buf, size_t len);
	};
}
#endif //YDOTOOL_LIB_UTILS_HPP
