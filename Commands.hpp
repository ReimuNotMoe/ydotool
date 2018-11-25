/*
    This file is part of ydotool.
    Copyright (C) 2018 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef YDOTOOL_COMMANDS_HPP
#define YDOTOOL_COMMANDS_HPP

#include "CommonIncludes.hpp"

using namespace uInputPlus;

extern std::unordered_map<std::string, void *> CommandTable;
extern uInput *myuInput;

extern void InitCommands();
extern int InituInput();

namespace po = boost::program_options;

extern int Command_Type(int argc, const char *argv[]);
extern int Command_Key(int argc, const char *argv[]);
extern int Command_MouseMove(int argc, const char *argv[]);

#endif //YDOTOOL_COMMANDS_HPP
