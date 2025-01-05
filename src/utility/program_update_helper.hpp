// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <string>

namespace mm
{
	struct UpdateCheckHelper
	{
		~UpdateCheckHelper();

		void clear();
		void stop();

		void checkForUpdate(std::function<void(nlohmann::json)> callback);
		void downloadUpdate();
		void installUpdate();

	private:
		std::jthread    _thread;
		std::stop_token _stopToken;

		std::mutex     _dataMutex;
		nlohmann::json _latestRelease;
	};
}
