// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "program_update_helper.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "sdlexcept.h"
#include "version.hpp"

namespace
{
	// TODO: make flexible urls
	inline constexpr const auto LastRelease = "https://api.github.com/repos/SyDr/sdmm/releases/latest";
}

using namespace mm;

UpdateCheckHelper::~UpdateCheckHelper()
{
	clear();
}

void UpdateCheckHelper::clear()
{
	stop();
	_latestRelease = {};
}

void UpdateCheckHelper::stop()
{
	_thread.get_stop_source().request_stop();
}

void UpdateCheckHelper::checkForUpdate(std::function<void(nlohmann::json)> callback)
{
	stop();
	if (_thread.joinable())
		_thread.join();

	MM_PRECONDTION(!_thread.joinable());

	_thread = std::jthread([=](std::stop_token stopToken) {
		cpr::Response r =
			cpr::Get(cpr::Url(LastRelease), cpr::Header { { "Accept", "application/vnd.github+json" },
												{ "X-GitHub-Api-Version", "2022-11-28" } });

		if (stopToken.stop_requested())
			return;

		MM_PRECONDTION(r.status_code == 200);

		{
			std::lock_guard lg(_dataMutex);
			_latestRelease = nlohmann::json::parse(r.text);
		}

		callback(_latestRelease);
	});
}

void UpdateCheckHelper::downloadUpdate()
{
	// TODO:
}

void UpdateCheckHelper::installUpdate()
{
	// TODO:
}
