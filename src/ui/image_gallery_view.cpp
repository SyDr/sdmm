// SD Mod Manager

// Copyright (c) 2023-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "image_gallery_view.hpp"

#include "application.h"

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
}

ImageGalleryView::ImageGalleryView(wxWindow* parent, wxWindowID winid, const fs::path& directory,
	const wxPoint& pos, const wxSize& size, const wxString& name)
	: wxScrolled<wxPanel>(parent, winid, pos, size, wxVSCROLL, name)
{
	SetScrollRate(0, 40);
	_gallerySizer = new wxWrapSizer();
	SetSizer(_gallerySizer);

	Bind(wxEVT_SHOW, [=](const wxShowEvent&) {
		SetMinSize({240, 200});
		Reload();
	});

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
		if (!is_directory(it->status()))
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
			assert(item->Destroy());

		_galleryImages.clear();

		for (const auto& item : _images)
		{
			_galleryImages.emplace_back(new wxGenericStaticBitmap(this, wxID_ANY, item.second));
			_gallerySizer->Add(_galleryImages.back(), wxSizerFlags(0).Expand().Border(wxALL, 4));
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
		assert(item->Destroy());

	_galleryImages.clear();
	Layout();
}
