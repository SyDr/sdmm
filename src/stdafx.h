// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

// stl
#include <atomic>
#include <compare>
#include <deque>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// boost
#include <boost/algorithm/string.hpp>
#include <boost/exception/all.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/convert.hpp>

#define BOOST_FILESYSTEM_VERSION 4
#include <boost/filesystem.hpp>

// wxWidgets
#include <wx/dataview.h>
#include <wx/wx.h>

// must be included after wxWidgets
#include <boost/stacktrace.hpp>

// rest
#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include <nlohmann/json.hpp>
#include <sigslot/signal.hpp>

namespace mm
{
	using traced = boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>;

	template <class T>
	void throw_with_trace(const T& t)
	{
		throw boost::enable_error_info(t) << traced(boost::stacktrace::stacktrace());
	}
}
