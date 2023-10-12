// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/iplatform_descriptor.hpp"

namespace mm
{
	struct Application;

	class Era2PlatfromDescriptor : public IPlatformDescriptor
	{
	public:
		explicit Era2PlatfromDescriptor(const Application& app);

		[[nodiscard]] std::string getId() const override;
		[[nodiscard]] std::string getPlatformName() const override;

		[[nodiscard]] wxIcon getIcon() const override;

		std::unique_ptr<IModPlatform> create() const override;

	private:
		const Application& _app;
	};
}
