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
	const auto pathFrom = toPath(_rootPath, from);
	const auto pathTo   = toPath(_rootPath, to);

	std::filesystem::copy(pathFrom, pathTo);

	_listChanged();
}

void Era2PresetManager::remove(const wxString& name)
{
	const auto path = toPath(_rootPath, name);

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
	const auto pathFrom = toPath(_rootPath, from);
	const auto pathTo   = toPath(_rootPath, to);

	std::filesystem::rename(pathFrom, pathTo);

	_listChanged();
}

sigslot::signal<>& Era2PresetManager::onListChanged()
{
	return _listChanged;
}

std::pair<ModList, std::unordered_map<wxString, PluginState>>
Era2PresetManager::loadPreset(wxString const& name)
{
	const auto    path = toPath(_rootPath, name);
	ModList       modList;
	std::unordered_map<wxString, PluginState> pluginList;
	std::ifstream datafile(path);

	if (!datafile)
	{
		wxLogError(wxString::Format("Cannot open file %s"_lng, path.string()));
		return { modList, pluginList };
	}

	nlohmann::json data;

	try
	{
		data = nlohmann::json::parse(datafile);
	}
	catch (nlohmann::json::parse_error const& e)
	{
		wxLogError(e.what());
		wxLogError(wxString::Format("Error while parsing file %s"_lng, path.string()));
		return { modList, pluginList };
	}

	if (!data.is_object())
		return { modList, pluginList };

	if (auto mods = data.find("mods"); mods != data.end() && mods->is_object())
	{
		if (auto active = mods->find("active"); active != mods->end() && active->is_array())
			for (const auto& item : *active)
				modList.active.emplace_back(wxString::FromUTF8(item));

		if (auto hidden = mods->find("hidden"); hidden != mods->end() && hidden->is_array())
			for (const auto& item : *hidden)
				modList.hidden.emplace(wxString::FromUTF8(item));
	}
	
	if (auto plugins = data.find("plugins"); plugins != data.end() && plugins->is_object())
	{
		if (auto enabled = plugins->find("enabled"); enabled != plugins->end() && enabled->is_array())
			for (const auto& item : *enabled)
				pluginList[wxString::FromUTF8(item)] = PluginState::enabled;

		if (auto disabled = plugins->find("disabled"); disabled != plugins->end() && disabled->is_array())
			for (const auto& item : *disabled)
				pluginList[wxString::FromUTF8(item)] = PluginState::disabled;
	}

	return { modList, pluginList };
}

void Era2PresetManager::savePreset(const wxString& name, const ModList& list,
								   const std::unordered_map<wxString, PluginState>& plugins)
{
	nlohmann::json data;

	data["mods"]           = nlohmann::json::object();
	data["mods"]["active"] = nlohmann::json::array();
	data["mods"]["hidden"] = nlohmann::json::array();

	for (const auto& item : list.active)
		data["mods"]["active"].emplace_back(item.ToStdString());

	for (const auto& item : list.hidden)
		data["mods"]["hidden"].emplace_back(item.ToStdString());

	data["plugins"]           = nlohmann::json::object();
	data["plugins"]["enabled"] = nlohmann::json::array();
	data["plugins"]["disabled"] = nlohmann::json::array();

	for (const auto& item : plugins)
		if (item.second == PluginState::enabled)
			data["plugins"]["enabled"].emplace_back(item.first.ToStdString());
		else
			data["plugins"]["disabled"].emplace_back(item.first.ToStdString());

	const auto path          = toPath(_rootPath, name);
	const bool already_exist = std::filesystem::exists(path);

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
