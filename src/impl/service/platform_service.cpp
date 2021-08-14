// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2/era2_platform_descriptor.h"
#include "utility/sdlexcept.h"
#include "interface/imod_platform.hpp"
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

std::unique_ptr<IModPlatform> PlatformService::create(const wxString& platformId) const
{
	const auto it = std::find_if(_platforms.cbegin(), _platforms.cend(), [&](const std::unique_ptr<IPlatformDescriptor>& item) {
		return item->getId() == platformId;
	});

	MM_EXPECTS(it != _platforms.cend(), unexpected_error);

	return (*it)->create();
}
