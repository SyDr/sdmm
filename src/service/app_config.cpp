// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "app_config.hpp"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include <wx/log.h>
#include <wx/stdpaths.h>

#include "type/main_window_properties.h"
#include "utility/sdlexcept.h"
#include "system_info.hpp"

using namespace mm;

namespace
{
	std::variant<PortableMode, MainMode> constructProgramMode()
	{
		fs::path result(wxStandardPaths::Get().GetDataDir().ToStdString(wxConvUTF8));
		result = result.parent_path().parent_path() / "_MM_Data";
		if (exists(result) && is_directory(result))
			return PortableMode(result.make_preferred());

		result = wxStandardPaths::Get().GetUserDataDir().ToStdString(wxConvUTF8);

		return MainMode(result.make_preferred());
	}

	struct IsPortable
	{
		bool operator()(const PortableMode&)
		{
			return true;
		}

		bool operator()(const MainMode&)
		{
			return false;
		}
	};

	struct DataPath
	{
		fs::path operator()(const PortableMode& pm)
		{
			return pm.managedPath;
		}

		fs::path operator()(const MainMode& mm)
		{
			return mm.programDataPath;
		}
	};

	struct ValidateMode
	{
		void operator()(const PortableMode&)
		{
		}

		void operator()(const MainMode& mm)
		{
			if (!exists(mm.programDataPath))
				create_directories(mm.programDataPath);
		}
	};
}

AppConfig::AppConfig()
	: _mode(constructProgramMode())
{
	std::visit(ValidateMode(), _mode);

	boost::nowide::ifstream datafile(configFilePath());

	if (datafile)
	{
		_data = nlohmann::json::parse(datafile, nullptr, false);

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
	return std::visit(IsPortable(), _mode);
}

fs::path AppConfig::dataPath() const
{
	return std::visit(DataPath(), _mode);
}

fs::path AppConfig::programPath() const
{
	return fs::path(wxStandardPaths::Get().GetDataDir().ToStdString(wxConvUTF8));
}

#define SD_LNG_CODE "language"
constexpr auto sd_game = "game";
#define SD_KNOWN     "directories"
#define SD_FAVS      "stars"
#define SD_PLATFORM  "platform"
#define SD_SELECTED  "selected"
#define SD_WINDOW    "window"
#define SD_WIDTH     "width"
#define SD_HEIGHT    "height"
#define SD_LEFT      "left"
#define SD_TOP       "top"
#define SD_MAXIMIZED "maximized"

std::string AppConfig::currentLanguageCode() const
{
	return _data[SD_LNG_CODE].get<std::string>();
}

void AppConfig::setCurrentLanguageCode(const std::string& lngCode)
{
	_data[SD_LNG_CODE] = lngCode;
}

void AppConfig::save()
{
	boost::nowide::ofstream datafile(configFilePath());
	datafile << _data.dump(2);
}

std::vector<fs::path> AppConfig::getKnownDataPathList() const
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	std::vector<fs::path> result;
	for (const auto& path :
		 _data[sd_game][selectedPlatform()][SD_KNOWN].get<std::vector<std::string>>())
		result.emplace_back(path);

	return result;
}

std::string AppConfig::selectedPlatform() const
{
	return _data[sd_game][SD_PLATFORM].get<std::string>();
}

fs::path AppConfig::getDataPath() const
{
	if (!portableMode())
		return _data[sd_game][selectedPlatform()][SD_SELECTED].get<std::string>();

	return dataPath().parent_path();
}

void AppConfig::setDataPath(const fs::path& path)
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	auto& knownPaths = _data[sd_game][selectedPlatform()][SD_KNOWN];

	if (std::find(knownPaths.begin(), knownPaths.end(), path.string()) == knownPaths.end())
		_data[sd_game][selectedPlatform()][SD_KNOWN].emplace_back(path.string());

	_data[sd_game][selectedPlatform()][SD_SELECTED] = path.string();
}

void AppConfig::forgetDataPath(const fs::path& path)
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	auto& knownPaths = _data[sd_game][selectedPlatform()][SD_KNOWN];

	auto it = std::find(knownPaths.begin(), knownPaths.end(), path.string());

	if (it != knownPaths.end())
		knownPaths.erase(it);
}

void AppConfig::setSelectedPlatformCode(const std::string& newPlatform)
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	_data[sd_game][SD_PLATFORM] = newPlatform;
}

#define SD_MM_DEFAULT_PLATFORM "era2"

