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
	const auto tmp = path.parent_path() / (path.extension().string() + ".tmp");
	const auto org = path.parent_path() / (path.extension().string() + ".org");

	boost::nowide::ofstream f(tmp, std::ios_base::out | std::ios_base::binary);
	f << content;
	f.close();

	rename(path, org);
	rename(tmp, path);
	remove(org);
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
