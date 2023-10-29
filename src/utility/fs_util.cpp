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

std::string mm::readFile(const mm::fs::path& path)
{
	boost::nowide::ifstream f(path);

	if (!f)
		return {};

	std::stringstream s;
	s << f.rdbuf();

	return s.str();
}

void mm::overwriteFileIfNeeded(const fs::path& path, const std::string& content)
{
	const auto current = readFile(path);
	if (current == content)
		return;

	overwriteFile(path, content);
}

void mm::overwriteFile(const fs::path& path, const std::string& content)
{
	// To be safe enough, do following:
	//	write content to temp file
	//	rename original file to other name
	//	rename temp file to original name
	//	now remove other name, it's not needed anymore

	const auto tmp = path.parent_path() / (path.filename().string() + ".tmp");
	const auto org = path.parent_path() / (path.filename().string() + ".mmorg");

	boost::nowide::ofstream f(tmp, std::ios_base::out | std::ios_base::binary);
	f << content;
	f.close();

	boost::system::error_code ec;
	remove(org, ec);        // if temp original exist -> remove it
	rename(path, org, ec);  // maybe there is no original yet
	rename(tmp, path);      // this must succeed
	remove(org, ec);        // remove original one if we have it
}

std::vector<wxString> mm::getAllDirs(const fs::path& path)
{
	std::vector<wxString> result;

	wxDir dir(wxString::FromUTF8(path.string()));
	if (!dir.IsOpened())
		return result;

	wxString tmp;
	if (dir.GetFirst(&tmp, L"", wxDIR_DIRS | wxDIR_HIDDEN))
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

	wxDir dir(wxString::FromUTF8(path.string()));
	if (!dir.IsOpened())
		return result;

	wxString tmp;
	if (dir.GetFirst(&tmp, L"", wxDIR_FILES | wxDIR_HIDDEN))
	{
		result.push_back(tmp);

		while (dir.GetNext(&tmp))
			result.push_back(tmp);
	}

	return result;
}
