// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_launch_helper.hpp"

#include <wx/icon.h>

#include "application.h"
#include "interface/iicon_storage.hpp"

using namespace mm;

Era2LaunchHelper::Era2LaunchHelper(Era2Config& config)
	: _config(config)
{}

std::string Era2LaunchHelper::getCaption() const
{
	if (auto result = getExecutable(); !result.empty())
		return result;

	return "message/info/exe_not_selected"_lng.ToStdString(wxConvUTF8);
}

std::string Era2LaunchHelper::getLaunchString() const
{
	return _config.getLaunchString();
}

sigslot::signal<>& Era2LaunchHelper::onDataChanged()
{
	return _dataChanged;
}

std::string Era2LaunchHelper::getExecutable() const
{
	return _config.getExecutable();
}

void Era2LaunchHelper::setExecutable(const std::string& executable)
{
	_config.setExecutable(executable);
	_dataChanged();
}
