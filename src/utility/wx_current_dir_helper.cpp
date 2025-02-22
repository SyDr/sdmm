// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "wx_current_dir_helper.hpp"

mm::CurrentDirHelper::CurrentDirHelper(const wxString& dir)
	: _currentDir(wxGetCwd())
{
	wxSetWorkingDirectory(dir);
}

mm::CurrentDirHelper::~CurrentDirHelper()
{
	wxSetWorkingDirectory(_currentDir);
}
