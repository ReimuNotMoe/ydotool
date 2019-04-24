/*
    This file is part of ydotool.
    Copyright (C) 2018-2019 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "Instance.hpp"

using namespace ydotool;

void Instance::Init() {
	Init("ydotool virtual device");
}

void Instance::Init(const std::string &device_name) {
//	uInputDeviceInfo ud(device_name);
	uInputSetup us(device_name);
	uInputContext = std::make_unique<uInput>();
	uInputContext->Init({us});
}
