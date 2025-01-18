// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "priority_data_renderer.h"

wxSize mmPriorityDataRenderer::GetSize() const
{
	return { _basicXSize, wxDataViewIconTextRenderer::GetSize().y };
}

mmPriorityDataRenderer::mmPriorityDataRenderer(int basicXSize)
	: _basicXSize(basicXSize)
{}
