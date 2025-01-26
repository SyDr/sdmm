// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "app_config.hpp"

#include "system_info.hpp"
#include "type/interface_size.hpp"
#include "type/main_window_properties.h"
#include "type/mod_description_used_control.hpp"
#include "type/update_check_mode.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include <boost/convert.hpp>
#include <boost/convert/lexical_cast.hpp>
#include <wx/log.h>
#include <wx/stdpaths.h>

using namespace mm;

struct boost::cnv::by_default : boost::cnv::lexical_cast
{};

namespace
{
	std::variant<PortableMode, MainMode> constructProgramMode()
	{
		fs::path myDir(wxStandardPaths::Get().GetDataDir().ToStdString(wxConvUTF8));

		if (exists(myDir / SystemInfo::BaseDirFile) && is_regular_file(myDir / SystemInfo::BaseDirFile))
		{
			return PortableMode(
				(myDir / readFile(myDir / SystemInfo::BaseDirFile) / SystemInfo::AppDataDirectory)
					.lexically_normal(),
				myDir / SystemInfo::SettingsFile);
		}

		myDir = wxStandardPaths::Get().GetUserDataDir().ToStdString(wxConvUTF8);
		return MainMode(myDir.lexically_normal());
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
		void operator()(const PortableMode& mm)
		{
			if (!exists(mm.managedPath))
				create_directories(mm.managedPath);
		}

		void operator()(const MainMode& mm)
		{
			if (!exists(mm.programDataPath))
				create_directories(mm.programDataPath);
		}
	};

	struct ConfigPath
	{
		fs::path operator()(const PortableMode& mm)
		{
			return mm.configLocation;
		}

		fs::path operator()(const MainMode& mm)
		{
			return mm.programDataPath / SystemInfo::SettingsFile;
		}
	};

	struct ProgramVersion
	{
		size_t major = 0;
		size_t minor = 0;
		size_t patch = 0;

		ProgramVersion(std::string_view version)
		{
			std::vector<std::string_view> v;
			boost::split(v, version, boost::is_any_of("."));

			if (v.size() == 3)
			{
				major = boost::convert<size_t>(v[0]).value_or(0);
				minor = boost::convert<size_t>(v[1]).value_or(0);
				patch = boost::convert<size_t>(v[2]).value_or(0);
			}
		}

		ProgramVersion(size_t major, size_t minor, size_t patch)
			: major(major)
			, minor(minor)
			, patch(patch)
		{}

		auto operator<=>(const ProgramVersion& r) const = default;
		bool operator==(const ProgramVersion& r) const = default;
	};
}

