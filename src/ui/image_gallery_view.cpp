// SD Mod Manager

// Copyright (c) 2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "image_gallery_view.hpp"

#include "application.h"

using namespace mm;

ImageGalleryView::ImageGalleryView(wxWindow* parent)
	: wxScrolled<wxPanel>(parent)
{
}

void ImageGalleryView::Clear()
{
	for (auto& item : _items)
		item->Destroy();

	_items.clear();
}

void ImageGalleryView::LoadFrom(wxString modId)
{
	Clear();

	int horSize = 0;

	_mainSizer = new wxBoxSizer(wxHORIZONTAL);

	using di = std::filesystem::directory_iterator;
	std::error_code ec;
	auto            it = di(std::filesystem::path(modId.ToStdString()), ec);

	if (!ec)
	{
		for (auto end = di(); it != end; ++it)
		{
			if (it->is_directory())
				continue;

			wxImage image;
			if (!image.LoadFile(it->path().string()))
				continue;

			int targetWidth  = 240;
			int targetHeight = 180;

			auto widthRatio  = static_cast<float>(image.GetWidth()) / targetWidth;
			auto heightRatio = static_cast<float>(image.GetHeight()) / targetHeight;
			
			if (widthRatio > heightRatio)
				targetHeight = image.GetHeight() / widthRatio;
			else if (heightRatio > widthRatio)
				targetWidth = image.GetWidth() / heightRatio;

			image.Rescale(targetWidth, targetHeight, wxIMAGE_QUALITY_BICUBIC);

			horSize += image.GetSize().GetWidth();

			wxBitmapBundle bundle(image);
			auto           control = _items.emplace_back(new wxGenericStaticBitmap(this, wxID_ANY, bundle));

			_mainSizer->Add(control, wxSizerFlags().Border(wxALL, 4));
		}
	}

	SetSizer(_mainSizer);
	FitInside();
	SetScrollRate(120, -1);
}
