// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/conflict_resolve_mode.hpp"

namespace mm
{
	struct ILocalConfig
	{
		virtual ~ILocalConfig() = default;

		virtual void save() = 0;

		virtual std::filesystem::path getDataPath() const = 0;
		virtual std::filesystem::path getTempPath() const = 0;

		virtual bool showHiddenMods() const     = 0;
		virtual void showHiddenMods(bool value) = 0;

		virtual ConflictResolveMode conflictResolveMode() const                    = 0;
		virtual void                conflictResolveMode(ConflictResolveMode value) = 0;

		virtual wxString getAcitvePreset() const                 = 0;
		virtual void     setActivePreset(const wxString& preset) = 0;
	};
}
