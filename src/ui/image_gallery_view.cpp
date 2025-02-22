// SD Mod Manager

// Copyright (c) 2023-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "image_gallery_view.hpp"

#include "application.h"
#include "utility/wx_current_dir_helper.hpp"

#include <wx/generic/statbmpg.h>
#include <wx/wrapsizer.h>


using namespace mm;

namespace
{
	const size_t defaultScreenHeight = 180;

	wxSize getBestSize(const wxSize initSize, const wxCoord maxHeight)
	{
		if (initSize.GetHeight() <= maxHeight)
			return initSize;

		return { std::max(1, initSize.GetWidth() * maxHeight / initSize.GetHeight()), maxHeight };
	}

	inline const std::unordered_set<std::string> SupportedImageTypes = { ".bmp", ".png", ".jpeg", ".jpg",
		".gif", ".pcx", ".pnm", ".tiff", ".tga" };
}

ImageGalleryView::ImageGalleryView(wxWindow* parent, wxWindowID winid, const fs::path& directory,
	const wxPoint& pos, const wxSize& size, const wxString& name)
	: wxScrolled<wxPanel>(parent, winid, pos, size, wxVSCROLL, name)
{
	SetScrollRate(0, 40);
	_gallerySizer = new wxWrapSizer();
	SetSizer(_gallerySizer);
	SetMinSize({ 240, 200 });

	Bind(wxEVT_SHOW, [=](const wxShowEvent&) { Reload(); });

	SetPath(directory);
}

ImageGalleryView::~ImageGalleryView()
{
	stopWork();
}

void ImageGalleryView::SetPath(const fs::path& directory)
{
	_path = directory;
	Reload();
}

void ImageGalleryView::Reload()
{
	Reset();

	if (!fs::exists(_path) || !IsShown())
	{
		CallAfter([=] { createImageControls(); });
		return;
	}

	using di = fs::directory_iterator;
	for (di it(_path); it != di(); ++it)
		if (!is_directory(it->status()) && SupportedImageTypes.contains(it->path().extension().string()))
			_images.emplace_back(it->path(), wxNullBitmap);

	start();
}

void ImageGalleryView::start()
{
	_future = std::async(std::launch::async, [=] { loadInBackground(); });
}

void ImageGalleryView::createImageControls()
{
	{
		std::lock_guard lg(_dataAccess);

		_gallerySizer->Clear();
		for (auto& item : _galleryImages)
			item->Destroy();

		_galleryImages.clear();

		auto cursor = wxCursor(wxStockCursor::wxCURSOR_HAND);
		for (const auto& item : _images)
		{
			auto control = new wxGenericStaticBitmap(this, wxID_ANY, item.second);

			_galleryImages.emplace_back(control);
			_gallerySizer->Add(control, wxSizerFlags(0).Expand().Border(wxALL, 4));

			control->SetCursor(cursor);
			control->Bind(wxEVT_LEFT_UP, [&](wxMouseEvent&) {
				CurrentDirHelper cdh(item.first.parent_path().wstring());
				wxLaunchDefaultApplication(wxString::FromUTF8(item.first.string()));
			});
		}
	}

	Layout();
	FitInside();
}

void ImageGalleryView::stopWork()
{
	_canceled = true;
	if (_future.valid())
		_future.wait();
}

void ImageGalleryView::loadInBackground()
{
	_canceled = false;

	if (!IsShown())
		return;

	for (size_t i = 0; !_canceled && i < _images.size(); ++i)
	{
		std::lock_guard<std::mutex> lock(_dataAccess);

		wxImage item;
		item.LoadFile(wxString::FromUTF8(_images[i].first.string()));

		if (!item.IsOk())
			continue;

		const wxSize bestSize(getBestSize(item.GetSize(), defaultScreenHeight));
		item.Rescale(bestSize.GetWidth(), bestSize.GetHeight(), wxIMAGE_QUALITY_NORMAL);

		_images[i].second = wxBitmap(item);
	}

	CallAfter([=] { createImageControls(); });
}

void ImageGalleryView::Reset()
{
	stopWork();

	std::lock_guard<std::mutex> lock(_dataAccess);
	_images.clear();

	if (_gallerySizer)
		_gallerySizer->Clear();
	for (auto& item : _galleryImages)
		item->Destroy();

	_galleryImages.clear();
	Layout();
}
