// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2/era2_platform_descriptor.h"
#include "utility/sdlexcept.h"
#include "domain/imod_platform.hpp"
#include "interface/domain/iplatform_descriptor.h"
#include "platform_service.h"

using namespace mm;

PlatformService::PlatformService(Application const& app)
{
	_platforms.emplace_back(std::make_unique<Era2PlatfromDescriptor>(app));
}

std::deque<IPlatformDescriptor*> PlatformService::availablePlatforms() const
{
	std::deque<IPlatformDescriptor*> result;

	for (const auto& platform : _platforms)
		result.emplace_back(platform.get());

	return result;
}

std::unique_ptr<IModPlatform> PlatformService::create(wxString const& platformId) const
{
	auto const it = std::find_if(_platforms.cbegin(), _platforms.cend(), [&](std::unique_ptr<IPlatformDescriptor> const& item) {
		return item->getId() == platformId;
	});

	MM_EXPECTS(it != _platforms.cend(), unexpected_error);

	return (*it)->create();
}
