// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/dataview.h>

class mmPriorityDataRenderer : public wxDataViewIconTextRenderer
{
	using wxDataViewIconTextRenderer::wxDataViewIconTextRenderer;

public:
	explicit mmPriorityDataRenderer(int _basicXSize);

	wxSize GetSize() const override;

private:
	int _basicXSize = 0;
};
