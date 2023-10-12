// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "fs_util.h"

#include "application.h"
#include "sdlexcept.h"

#include <boost/algorithm/string/replace.hpp>
#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/textfile.h>

void mm::overwriteFileContent(const fs::path& path, const wxString& content)
{
	wxTempFile target(path.wstring());

	MM_PRECONDTION(target.IsOpened());
	MM_PRECONDTION(target.Write(content));
	MM_PRECONDTION(target.Commit());
}

bool mm::createDir(const fs::path& path)
{
	boost::system::error_code ec;
	bool                      created = fs::create_directories(path, ec);

	if (ec)
		wxLogError(wxString("Can't create dir '%s'\r\n\r\n%s (code: %d)"_lng), path.wstring(), ec.message(),
			ec.value());

	return created && !ec;
}

std::vector<wxString> mm::getAllDirs(const fs::path& path)
{
	std::vector<wxString> result;

	wxDir dir(path.string());
	if (!dir.IsOpened())
		return result;

	wxString tmp;
	if (dir.GetFirst(&tmp, "", wxDIR_DIRS | wxDIR_HIDDEN))
	{
		result.push_back(tmp);

		while (dir.GetNext(&tmp))
			result.push_back(tmp);
	}

	return result;
}

std::vector<wxString> mm::getAllFiles(const fs::path& path)
{
	std::vector<wxString> result;

	wxDir dir(path.string());
	if (!dir.IsOpened())
		return result;

	wxString tmp;
	if (dir.GetFirst(&tmp, "", wxDIR_FILES | wxDIR_HIDDEN))
	{
		result.push_back(tmp);

		while (dir.GetNext(&tmp))
			result.push_back(tmp);
	}

	return result;
}

std::vector<wxString> mm::readFile(const fs::path& path)
{
	wxLogNull noLogging;  // suppress wxWidgets messages about inability to open file

	wxTextFile file;
	if (!file.Open(path.wstring()))
		return {};

	std::vector<wxString> result;
	for (auto& str = file.GetFirstLine(); !file.Eof(); str = file.GetNextLine())
		result.emplace_back(str);

	file.Close();

	return result;
}