void AppConfig::validate()
{
	if (!_data.count(SD_LNG_CODE) || !_data[SD_LNG_CODE].is_string())
		_data[SD_LNG_CODE] = SystemInfo::DefaultLanguage;

	if (!_data.count(SD_WINDOW) || !_data[SD_WINDOW].is_object())
		_data[SD_WINDOW] = {};

	if (!_data[SD_WINDOW].count(SD_WIDTH) || !_data[SD_WINDOW][SD_WIDTH].is_number_unsigned() ||
		_data[SD_WINDOW][SD_WIDTH].get<unsigned>() < 600u)
		_data[SD_WINDOW][SD_WIDTH] = 600u;

	if (!_data[SD_WINDOW].count(SD_HEIGHT) || !_data[SD_WINDOW][SD_HEIGHT].is_number_unsigned() ||
		_data[SD_WINDOW][SD_HEIGHT].get<unsigned>() < 560u)
		_data[SD_WINDOW][SD_HEIGHT] = 560u;

	if (!_data[SD_WINDOW].count(SD_LEFT) || !_data[SD_WINDOW][SD_LEFT].is_number_integer())
		_data[SD_WINDOW][SD_LEFT] = 320;

	if (!_data[SD_WINDOW].count(SD_TOP) || !_data[SD_WINDOW][SD_TOP].is_number_integer())
		_data[SD_WINDOW][SD_TOP] = 240;

	if (!_data[SD_WINDOW].count(SD_MAXIMIZED) || !_data[SD_WINDOW][SD_MAXIMIZED].is_boolean())
		_data[SD_WINDOW][SD_MAXIMIZED] = false;

	if (!_data.count(sd_game) || !_data[sd_game].is_object())
		_data[sd_game] = {};

	if (!_data[sd_game].count(SD_PLATFORM) || !_data[sd_game][SD_PLATFORM].is_string())
		_data[sd_game][SD_PLATFORM] = SD_MM_DEFAULT_PLATFORM;

	const std::string selectedPlatform = _data[sd_game][SD_PLATFORM];

	if (!_data[sd_game][selectedPlatform].count(SD_SELECTED) ||
		!_data[sd_game][selectedPlatform][SD_SELECTED].is_string())
		_data[sd_game][selectedPlatform][SD_SELECTED] = std::string();

	if (!_data[sd_game][selectedPlatform].count(SD_KNOWN) ||
		!_data[sd_game][selectedPlatform][SD_KNOWN].is_array())
		_data[sd_game][selectedPlatform][SD_KNOWN] = nlohmann::json::array({});

	if (!_data[sd_game][selectedPlatform].count(SD_FAVS) ||
		!_data[sd_game][selectedPlatform][SD_FAVS].is_array())
		_data[sd_game][selectedPlatform][SD_FAVS] = nlohmann::json::array({});
}

void AppConfig::setMainWindowProperties(const MainWindowProperties& props)
{
	_data[SD_WINDOW][SD_MAXIMIZED] = props.maximized;
	if (!props.maximized)
	{
		_data[SD_WINDOW][SD_WIDTH]  = props.size.GetWidth();
		_data[SD_WINDOW][SD_HEIGHT] = props.size.GetHeight();
		_data[SD_WINDOW][SD_LEFT]   = props.position.x;
		_data[SD_WINDOW][SD_TOP]    = props.position.y;
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

bool AppConfig::dataPathHasStar(const fs::path& path) const
{
	auto& knownPaths = _data[sd_game][selectedPlatform()][SD_FAVS];

	auto it = std::find(knownPaths.begin(), knownPaths.end(), path.string());
	return it != knownPaths.end();
}

void AppConfig::starDataPath(const fs::path& path, bool star /*= true*/)
{
	MM_EXPECTS(!portableMode(), unexpected_error);

	auto&      knownPaths = _data[sd_game][selectedPlatform()][SD_FAVS];

	auto it = std::find(knownPaths.begin(), knownPaths.end(), path.string());

	MM_EXPECTS(star == (it == knownPaths.end()),
			   unexpected_error);  // either star non-starred on remove star from starred

	if (star)
		knownPaths.emplace_back(path.string());
	else
		knownPaths.erase(it);
}

void AppConfig::unstarDataPath(const fs::path& path)
{
	starDataPath(path, false);
}

fs::path AppConfig::configFilePath() const
{
	return dataPath() / "settings.json";
}
