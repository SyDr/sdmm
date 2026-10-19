// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "icon_storage.hpp"

#include <wx/dcgraph.h>
#include <wx/icon.h>
#include <wx/log.h>

#include "type/filesystem.hpp"
#include "type/icon.hpp"
#include "type/interface_size.hpp"
#include "utility/fs_util.h"

#include <format>

using namespace mm;

std::size_t hash_value(const wxSize& v)
{
	boost::hash<int> hasher;

	auto r = hasher(v.GetWidth());

	boost::hash_combine(r, hasher(v.GetHeight()));

	return r;
}

std::size_t hash_value(const wxColour& v)
{
	boost::hash<int> hasher;

	auto r = hasher(v.Red());

	boost::hash_combine(r, hasher(v.Green()));
	boost::hash_combine(r, hasher(v.Blue()));

	return r;
}

namespace
{
	wxIcon loadSvgIcon(Icon::Stock location, const wxSize& targetSize, std::optional<wxColour> targetColor)
	{
		const auto path = fs::path("icons") / (std::string(magic_enum::enum_name(location)) + ".svg");

		std::vector<unsigned char> data = readBinary(path);
		if (targetColor)
		{
			const std::string from = "\"#0000FF\"";
			const std::string to = "\"" +
								   std::format("#{:02X}{:02X}{:02X}", targetColor->Red(), targetColor->Green(),
									   targetColor->Blue()) +
								   "\"";

			if (auto pos = std::search(data.begin(), data.end(), from.begin(), from.end()); pos != data.end())
			{
				std::copy(to.begin(), to.end(), pos);
			}
		}

		wxIcon icon;
		icon.CopyFromBitmap(
			wxBitmapBundle::FromSVG(data.data(), data.size(), targetSize).GetBitmap(targetSize));

		return icon;
	}

	wxIcon loadNormalIcon(const wxString& location)
	{
		wxIcon icon(location, wxBITMAP_TYPE_ANY);

		if (!icon.IsOk())
			icon = wxIcon(location, wxBITMAP_TYPE_ICO);

		return icon;
	}

	wxBitmap loadImpl(IconStorage::IconCache& cache, const IconStorage::IconLocation& location,
		const wxSize& targetSize, std::optional<wxColour> targetColor = {})
	{
		const auto it = cache.find({ location, targetSize, targetColor ? *targetColor : *wxWHITE });

		if (it != std::end(cache))
			return it->second;

		wxLogNull noLogging;  // suppress wxWidgets messages about inability to load icon

		wxIcon icon;
		if (auto stock = std::get_if<Icon::Stock>(&location))
		{
			if (*stock == Icon::Stock::empty)
			{
				wxBitmap bitmap(targetSize, 32);
				auto     mask = new wxMask(bitmap, *wxBLACK);
				bitmap.SetMask(mask);

				icon.CopyFromBitmap(bitmap);
			}
			else
			{
				icon = loadSvgIcon(*stock, targetSize, targetColor);
			}
		}
		else
		{
			const auto path = wxString::FromUTF8(std::get<std::string>(location));
			icon            = loadNormalIcon(path);
		}

		if (!icon.IsOk())
			return loadImpl(cache, Icon::Stock::blank, targetSize);

		if (icon.GetSize() != targetSize)
			icon.CopyFromBitmap(wxBitmap(wxBitmap(icon).ConvertToImage().Rescale(
				targetSize.GetWidth(), targetSize.GetHeight(), wxIMAGE_QUALITY_NEAREST)));

		return cache[{ location, targetSize, targetColor ? *targetColor : *wxWHITE }] = icon;
	}
}

mm::IconStorage::IconStorage(InterfaceSize interfaceSize)
	: _defaultSize(toIconPredefinedSize(interfaceSize))
{}

wxBitmap IconStorage::get(
	Icon::Stock icon, std::optional<Icon::Size> targetSize, std::optional<wxColour> targetColor)
{
	return loadImpl(_iconCache, icon, iconSize(targetSize.value_or(_defaultSize)), targetColor);
}

wxBitmap IconStorage::get(const std::string& name, std::optional<Icon::Size> resizeTo)
{
	return loadImpl(_iconCache, name, iconSize(resizeTo.value_or(_defaultSize)));
}
