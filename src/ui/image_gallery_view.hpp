// SD Mod Manager

// Copyright (c) 2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <future>
#include <mutex>

#include <wx/sizer.h>
#include <wx/vscroll.h>

namespace mm
{
	struct ImageGalleryView : public wxScrolledCanvas
	{
	public:
		ImageGalleryView(wxWindow* parent, wxWindowID winid, const fs::path& directory = {},
			const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
			const wxString& name = L"mmGalleryWidget");
		~ImageGalleryView();

		void SetPath(const fs::path& directory);
		void Reset();
		void Expand(bool value);
		void Reload();

	protected:
		wxSize DoGetBestSize() const override;
		void   OnPaint(wxPaintEvent&);

	private:
		void stopWork();
		void start();
		void loadInBackground();

	private:
		fs::path _path;
		bool     _expanded = false;

		mutable std::mutex _dataAccess;
		std::future<void>  _future;
		std::atomic_bool   _canceled   = false;
		std::atomic_size_t _bestHeight = 0;

		std::vector<std::pair<fs::path, wxBitmap>> _images;
	};
}
