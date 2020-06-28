// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/dataview.h>

class mmPriorityDataRenderer : public wxDataViewIconTextRenderer
{
	using wxDataViewIconTextRenderer::wxDataViewIconTextRenderer;

public:
	wxSize GetSize() const override;
};
