// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "app_config.h"

#include <unordered_set>
#include <fstream>
#include <sstream>

#include <wx/stdpaths.h>
#include <wx/log.h>

#include "utility/string_util.h"
#include "utility/sdlexcept.h"
#include "types/main_window_properties.h"

using namespace mm;

namespace
{
	std::filesystem::path constructPortableDataPath()
	{
		std::filesystem::path result(wxStandardPaths::Get().GetDataDir().ToStdString());
		result = result.parent_path();
		result /= "_MM_Data";

		return result.make_preferred();
	}

	std::filesystem::path constructUserDataPath()
	{
		std::filesystem::path result(wxStandardPaths::Get().GetUserDataDir().ToStdString());

		return result.make_preferred();
	}
}

AppConfig::AppConfig()
	: _portableDataPath(constructPortableDataPath())
	, _userDataPath(constructUserDataPath())
	, _portableMode(std::filesystem::exists(_portableDataPath) && std::filesystem::is_directory(_portableDataPath))
{
	if (!portableMode() && !std::filesystem::exists(_userDataPath))
		std::filesystem::create_directories(_userDataPath);

	std::ifstream datafile(configFilePath().string());

	if (datafile)
	{
		std::stringstream stream;
		stream << datafile.rdbuf();
		datafile.close();

		_data = nlohmann::json::parse(stream, nullptr, false);

		if (_data.is_discarded())
		{
			_data = {};
			wxLogDebug("Can't parse config file. Default config is used instead");
		}
	}
	else
	{
		wxLogDebug("Can't load config file. Default config is used instead");
	}

	validate();
}

bool AppConfig::portableMode() const
{
	return _portableMode;
}

std::filesystem::path AppConfig::dataPath() const
{
	return portableMode() ? _portableDataPath : _userDataPath;
}

std::filesystem::path AppConfig::programPath() const
{
	return std::filesystem::path(wxStandardPaths::Get().GetDataDir().ToStdString());
}

#define SD_LNG_CODE "language"
#define SD_GAME "game"
#define SD_KNOWN "directories"
#define SD_FAVS "stars"
#define SD_PLATFORM "platform"
#define SD_SELECTED "selected"
#define SD_WINDOW "window"
#define SD_WIDTH "width"
#define SD_HEIGHT "height"
#define SD_LEFT "left"
#define SD_TOP "top"
#define SD_MAXIMIZED "maximized"

auto AppConfig::currentLanguageCode() const -> std::string
{
	return _data[SD_LNG_CODE].get<std::string>();
}

void AppConfig::setCurrentLanguageCode(const wxString& lngCode)
{
	_data[SD_LNG_CODE] = lngCode.ToStdString();
}

void AppConfig::save()
{
	std::ofstream datafile(configFilePath().string());
	datafile << _data.dump(2);
}

std::vector<std::filesystem::path> AppConfig::getKnownDataPathList() const
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	std::vector<std::filesystem::path> result;
	for (auto path : _data[SD_GAME][selectedPlatform().ToStdString()][SD_KNOWN].get<std::vector<std::string>>())
		result.emplace_back(path);

	return result;
}

wxString AppConfig::selectedPlatform() const
{
	//expects(!portableMode());

	return _data[SD_GAME][SD_PLATFORM].get<std::string>();
}

std::filesystem::path AppConfig::getDataPath() const
{
	if (!portableMode())
		return _data[SD_GAME][selectedPlatform().ToStdString()][SD_SELECTED].get<std::string>();
	else
		return _portableDataPath.parent_path();
}

void AppConfig::setDataPath(const std::filesystem::path& path)
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	const auto newPath = path.string();
	auto& knownPaths = _data[SD_GAME][selectedPlatform().ToStdString()][SD_KNOWN];

	if (std::find(knownPaths.cbegin(), knownPaths.cend(), newPath) == knownPaths.end())
		_data[SD_GAME][selectedPlatform().ToStdString()][SD_KNOWN].emplace_back(newPath);

	_data[SD_GAME][selectedPlatform().ToStdString()][SD_SELECTED] = newPath;
}

void AppConfig::forgetDataPath(const std::filesystem::path& path)
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	const auto toRemove = path.string();
	auto& knownPaths = _data[SD_GAME][selectedPlatform().ToStdString()][SD_KNOWN];

	auto it = std::find(knownPaths.cbegin(), knownPaths.cend(), toRemove);

	if (it != knownPaths.end())
		knownPaths.erase(it);
}

void AppConfig::setSelectedPlatformCode(const wxString& newPlatform)
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	_data[SD_GAME][SD_PLATFORM] = newPlatform.ToStdString();
}

#define SD_MM_DEFAULT_LNG_CODE "en_US"
#define SD_MM_DEFAULT_PLATFORM "era2"

