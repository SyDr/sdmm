// SD Mod Manager

// Copyright (c) 2023-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"
#include "utility/wx_widgets_ptr.hpp"

#include <future>
#include <mutex>

#include <wx/scrolwin.h>

class wxWrapSizer;
class wxGenericStaticBitmap;

namespace mm
{
	struct ImageGalleryView : public wxScrolled<wxPanel>
	{
	public:
		ImageGalleryView(wxWindow* parent, wxWindowID winid, const fs::path& directory = {},
			const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
			const wxString& name = L"mmGalleryWidget");
		~ImageGalleryView();

		void SetPath(const fs::path& directory);
		void Reset();
		void Reload();

	private:
		void createImageControls();
		void stopWork();
		void start();
		void loadInBackground();

	private:
		fs::path _path;

		mutable std::mutex _dataAccess;
		std::future<void>  _future;
		std::atomic_bool   _canceled   = false;

		std::vector<std::pair<fs::path, wxBitmap>>       _images;
		wxWidgetsPtr<wxWrapSizer>                        _gallerySizer = nullptr;
		std::vector<wxWidgetsPtr<wxGenericStaticBitmap>> _galleryImages;
	};
}
