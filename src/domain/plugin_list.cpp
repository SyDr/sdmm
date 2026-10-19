// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include <boost/locale/conversion.hpp>

#include "plugin_list.hpp"

using namespace mm;

Plugin::Plugin(std::string modId, fs::path from)
	: modId { boost::locale::fold_case(std::move(modId)) }
	, source { .v = from }
{
	if (from.extension() == ".off")
		from = from.parent_path() / from.filename().replace_extension();

	id.v = boost::locale::fold_case(from.string());
}
