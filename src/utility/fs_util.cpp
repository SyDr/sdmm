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

void mm::overwriteFileContent(const fs::path& path, const std::string& content)
{
	wxTempFile target(path.string());

	MM_PRECONDTION(target.IsOpened());
	MM_PRECONDTION(target.Write(content));
	MM_PRECONDTION(target.Commit());
}

bool mm::createDir(const std::filesystem::path& path)
{
	std::error_code ec;
	bool            created = std::filesystem::create_directories(path, ec);

	if (ec)
		wxLogError(wxString("Can't create dir '%s'\r\n\r\n%s (code: %d)"_lng), path.string(), ec.message(),
				   ec.value());

	return created && !ec;
}

bool mm::copyDir(const wxString& path, const wxString& newPath)
{
	if (!createDir(newPath.ToStdString()))
		return false;

	using rdi = std::filesystem::recursive_directory_iterator;
	for (auto it = rdi(path.ToStdString()), end = rdi(); it != end; ++it)
	{
		const auto from = it->path();
		const auto to   = std::filesystem::path(newPath.ToStdString()) /
						boost::replace_first_copy(from.string(), path.ToStdString(), "");

		std::error_code ec;
		std::filesystem::copy(from, to, ec);

		if (ec)
			wxLogError(wxString("Can't copy '%s' to '%s'\r\n\r\n%s (code: %d)"_lng), wxString(from.string()),
					   wxString(to.string()), ec.message(), ec.value());
	}

	return true;
}

std::vector<wxString> mm::getAllDirs(const std::filesystem::path& path)
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

std::vector<wxString> mm::getAllFiles(const std::filesystem::path& path)
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
