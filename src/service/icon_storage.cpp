// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "icon_storage.hpp"

#include <wx/icon.h>
#include <wx/log.h>

#include "type/icon.hpp"
#include "type/interface_size.hpp"

using namespace mm;

std::size_t hash_value(const wxSize& v)
{
	boost::hash<int> hasher;

	auto r = hasher(v.GetWidth());

	boost::hash_combine(r, hasher(v.GetHeight()));

	return r;
}

namespace
{
	wxString toPath(Icon::Stock value)
	{
		return wxString(L"icons/") + wxString::FromUTF8(std::string(magic_enum::enum_name(value))) + L".svg";
	}

	wxIcon loadSvgIcon(Icon::Stock location, const wxSize& targetSize)
	{
		wxIcon icon;
		icon.CopyFromBitmap(wxBitmapBundle::FromSVGFile(toPath(location), targetSize).GetBitmap(targetSize));

		return icon;
	}

	wxIcon loadNormalIcon(const wxString& location)
	{
		wxIcon icon(location, wxBITMAP_TYPE_ANY);

		if (!icon.IsOk())
			icon = wxIcon(location, wxBITMAP_TYPE_ICO);

		return icon;
	}

	wxBitmap loadImpl(
		IconStorage::IconCache& cache, const IconStorage::IconLocation& location, const wxSize& targetSize)
	{
		const auto it = cache.find({ location, targetSize });

		if (it != std::end(cache))
			return it->second;

		wxLogNull noLogging;  // suppress wxWidgets messages about inability to load icon

		wxIcon icon;
		if (std::holds_alternative<Icon::Stock>(location))
			icon = loadSvgIcon(std::get<Icon::Stock>(location), targetSize);
		else
		{
			const auto path = wxString::FromUTF8(std::get<std::string>(location));
			icon = loadNormalIcon(path);
		}

		if (!icon.IsOk())
			return loadImpl(cache, Icon::Stock::blank, targetSize);

		if (icon.GetSize() != targetSize)
			icon.CopyFromBitmap(wxBitmap(wxBitmap(icon).ConvertToImage().Rescale(
				targetSize.GetWidth(), targetSize.GetHeight(), wxIMAGE_QUALITY_NEAREST)));

		return cache[{ location, targetSize }] = icon;
	}
}

mm::IconStorage::IconStorage(InterfaceSize interfaceSize)
	: _defaultSize(toIconPredefinedSize(interfaceSize))
{}

wxBitmap IconStorage::get(Icon::Stock icon, std::optional<Icon::Size> targetSize)
{
	return loadImpl(_iconCache, icon, iconSize(targetSize.value_or(_defaultSize)));
}

wxBitmap IconStorage::get(const std::string& name, std::optional<Icon::Size> resizeTo)
{
	return loadImpl(_iconCache, name, iconSize(resizeTo.value_or(_defaultSize)));
}
