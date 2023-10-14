// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/ipreset_manager.hpp"

#include "type/filesystem.hpp"

namespace mm
{
	struct Era2Platform;

	struct Era2PresetManager : IPresetManager
	{
		explicit Era2PresetManager(fs::path rootPath, fs::path modsPath);

		PresetData     loadPreset(const std::string& name) override;
		nlohmann::json savePreset(const PresetData& preset) override;
		void           savePreset(const std::string& name, const PresetData& preset) override;

		std::set<std::string> list() const override;

		bool exists(const std::string& name) const override;
		void copy(const std::string& from, const std::string& to) override;
		void rename(const std::string& from, const std::string& to) override;
		void remove(const std::string& name) override;

		sigslot::signal<>& onListChanged() override;

	private:
		const fs::path _rootPath;
		const fs::path _modsPath;

		sigslot::signal<> _listChanged;
	};
}
