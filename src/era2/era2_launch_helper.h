// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/ilaunch_helper.h"

#include "era2_config.h"

namespace mm
{
	struct IIconStorage;
	struct Era2LaunchHelper : ILaunchHelper
	{
		Era2LaunchHelper(Era2Config& config, IIconStorage& iconStorage);

		std::string getExecutable() const override;
		void        setExecutable(const std::string& executable) override;

		wxIcon   getIcon() const override;
		wxString getCaption() const override;
		wxString getLaunchString() const override;

		sigslot::signal<>& onDataChanged() override;

	private:
		IIconStorage& _iconStorage;
		Era2Config&   _config;

		sigslot::signal<> _dataChanged;
	};
}
