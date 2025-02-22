// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

namespace mm
{
	struct CurrentDirHelper
	{
		CurrentDirHelper(const wxString& dir);
		~CurrentDirHelper();

	private:
		const wxString _currentDir;
	};
}