AppConfig::AppConfig()
	: _mode(constructProgramMode())
{
	std::visit(ValidateMode(), _mode);

	_data = loadJsonFromFile(configFilePath(), true);

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

namespace
{
	namespace Key
	{
		inline constexpr const auto MMVersion                 = "mm_version";
		inline constexpr const auto UpdateCheckMode           = "update_check_mode";
		inline constexpr const auto LastCheckForUpdateOn      = "last_check_for_update_on";
		inline constexpr const auto ModDescriptionUsedControl = "mod_description_use_control";
		inline constexpr const auto InterfaceSize             = "interface_size";
	}
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
	for (const auto& path : _data[sd_game][selectedPlatform()][SD_KNOWN].get<std::vector<std::string>>())
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
		return fs::path(_data[sd_game][selectedPlatform()][SD_SELECTED].get<std::string>())
			.lexically_normal();

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
	if (_data.is_discarded())
		_data = { { Key::MMVersion, PROGRAM_VERSION_BASE } };

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

	const auto selectedPlatform = _data[sd_game][SD_PLATFORM].get<std::string>();

	if (!_data[sd_game][selectedPlatform].count(SD_SELECTED) ||
		!_data[sd_game][selectedPlatform][SD_SELECTED].is_string())
		_data[sd_game][selectedPlatform][SD_SELECTED] = std::string();

	if (!_data[sd_game][selectedPlatform].count(SD_KNOWN) ||
		!_data[sd_game][selectedPlatform][SD_KNOWN].is_array())
		_data[sd_game][selectedPlatform][SD_KNOWN] = nlohmann::json::array({});

	if (!_data[sd_game][selectedPlatform].count(SD_FAVS) ||
		!_data[sd_game][selectedPlatform][SD_FAVS].is_array())
		_data[sd_game][selectedPlatform][SD_FAVS] = nlohmann::json::array({});

	auto simpleEnumCheck = [&](const auto& key, const auto& defaultValue, const auto& lastValue) {
		if (!_data.count(key) || !_data[key].is_number_unsigned())
			_data[key] = static_cast<int>(defaultValue);

		if (_data[key] < 0 || _data[key] > lastValue)
			_data[key] = static_cast<int>(defaultValue);
	};

	simpleEnumCheck(Key::UpdateCheckMode, UpdateCheckMode::once_per_week, UpdateCheckMode::once_per_month);

	if (!_data.count(Key::LastCheckForUpdateOn) || !_data[Key::LastCheckForUpdateOn].is_string())
		_data[Key::LastCheckForUpdateOn] = std::string();

	simpleEnumCheck(Key::ModDescriptionUsedControl, ModDescriptionUsedControl::use_wxhtml_control,
		ModDescriptionUsedControl::use_plain_text_control);

	simpleEnumCheck(Key::InterfaceSize, InterfaceSize::big, InterfaceSize::big);

	if (!_data.count(Key::MMVersion) || !_data[Key::MMVersion].is_string())
		_data[Key::MMVersion] = "";

	auto cfgVersion = ProgramVersion(_data[Key::MMVersion].get<std::string>());
	if (cfgVersion <= ProgramVersion(0, 98, 53))
	{
		// 0.98.55: wxHtmlWindow is now used by default instead
		if (modDescriptionUsedControl() == ModDescriptionUsedControl::try_to_use_webview2)
			modDescriptionUsedControl(ModDescriptionUsedControl::use_wxhtml_control);
	}

	if (ProgramVersion(PROGRAM_VERSION_BASE) > cfgVersion)
		_data[Key::MMVersion] = PROGRAM_VERSION_BASE;
}

UpdateCheckMode AppConfig::updateCheckMode() const
{
	return static_cast<UpdateCheckMode>(_data[Key::UpdateCheckMode]);
}

void AppConfig::updateCheckMode(UpdateCheckMode value)
{
	_data[Key::UpdateCheckMode] = static_cast<int>(value);
}

time_point AppConfig::lastUpdateCheck() const
{
	const auto         s = _data[Key::LastCheckForUpdateOn].get<std::string>();
	std::istringstream in(s);

	time_point tp;
	in >> std::chrono::parse(TimeInputFormat, tp);

	return tp;
}

void AppConfig::lastUpdateCheck(time_point value)
{
	_data[Key::LastCheckForUpdateOn] = std::format(TimeOutputFormat, value);
}

ModDescriptionUsedControl AppConfig::modDescriptionUsedControl() const
{
	return static_cast<ModDescriptionUsedControl>(_data[Key::ModDescriptionUsedControl]);
}

bool AppConfig::modDescriptionUsedControl(ModDescriptionUsedControl value)
{
	if (value == modDescriptionUsedControl())
		return false;

	_data[Key::ModDescriptionUsedControl] = static_cast<int>(value);

	return true;
}

InterfaceSize AppConfig::interfaceSize() const
{
	return static_cast<InterfaceSize>(_data[Key::InterfaceSize]);
}

bool AppConfig::interfaceSize(InterfaceSize value)
{
	if (value == interfaceSize())
		return false;

	_data[Key::InterfaceSize] = static_cast<int>(value);

	return true;
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

	result.maximized = _data[SD_WINDOW][SD_MAXIMIZED].get<bool>();
	result.size.SetWidth(_data[SD_WINDOW][SD_WIDTH].get<unsigned>());
	result.size.SetHeight(_data[SD_WINDOW][SD_HEIGHT].get<unsigned>());
	result.position.x = _data[SD_WINDOW][SD_LEFT].get<int>();
	result.position.y = _data[SD_WINDOW][SD_TOP].get<int>();

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

	auto& knownPaths = _data[sd_game][selectedPlatform()][SD_FAVS];

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
	return std::visit(ConfigPath(), _mode);
}
