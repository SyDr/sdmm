// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/dataview.h>

class mmDataViewMultipleIconsRenderer : public wxDataViewCustomRenderer
{
public:
	mmDataViewMultipleIconsRenderer();

	bool GetValue(wxVariant& value) const override;
	bool Render(wxRect cell, wxDC *dc, int state) override;
	bool SetValue(const wxVariant& value) override;
	wxSize GetSize() const override;

private:
	wxVariant _data;
};
