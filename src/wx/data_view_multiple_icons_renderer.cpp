// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "data_view_multiple_icons_renderer.h"

#include <wx/dc.h>

mmDataViewMultipleIconsRenderer::mmDataViewMultipleIconsRenderer()
	: wxDataViewCustomRenderer("list")
{
	_data.NullList();
}

bool mmDataViewMultipleIconsRenderer::Render(wxRect cell, wxDC* dc, int)
{
	int xOffset = 0;

	for (size_t i = 0; i < _data.GetCount(); ++i)
	{
		wxIcon icon;
		icon << _data[i];

		if (icon.IsOk())
		{
			dc->DrawIcon(icon, cell.x + xOffset, cell.y + (cell.height - icon.GetHeight()) / 2);
			xOffset += icon.GetWidth() + 4;
		}
	}

	return true;
}

wxSize mmDataViewMultipleIconsRenderer::GetSize() const
{
	int xSize = 0;

	for (size_t i = 0; i < _data.GetCount(); ++i)
	{
		wxIcon icon;
		icon << _data[i];

		if (icon.IsOk())
			xSize += icon.GetWidth() + 4;
	}

	if (!_data.GetCount())
		xSize = 20;

	return { xSize, 20 };
}

bool mmDataViewMultipleIconsRenderer::SetValue(const wxVariant& value)
{
	if (value.GetType() != "list")
		return false;

	_data = value;

	return true;
}

bool mmDataViewMultipleIconsRenderer::GetValue(wxVariant& value) const
{
	value = _data;

	return true;
}
