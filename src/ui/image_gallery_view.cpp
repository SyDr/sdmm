// SD Mod Manager

// Copyright (c) 2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "image_gallery_view.hpp"

#include "application.h"


#include <wx/dcbuffer.h>

using namespace mm;

namespace
{
	wxSize getBestSize(const wxSize initSize, const wxCoord maxHeight)
	{
		if (initSize.GetHeight() <= maxHeight)
			return initSize;

		return { std::max(1, initSize.GetWidth() * maxHeight / initSize.GetHeight()), maxHeight };
	}
}

ImageGalleryView::ImageGalleryView(wxWindow* parent, wxWindowID winid, const fs::path& directory,
	const wxPoint& pos, const wxSize& size,
	const wxString& name)
	: wxScrolledCanvas(parent, winid, pos, size, wxSTATIC_BORDER | wxALWAYS_SHOW_SB, name)
{
	SetScrollRate(180, -1);
	Bind(wxEVT_PAINT, &ImageGalleryView::OnPaint, this);
	Bind(wxEVT_SHOW, [=](const wxShowEvent& event) {
		if (!event.IsShown())
			_bestHeight = 0;
	});
	SetPath(directory);
}

ImageGalleryView::~ImageGalleryView()
{
	resetThread();
}

void ImageGalleryView::SetPath(const fs::path& directory)
{
	_path = directory;
	Reload();
}

void ImageGalleryView::Expand(bool value)
{
	if (_expanded != value)
	{
		_expanded = value;
		CacheBestSize(DoGetBestSize());
		Refresh();
	}
}

wxSize ImageGalleryView::DoGetBestSize() const
{
	return { _expanded ? 80000 : 240, _expanded ? 60000 : 180 };
}

void ImageGalleryView::OnPaint(wxPaintEvent&)
{
	wxPaintDC dc(this);
	DoPrepareDC(dc);
	dc.Clear();

	const wxSize  dcSize   = dc.GetSize();
	const wxPoint dcOrigin = dc.GetDeviceOrigin();

	const size_t bestHeight = dcSize.GetHeight() - 4;

	if (_bestHeight != bestHeight)
	{
		_bestHeight = bestHeight;
		Reload();
	}

	wxCoord curX = 2;

	std::lock_guard<std::mutex> lock(_dataAccess);

	for (size_t i = 0; i < _images.size(); ++i)
	{
		const wxBitmap& image = _images[i].second;

		if (!image.IsOk())
			continue;

		if (curX + dcOrigin.x <= dcSize.GetWidth() && curX + dcOrigin.x + image.GetWidth() >= 0)
			dc.DrawBitmap(image, curX, (dcSize.GetHeight() - image.GetHeight()) / 2, false);

		curX += image.GetWidth() + 2;
	}

	SetVirtualSize(curX, dcSize.GetHeight());
}

void ImageGalleryView::Reload()
{
	Reset();

	if (fs::exists(_path))
	{
		using di = fs::directory_iterator;
		for (di it(_path); it != di(); ++it)
			if (!is_directory(it->status()))
				_images.emplace_back(it->path(), wxNullBitmap);
	}

	start();
}

void ImageGalleryView::start()
{
	_future = std::async(std::launch::async, [=] { loadInBackground(); });
}

void ImageGalleryView::resetThread()
{
	_canceled = true;
	if (_future.valid())
		_future.wait();
}

void ImageGalleryView::loadInBackground()
{
	_canceled = false;

	if (!_bestHeight)
		return;

	for (size_t i = 0; !_canceled && i < _images.size(); ++i)
	{
		std::lock_guard<std::mutex> lock(_dataAccess);

		wxImage item;
		item.LoadFile(wxString::FromUTF8(_images[i].first.string()));

		if (!item.IsOk())
			continue;

		const wxSize bestSize(getBestSize(item.GetSize(), _bestHeight));
		item.Rescale(bestSize.GetWidth(), bestSize.GetHeight(), wxIMAGE_QUALITY_NORMAL);

		_images[i].second = wxBitmap(item);

		CallAfter([=] { Refresh(false); });
	}

	CallAfter([=] { Refresh(false); });
}

void ImageGalleryView::Reset()
{
	resetThread();
	_images.clear();

	SetVirtualSize(0, 0);
}
