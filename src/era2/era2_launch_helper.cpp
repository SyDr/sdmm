// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_launch_helper.h"

#include <wx/icon.h>
#include <wx/string.h>

#include "interface/iicon_storage.h"
#include "mod_manager_app.h"

using namespace mm;

Era2LaunchHelper::Era2LaunchHelper(Era2Config& config, IIconStorage& iconStorage)
	: _config(config)
	, _iconStorage(iconStorage)
{}

wxIcon Era2LaunchHelper::getIcon() const
{
	return _iconStorage.get(getLaunchString());
}

wxString Era2LaunchHelper::getCaption() const
{
	auto result = getExecutable();
	if (result.empty())
		result = "not selected"_lng.ToStdString();

	return result;
}

wxString Era2LaunchHelper::getLaunchString() const
{
	return _config.getLaunchString();
}

sigslot::signal<>& Era2LaunchHelper::onDataChanged()
{
	return _dataChanged;
}

std::string Era2LaunchHelper::getExecutable() const
{
	return _config.getExecutable().ToStdString();
}

void Era2LaunchHelper::setExecutable(const std::string& executable)
{
	_config.setExecutable(executable);
	_dataChanged();
}
