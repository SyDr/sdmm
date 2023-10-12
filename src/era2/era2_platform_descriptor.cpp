// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_platform_descriptor.hpp"

#include "era2/era2_platform.h"
#include "interface/iapp_config.hpp"
#include "application.h"
#include "interface/ii18n_service.hpp"
#include "utility/sdlexcept.h"

using namespace mm;

Era2PlatfromDescriptor::Era2PlatfromDescriptor(Application const& app)
	: _app(app)
{}

std::string Era2PlatfromDescriptor::getId() const
{
	return "era2";
}

std::string Era2PlatfromDescriptor::getPlatformName() const
{
	return "Era II (HoMM III: In the Wake of Gods)";
}

std::unique_ptr<IModPlatform> Era2PlatfromDescriptor::create() const
{
	const auto path = _app.appConfig().getDataPath();

	MM_EXPECTS(!path.empty(), empty_path_error);
	MM_EXPECTS(exists(path), not_exist_path_error);

	return std::make_unique<Era2Platform>(_app);
}

wxIcon Era2PlatfromDescriptor::getIcon() const
{
	return wxNullIcon;
}
