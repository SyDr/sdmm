// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "plugin_list.hpp"

using namespace mm;

PluginSource::PluginSource(const wxString& modId, PluginLocation location, const wxString& name)
	: modId(modId)
	, location(location)
	, name(name)
{}
