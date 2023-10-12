// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <stdexcept>
#include <type_traits>
#include <wx/cpp.h>

#include "ui/error_view.h"

namespace mm
{
	class empty_path_error : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

	class not_exist_path_error : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

	class no_parent_window_error : public std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	class unexpected_error : public std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	class condtion_error : public std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};
}

#define MM_EXPECTS(what, exception_type) \
	if (!(what))                         \
		throw_with_trace(exception_type(wxSTRINGIZE(what)));

#define MM_PRECONDTION(what) \
	if (!(what))             \
		throw_with_trace(mm::condtion_error(wxSTRINGIZE(what)));

#define EX_TRY \
	try        \
	{

#define EX_ON_EXCEPTION(type, handler) \
	}                                  \
	catch (const type& e)              \
	{                                  \
		handler(e);

#define EX_UNEXPECTED       \
	}                       \
	catch (...)             \
	{                       \
		ErrorView ev(this); \
		ev.ShowModal();     \
	};

#define SINK_EXCEPTION(handler) [=](const auto&) { handler(); }
