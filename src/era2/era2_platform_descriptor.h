// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/domain/iplatform_descriptor.h"

namespace mm
{
	struct Application;

	class Era2PlatfromDescriptor : public IPlatformDescriptor
	{
	public:
		explicit Era2PlatfromDescriptor(const Application& app);

		wxString getId() const override;
		wxString getPlatformName() const override;

		wxIcon getIcon() const override;

		std::unique_ptr<IModPlatform> create() const override;

	private:
		const Application& _app;
	};
}
