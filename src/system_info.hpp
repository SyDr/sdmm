// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "utility/macro.h"

#define PROGRAM_NAME "SD Mod Manager"
#define PROGRAM_ALIAS "The Sparrow"
#define PROGRAM_VERSION "0.98.1.alpha"

namespace mm::constant
{
	auto constexpr data_dir = "data";
	auto constexpr mod_info_filename = "mod.json";
	auto constexpr default_language = "en_US";

	auto constexpr program_full_version = PROGRAM_NAME " [" PROGRAM_VERSION " - " PROGRAM_ALIAS "]";

	auto constexpr program_license_text = R"LICENSE(Copyright (c) 2020 Aliaksei Karalenka

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.)LICENSE";
}
