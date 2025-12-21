// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "error_view.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/stacktrace.hpp>

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/hyperlink.h>

#include "application.h"

using namespace mm;

ErrorView::ErrorView(wxWindow *parent)
	: wxDialog(parent, wxID_ANY, "dialog/error/caption"_lng, wxDefaultPosition, wxSize(800, 444))
{
	SetIcon(wxICON(sample));

	createControls();
	buildLayout();
	fillData();
}

void ErrorView::createControls()
{
	_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	_text = new wxTextCtrl(_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	_ok = new wxButton(_panel, wxID_OK, "dialog/close"_lng);
	_label = new wxStaticText(_panel, wxID_ANY, "dialog/error/prompt"_lng);
	_link = new wxHyperlinkCtrl(_panel, wxID_ANY, L"https://github.com/SyDr/sdmm/issues",
		L"https://github.com/SyDr/sdmm/issues");
}

void ErrorView::buildLayout()
{
	auto bHorizontal = new wxBoxSizer(wxHORIZONTAL);
	bHorizontal->Add(_link, wxSizerFlags().Proportion(1));
	bHorizontal->AddStretchSpacer();
	bHorizontal->Add(_ok, wxSizerFlags().Proportion(0));

	auto bVertical = new wxStaticBoxSizer(wxVERTICAL, _panel, "dialog/error/details"_lng);
	bVertical->Add(_label, wxSizerFlags().Proportion(0).Border(wxALL, 5));
	bVertical->Add(_text, wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5));
	bVertical->Add(bHorizontal, wxSizerFlags().Proportion(0).Expand().Border(wxALL, 5));
	_panel->SetSizer(bVertical);

	auto bSizerMain = new wxBoxSizer(wxVERTICAL);
	bSizerMain->Add(_panel, wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5));
	this->SetSizer(bSizerMain);

	this->Layout();
	_label->Wrap(_label->GetClientSize().GetWidth());
	this->Layout();
}

void ErrorView::fillData()
{
	wxString exceptionInfo;
	std::stringstream stacktrace;

	if (auto curExcept = std::current_exception())
	{
		try
		{
			std::rethrow_exception(curExcept);
		}
		catch (const std::exception& e)
		{
			exceptionInfo = wxString::FromUTF8(e.what());
			const boost::stacktrace::stacktrace* st = boost::get_error_info<traced>(e);
			if (st)
				stacktrace << *st << '\n';
		}
		catch (...)
		{
			exceptionInfo = L"Unknown exception";
		}
	}

	_text->SetValue(exceptionInfo + L"\r\n\r\n" + wxString::FromUTF8(stacktrace.str()));
}
