// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_preset_manager.h"

#include "application.h"
#include "era2_config.h"
#include "era2_mod_manager.h"
#include "era2_platform.h"
#include "utility/fs_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "utility/string_util.h"

#include <wx/dir.h>
#include <wx/file.h>

using namespace mm;

namespace
{
	std::filesystem::path toPath(std::filesystem::path const& base, wxString const& name)
	{
		return base / (name + ".json").ToStdString();
	}
}

Era2PresetManager::Era2PresetManager(std::filesystem::path rootPath)
	: _rootPath(std::move(rootPath))
{
}

void Era2PresetManager::copy(const wxString& from, const wxString& to)
{
	auto const pathFrom = toPath(_rootPath, from);
	auto const pathTo   = toPath(_rootPath, to);

	std::filesystem::copy(pathFrom, pathTo);

	_listChanged();
}

void Era2PresetManager::remove(const wxString& name)
{
	auto const path = toPath(_rootPath, name);

	std::filesystem::remove(path);

	_listChanged();
}

std::set<wxString> Era2PresetManager::list() const
{
	std::set<wxString> result;

	std::filesystem::directory_iterator di(_rootPath);

	for (const auto end = std::filesystem::directory_iterator(); di != end; ++di)
	{
		if (std::filesystem::is_regular_file(*di) && di->path().extension() == ".json")
			result.emplace(di->path().stem());
	}


	return result;
}

void Era2PresetManager::rename(const wxString& from, const wxString& to)
{
	auto const pathFrom = toPath(_rootPath, from);
	auto const pathTo   = toPath(_rootPath, to);

	std::filesystem::rename(pathFrom, pathTo);

	_listChanged();
}

wigwag::signal_connector<void()> Era2PresetManager::onListChanged() const
{
	return _listChanged.connector();
}

ModList Era2PresetManager::loadPreset(wxString const& name)
{
	auto const    path = toPath(_rootPath, name);
	ModList       modList;
	std::ifstream datafile(path);

	if (!datafile)
	{
		wxLogError(wxString::Format("Cannot open file %s"_lng, path.u8string()));
		return modList;
	}

	nlohmann::json data;

	try
	{
		data = nlohmann::json::parse(datafile);
	}
	catch (nlohmann::json::parse_error const& e)
	{
		wxLogError(e.what());
		wxLogError(wxString::Format("Error while parsing file %s"_lng, path.u8string()));
		return modList;
	}

	if (!data.is_object())
		return modList;

	if (auto active = data.find("active"); active != data.end() && active->is_array())
		for (auto const& item : *active)
			modList.active.emplace_back(wxString::FromUTF8(item));

	if (auto hidden = data.find("hidden"); hidden != data.end() && hidden->is_array())
		for (auto const& item : *hidden)
			modList.hidden.emplace(wxString::FromUTF8(item));

	return modList;
}

void Era2PresetManager::savePreset(wxString const& name, ModList const& list)
{
	std::vector<std::string> active(list.active.size());
	for (size_t i = 0; i < list.active.size(); ++i)
		active[i] = list.active[i].ToStdString();

	std::vector<std::string> hidden;
	for (auto const& item : list.hidden)
		hidden.emplace_back(item.ToStdString());

	nlohmann::json data;
	data["active"] = active;
	data["hidden"] = hidden;

	auto const path          = toPath(_rootPath, name);
	bool const already_exist = std::filesystem::exists(path);

	std::ofstream datafile(path);
	datafile << data.dump(2);
	datafile.close();

	if (!already_exist)
		_listChanged();
}

bool Era2PresetManager::exists(wxString const& name) const
{
	return std::filesystem::exists(toPath(_rootPath, name));
}
