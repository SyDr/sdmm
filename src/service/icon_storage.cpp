// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "icon_storage.hpp"

#include <wx/icon.h>
#include <wx/log.h>

#include "type/embedded_icon.h"

using namespace mm;

std::size_t hash_value(const wxSize& v)
{
	boost::hash<int> hasher;

	auto r = hasher(v.GetWidth());

	boost::hash_combine(r, hasher(v.GetHeight()));

	return r;
}

wxBitmap IconStorage::get(const std::string& name, const wxSize& targetSize)
{
	if (name.empty())
		return get("icons/blank.svg", targetSize);

	const auto it = _iconCache.find({ name, targetSize });

	if (it != std::end(_iconCache))
		return it->second;

	wxLogNull noLogging;  // suppress wxWidgets messages about inability to load icon

	const auto path = wxString::FromUTF8(name);

	wxIcon icon;

	if (path.ends_with(L".svg"))
		icon.CopyFromBitmap(wxBitmapBundle::FromSVGFile(path, targetSize).GetBitmap(targetSize));

	if (!icon.IsOk())
		icon = wxIcon(path, wxBITMAP_TYPE_ANY);

	if (!icon.IsOk())
		icon = wxIcon(path, wxBITMAP_TYPE_ICO);

	if (!icon.IsOk())
	{
		if (name != "icons/blank.svg")
			return get("icons/blank.svg");

		return wxNullIcon;
	}

	if (icon.GetSize() != targetSize)
		icon.CopyFromBitmap(wxBitmap(wxBitmap(icon).ConvertToImage().Rescale(
			targetSize.GetWidth(), targetSize.GetHeight(), wxIMAGE_QUALITY_NEAREST)));

	return _iconCache[{ name, targetSize }] = icon;
}

wxBitmap IconStorage::get(IconPredefined icon, IconPredefinedSize targetSize)
{
	std::string name = "icons/";
	wxSize      size = { 16, 16 };

	switch (icon)
	{
	case IconPredefined::blank: name += "blank.png"; break;
	case IconPredefined::circle: name += "circle.svg"; break;
	default: break;
	}

	switch (targetSize)
	{
	case IconPredefinedSize::x16: size = { 16, 16 }; break;
	case IconPredefinedSize::x24: size = { 24, 24 }; break;
	case IconPredefinedSize::x32: size = { 32, 32 }; break;
	case IconPredefinedSize::x48: size = { 48, 48 }; break;
	case IconPredefinedSize::x64: size = { 64, 64 }; break;
	}

	return get(name, size);
}

wxBitmap IconStorage::get(const std::string& name)
{
	return get(name, { 16, 16 });
}
