// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_preset_manager.h"

#include "application.h"
#include "era2_config.hpp"
#include "era2_mod_manager.h"
#include "era2_platform.h"
#include "era2_plugin_manager.hpp"
#include "utility/fs_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"

#include <wx/dir.h>
#include <wx/file.h>

using namespace mm;

namespace
{
	fs::path toPath(fs::path const& base, const wxString& name)
	{
		return base / (name + L".json").ToStdString(wxConvUTF8);
	}
}

Era2PresetManager::Era2PresetManager(fs::path rootPath, fs::path modsPath)
	: _rootPath(std::move(rootPath))
	, _modsPath(std::move(modsPath))
{}

void Era2PresetManager::copy(const wxString& from, const wxString& to)
{
	const auto pathFrom = toPath(_rootPath, from);
	const auto pathTo   = toPath(_rootPath, to);

	fs::copy(pathFrom, pathTo);

	_listChanged();
}

void Era2PresetManager::remove(const wxString& name)
{
	const auto path = toPath(_rootPath, name);

	fs::remove(path);

	_listChanged();
}

std::set<wxString> Era2PresetManager::list() const
{
	std::set<wxString> result;

	fs::directory_iterator di(_rootPath);

	for (const auto end = fs::directory_iterator(); di != end; ++di)
	{
		if (fs::is_regular_file(*di) && di->path().extension() == ".json")
			result.emplace(di->path().stem().wstring());
	}

	return result;
}

void Era2PresetManager::rename(const wxString& from, const wxString& to)
{
	const auto pathFrom = toPath(_rootPath, from);
	const auto pathTo   = toPath(_rootPath, to);

	fs::rename(pathFrom, pathTo);

	_listChanged();
}

sigslot::signal<>& Era2PresetManager::onListChanged()
{
	return _listChanged;
}

std::pair<ModList, PluginList> Era2PresetManager::loadPreset(const wxString& name)
{
	ModList    modList;
	PluginList pluginList;

	const auto path = toPath(_rootPath, name);

	boost::nowide::ifstream datafile(path);

	if (!datafile)
	{
		wxLogError(wxString::Format("Cannot open file %s"_lng, wxString::FromUTF8(path.string())));
		return { modList, pluginList };
	}

	nlohmann::json data;

	try
	{
		data = nlohmann::json::parse(datafile);
	}
	catch (nlohmann::json::parse_error const& e)
	{
		wxLogError(wxString::FromUTF8(e.what()));
		wxLogError(wxString::Format("Error while parsing file %s"_lng, wxString::FromUTF8(path.string())));
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

	pluginList.available = Era2PluginManager::loadAvailablePlugins(_modsPath, modList);
	if (auto plugins = data.find("plugins"); plugins != data.end())
		pluginList.managed = Era2PluginManager::loadManagedState(*plugins);

	erase_if(
		pluginList.available, [&](const PluginSource& item) { return !pluginList.managed.contains(item); });

	return { modList, pluginList };
}

void Era2PresetManager::savePreset(const wxString& name, const ModList& list, const PluginList& plugins)
{
	nlohmann::json data;

	data["mods"]           = nlohmann::json::object();
	data["mods"]["active"] = nlohmann::json::array();
	data["mods"]["hidden"] = nlohmann::json::array();

	for (const auto& item : list.active)
		data["mods"]["active"].emplace_back(item.ToStdString(wxConvUTF8));

	for (const auto& item : list.hidden)
		data["mods"]["hidden"].emplace_back(item.ToStdString(wxConvUTF8));

	auto& ref = data["plugins"] = nlohmann::json::array();
	for (const auto& source : plugins.managed)
	{
		const auto path =
			(fs::path(source.modId) / to_string(source.location) / source.name).lexically_normal();
		ref.emplace_back(path.string());
	}

	const auto path          = toPath(_rootPath, name);
	const bool already_exist = fs::exists(path);

	boost::nowide::ofstream datafile(path);
	datafile << data.dump(2);
	datafile.close();

	if (!already_exist)
		_listChanged();
}

bool Era2PresetManager::exists(const wxString& name) const
{
	return fs::exists(toPath(_rootPath, name));
}
