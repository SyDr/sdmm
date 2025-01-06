// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <chrono>
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
		std::chrono::sys_seconds _nextRequestCanBeMadeAt;

		std::jthread    _thread;
		std::stop_token _stopToken;
	};
}