void AppConfig::validate()
{
	if (!_data.count(SD_LNG_CODE) || !_data[SD_LNG_CODE].is_string())
		_data[SD_LNG_CODE] = SD_MM_DEFAULT_LNG_CODE;

	if (!_data.count(SD_WINDOW) || !_data[SD_WINDOW].is_object())
		_data[SD_WINDOW] = {};

	if (!_data[SD_WINDOW].count(SD_WIDTH) || !_data[SD_WINDOW][SD_WIDTH].is_number_unsigned() || _data[SD_WINDOW][SD_WIDTH].get<unsigned>() < 600u)
		_data[SD_WINDOW][SD_WIDTH] = 600u;

	if (!_data[SD_WINDOW].count(SD_HEIGHT) || !_data[SD_WINDOW][SD_HEIGHT].is_number_unsigned() || _data[SD_WINDOW][SD_HEIGHT].get<unsigned>() < 560u)
		_data[SD_WINDOW][SD_HEIGHT] = 560u;

	if (!_data[SD_WINDOW].count(SD_LEFT) || !_data[SD_WINDOW][SD_LEFT].is_number_integer())
		_data[SD_WINDOW][SD_LEFT] = 320;

	if (!_data[SD_WINDOW].count(SD_TOP) || !_data[SD_WINDOW][SD_TOP].is_number_integer())
		_data[SD_WINDOW][SD_TOP] = 240;

	if (!_data[SD_WINDOW].count(SD_MAXIMIZED) || !_data[SD_WINDOW][SD_MAXIMIZED].is_boolean())
		_data[SD_WINDOW][SD_MAXIMIZED] = false;

	if (!_data.count(SD_GAME) || !_data[SD_GAME].is_object())
		_data[SD_GAME] = {};

	if (!_data[SD_GAME].count(SD_PLATFORM) || !_data[SD_GAME][SD_PLATFORM].is_string())
		_data[SD_GAME][SD_PLATFORM] = SD_MM_DEFAULT_PLATFORM;

	const std::string selectedPlatform = _data[SD_GAME][SD_PLATFORM];

	if (!_data[SD_GAME][selectedPlatform].count(SD_SELECTED) || !_data[SD_GAME][selectedPlatform][SD_SELECTED].is_string())
		_data[SD_GAME][selectedPlatform][SD_SELECTED] = std::string();

	if (!_data[SD_GAME][selectedPlatform].count(SD_KNOWN) || !_data[SD_GAME][selectedPlatform][SD_KNOWN].is_array())
		_data[SD_GAME][selectedPlatform][SD_KNOWN] = nlohmann::json::array({});

	if (!_data[SD_GAME][selectedPlatform].count(SD_FAVS) || !_data[SD_GAME][selectedPlatform][SD_FAVS].is_array())
		_data[SD_GAME][selectedPlatform][SD_FAVS] = nlohmann::json::array({});
}

void AppConfig::setMainWindowProperties(const MainWindowProperties& props)
{
	_data[SD_WINDOW][SD_MAXIMIZED] = props.maximized;
	if (!props.maximized)
	{
		_data[SD_WINDOW][SD_WIDTH] = props.size.GetWidth();
		_data[SD_WINDOW][SD_HEIGHT] = props.size.GetHeight();
		_data[SD_WINDOW][SD_LEFT] = props.position.x;
		_data[SD_WINDOW][SD_TOP] = props.position.y;
	}
}

MainWindowProperties AppConfig::mainWindow() const
{
	MainWindowProperties result;

	result.maximized = _data[SD_WINDOW][SD_MAXIMIZED];
	result.size.SetWidth(_data[SD_WINDOW][SD_WIDTH]);
	result.size.SetHeight(_data[SD_WINDOW][SD_HEIGHT]);
	result.position.x = _data[SD_WINDOW][SD_LEFT];
	result.position.y = _data[SD_WINDOW][SD_TOP];

	return result;
}

bool AppConfig::dataPathHasStar(const std::filesystem::path& path) const
{
	const auto toStar = path.string();
	auto& knownPaths = _data[SD_GAME][selectedPlatform().ToStdString()][SD_FAVS];

	auto it = std::find(knownPaths.cbegin(), knownPaths.cend(), toStar);
	return it != knownPaths.end();
}

void AppConfig::starDataPath(const std::filesystem::path& path, bool star /*= true*/)
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	const auto toStar = path.string();
	auto& knownPaths = _data[SD_GAME][selectedPlatform().ToStdString()][SD_FAVS];

	auto it = std::find(knownPaths.cbegin(), knownPaths.cend(), toStar);

	MM_EXPECTS(star == (it == knownPaths.end()), unexpected_error); // either star non-starred on remove star from starred

	if (star)
		knownPaths.emplace_back(toStar);
	else
		knownPaths.erase(it);
}

void AppConfig::unstarDataPath(const std::filesystem::path& path)
{
	starDataPath(path, false);
}

std::filesystem::path AppConfig::configFilePath() const
{
	return dataPath() / "settings.json";
}
