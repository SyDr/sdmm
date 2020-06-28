// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <vector>

#include <wx/dialog.h>

class wxListView;
class wxImageList;

namespace mm
{
	class IIconStorage;

	class SelectExe : public wxDialog
	{
	public:
		SelectExe(wxWindow *parent, const std::filesystem::path& basePath, const wxString& initiallySelectedFile,
			IIconStorage& iconStorage);

		wxString getSelectedFile() const;

	private:
		void refreshListContent();
		std::vector<wxString> getFileList();

	private:
		const std::filesystem::path _basePath;
		IIconStorage& _iconStorage;
		wxString _selectedFile;

		wxListView* _list = nullptr;
		std::unique_ptr<wxImageList> _imageList;
	};
}
