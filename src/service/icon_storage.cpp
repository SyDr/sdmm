// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "icon_storage.hpp"

#include <wx/log.h>
#include <wx/icon.h>

#include "type/embedded_icon.h"

using namespace mm;

wxIcon IconStorage::get(const std::string& name)
{
	if (name.empty())
		return get(embedded_icon::blank);

	const auto it = _cache.find(name);

	if (it != std::end(_cache))
		return it->second;

	wxLogNull noLogging; // suppress wxWidgets messages about inability to load icon

	const auto path = wxString::FromUTF8(name);

	wxIcon icon(path, wxBITMAP_TYPE_PNG);
	if (!icon.IsOk())
		icon = wxIcon(path, wxBITMAP_TYPE_ICO);

	if (!icon.IsOk())
	{
		if (name != embedded_icon::blank)
			return get(embedded_icon::blank);

		return wxNullIcon;
	}

	if (icon.GetSize() != wxSize(16, 16))
		icon.CopyFromBitmap(wxBitmap(wxBitmap(icon).ConvertToImage().Rescale(16, 16, wxIMAGE_QUALITY_NEAREST)));

	return _cache[name] = icon;
}
