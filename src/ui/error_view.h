// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <vector>

#include <wx/dialog.h>

#include "utility/wx_widgets_ptr.hpp"

class wxHyperlinkCtrl;
class wxStaticBox;
class wxStaticText;
class wxTextCtrl;

namespace mm
{
	class ErrorView : public wxDialog
	{
	public:
		explicit ErrorView(wxWindow *parent);

	private:
		void createControls();
		void buildLayout();
		void fillData();

	private:
		wxWidgetsPtr<wxPanel> _panel = nullptr;
		wxWidgetsPtr<wxStaticBox> _group = nullptr;
		wxWidgetsPtr<wxTextCtrl> _text = nullptr;
		wxWidgetsPtr<wxButton> _ok = nullptr;
		wxWidgetsPtr<wxStaticText> _label = nullptr;
		wxWidgetsPtr<wxHyperlinkCtrl> _link = nullptr;
	};
}
