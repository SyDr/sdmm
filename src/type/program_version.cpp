// SD Mod Manager

// Copyright (c) 2025-2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "program_version.hpp"
#include "version.hpp"

#include <boost/convert.hpp>
#include <boost/convert/lexical_cast.hpp>

using namespace mm;

struct boost::cnv::by_default : boost::cnv::lexical_cast
{};

ProgramVersion::ProgramVersion(std::string_view version)
{
	std::vector<std::string_view> v;
	boost::split(v, version, boost::is_any_of("."));

	if (v.size() >= 3)
	{
		if (!v[0].empty() && v[0].front() == 'v')
			v[0] = v[0].substr(1);

		major = boost::convert<size_t>(v[0]).value_or(0);
		minor = boost::convert<size_t>(v[1]).value_or(0);
		patch = boost::convert<size_t>(v[2]).value_or(0);
	}
}

ProgramVersion::ProgramVersion(size_t major, size_t minor, size_t patch)
	: major(major)
	, minor(minor)
	, patch(patch)
{}

ProgramVersion ProgramVersion::current()
{
	return ProgramVersion(PROGRAM_VERSION_MAJOR, PROGRAM_VERSION_MINOR, PROGRAM_VERSION_PATCH);
}
