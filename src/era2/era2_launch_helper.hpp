// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/ilaunch_helper.hpp"

#include "era2_config.hpp"

namespace mm
{
	struct IIconStorage;

	struct Era2LaunchHelper : ILaunchHelper
	{
		Era2LaunchHelper(Era2Config& config, IIconStorage& iconStorage);

		[[nodiscard]] std::string getExecutable() const override;
		void                      setExecutable(const std::string& executable) override;

		[[nodiscard]] wxIcon      getIcon() const override;
		[[nodiscard]] std::string getCaption() const override;
		[[nodiscard]] std::string getLaunchString() const override;

		[[nodiscard]] sigslot::signal<>& onDataChanged() override;

	private:
		IIconStorage& _iconStorage;
		Era2Config&   _config;

		sigslot::signal<> _dataChanged;
	};
}
