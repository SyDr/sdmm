// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "priority_data_renderer.h"

wxSize mmPriorityDataRenderer::GetSize() const
{
	return { 40, wxDataViewIconTextRenderer::GetSize().y };
}
