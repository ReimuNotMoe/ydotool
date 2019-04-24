/*
    This file is part of ydotool.
    Copyright (C) 2018-2019 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef YDOTOOL_TOOL_MOUSEMOVE_HPP
#define YDOTOOL_TOOL_MOUSEMOVE_HPP

#include "../../Library/Tool.hpp"

using namespace ydotool;
using namespace uInputPlus;
namespace po = boost::program_options;

extern "C" {
extern const char ydotool_tool_name[];
}

class MouseMove : public Tool::ToolTemplate {
private:
	const std::string myName = ydotool_tool_name;
public:
	const std::string& Name() override {
		return myName;
	}

	int Exec(int argc, char **argv) override;
};

#endif //YDOTOOL_TOOL_MOUSEMOVE_HPP
