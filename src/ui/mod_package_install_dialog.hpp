// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_package_install.hpp"
#include "utility/wx_widgets_ptr.hpp"

#include <wx/dialog.h>

class wxButton;
class wxDataViewListCtrl;
class wxStaticBox;
class wxTextCtrl;

namespace mm
{
	class ModPackageInstallDialog : public wxDialog
	{
	public:
		ModPackageInstallDialog(wxWindow* parent, const ModPackageInspection& inspection);

		[[nodiscard]] std::vector<std::size_t> selectedMods() const;

	private:
		void populate();
		void updateDescription();
		void updateInstallButton();

	private:
		const ModPackageInspection& _inspection;

		wxWidgetsPtr<wxDataViewListCtrl> _mods             = nullptr;
		wxWidgetsPtr<wxStaticBox>        _descriptionGroup = nullptr;
		wxWidgetsPtr<wxTextCtrl>         _description      = nullptr;
		wxWidgetsPtr<wxStaticBox>        _diagnosticsGroup = nullptr;
		wxWidgetsPtr<wxTextCtrl>         _diagnostics      = nullptr;
		wxWidgetsPtr<wxButton>           _install          = nullptr;
	};
}
