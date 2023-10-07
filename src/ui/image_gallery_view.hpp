// SD Mod Manager

// Copyright (c) 2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/panel.h>
#include <wx/generic/statbmpg.h>

#include "utility/wx_widgets_ptr.hpp"

namespace mm
{
	struct IIconStorage;

	struct ImageGalleryView : public wxScrolled<wxPanel>
	{
	public:
		ImageGalleryView(wxWindow* parent);

		void Clear();
		void LoadFrom(wxString modId);

	private:
		wxWidgetsPtr<wxBoxSizer>                         _mainSizer = nullptr;
		std::vector<wxWidgetsPtr<wxGenericStaticBitmap>> _items;
	};
}
