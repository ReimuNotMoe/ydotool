/*
    This file is part of ydotool.
    Copyright (C) 2018 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "Commands.hpp"

std::unordered_map<std::string, void *> CommandTable;

void InitCommands() {
	CommandTable["type"] = (void *)Command_Type;
	CommandTable["key"] = (void *)Command_Key;
	CommandTable["mousemove"] = (void *)Command_MouseMove;
	CommandTable["mousemove_relative"] = (void *)Command_MouseMove;
}