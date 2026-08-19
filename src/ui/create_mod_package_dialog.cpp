// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "create_mod_package_dialog.hpp"

#include "application.h"
#include "domain/mod_data.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/imod_data_provider.hpp"
#include "mod_list_model.h"
#include "system_info.hpp"
#include "type/icon.hpp"
#include "utility/sdlexcept.h"
#include "wx/priority_data_renderer.h"

#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/process/v1/windows.hpp>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dataview.h>
#include <wx/dirctrl.h>
#include <wx/filepicker.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include "icon_helper.hpp"
#include <algorithm>
#include <chrono>
#include <thread>

#include "utility/fs_util.h"
#include "utility/sfx_util.hpp"
#include "utility/shell_util.h"

using namespace mm;

namespace
{
	bool prepareSevenZipFiles(
		const fs::path& sourceDir, const fs::path& tempDir, const std::optional<fs::path>& icon)
	{
		boost::system::error_code ec;

		if (!exists(tempDir, ec))
		{
			create_directories(tempDir, ec);
			if (ec)
			{
				// notify about error
				return false;
			}
		}

		for (const auto& item : { "7z.exe", "7z.dll", "7z.sfx" })
		{
			copy_file(sourceDir / item, tempDir / item, fs::copy_options::overwrite_existing, ec);
			if (ec)
			{
				// notify about error
				return false;
			}
		}

		if (icon)
			sfx::updateData(tempDir / "7z.sfx", *icon);

		return true;
	}

	fs::path generateArchiveName(const std::string& preferred)
	{
		static const std::set toReplace = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };

		std::string r;
		r.reserve(preferred.size());

		for (const auto& c : preferred)
			r += !toReplace.contains(c) ? c : '_';

		return fs::path(r).replace_extension(".exe");
	}

	bool validateOutputDirectory(const std::vector<CreateModPackageDialog::PacketInfo>& items)
	{
		std::set<fs::path> problematic;

		boost::system::error_code ec;

		for (const auto& [_, target, icon] : items)
			if (exists(target, ec))
				problematic.emplace(target);

		if (!problematic.empty())
		{
			const int answer = wxMessageBox("message/question/some_of_targets_exists"_lng,
				wxTheApp->GetAppName(), wxYES_NO | wxNO_DEFAULT);

			if (answer != wxYES)
				return false;
		}

		return true;
	}
}

CreateModPackageDialog::CreateModPackageDialog(wxWindow* parent, IIconStorage& iconStorage,
	IModDataProvider& dataProvider, ModList list, const fs::path& basePath, const fs::path& tempPath,
	const std::string& selectedMod)
	: wxDialog(parent, wxID_ANY, "dialog/create_mod_package/caption"_lng, wxDefaultPosition,
		  wxSize(1280, 720), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, _iconStorage(iconStorage)
	, _dataProvider(dataProvider)
	, _selectModsModel(new ModListModel(dataProvider, iconStorage, ModListModelManagedMode::as_flat_list,
		  ModListModelArchivedMode::as_single_group, Icon::Size::x16))
	, _basePath(basePath)
	, _tempPath(tempPath)
	, _mods(list)
	, _initiallySelected(selectedMod)
{
	createControls();
	buildLayout();
	bindEvents();

	_selectModsModel->modList(_mods);

	_progressTimer.SetOwner(this);

	CallAfter(&CreateModPackageDialog::selectOutputDirectoryRequested);
}

void CreateModPackageDialog::createControls()
{
	_selectOptionsGroup = new wxStaticBox(this, wxID_ANY, "dialog/label/options"_lng);
	createSelectModsList();

	_targetStatic =
		new wxStaticText(_selectOptionsGroup, wxID_ANY, "dialog/create_mod_package/target_directory"_lng);

	_targetDirectory = new wxTextCtrl(
		_selectOptionsGroup, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);

	_pickTargetDirectory = new wxButton(_selectOptionsGroup, wxID_ANY, L"...");

	_packIntoSingleArchive = new wxCheckBox(
		_selectOptionsGroup, wxID_ANY, "dialog/create_mod_package/pack_into_single_archive"_lng);

	_packInBackground =
		new wxCheckBox(_selectOptionsGroup, wxID_ANY, "dialog/create_mod_package/pack_in_background"_lng);

	// _sevenZipSwitchesStatic =	new wxStaticText(_selectOptionsGroup, wxID_ANY,
	// "dialog/create_mod_package/7z_switches"_lng);

	// _sevenZipSwitchesEdit = new wxTextCtrl(_selectOptionsGroup, wxID_ANY, L"-bb1 -bd -bsp1");

	_continue = new wxButton(_selectOptionsGroup, wxID_ANY, "dialog/button/continue"_lng);

	_logGroup = new wxStaticBox(this, wxID_ANY, "column/results"_lng);

	_logCtrl = new wxTextCtrl(
		this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

	_close = new wxButton(this, wxID_CLOSE, "dialog/button/close"_lng);
}

void CreateModPackageDialog::createSelectModsList()
{
	_selectModsList = new wxDataViewCtrl(
		_selectOptionsGroup, wxID_ANY, wxDefaultPosition, { 400, 350 }, wxDV_ROW_LINES | wxDV_VERT_RULES);
	_selectModsList->AssociateModel(_selectModsModel.get());
	auto r0 =
		new wxDataViewToggleRenderer(wxDataViewToggleRenderer::GetDefaultType(), wxDATAVIEW_CELL_ACTIVATABLE);
	auto r1 = new mmPriorityDataRenderer(FromDIP(32));
	auto r2 = new wxDataViewIconTextRenderer();

	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r2->SetAlignment(wxALIGN_CENTER_VERTICAL);

	auto column0 = new wxDataViewColumn(L" ", r0, static_cast<unsigned int>(ModListModelColumn::checkbox),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column1 = new wxDataViewColumn(L" ", r1, static_cast<unsigned int>(ModListModelColumn::priority),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column2 = new wxDataViewColumn("dialog/label/mod"_lng, r2,
		static_cast<unsigned int>(ModListModelColumn::name), wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);

	_selectModsList->AppendColumn(column0);
	_selectModsList->AppendColumn(column1);
	_selectModsList->AppendColumn(column2);

	column1->SetSortOrder(true);

	_selectModsModel->modList(_mods);

	std::unordered_set<std::string> items = { _initiallySelected };

	if (items.empty())
	{
		for (const auto& item : _mods.data)
			if (item.state == ModList::ModState::enabled)
				items.emplace(item.id);
	}

	_selectModsModel->setChecked(items);
}

void CreateModPackageDialog::bindEvents()
{
	_pickTargetDirectory->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { selectOutputDirectoryRequested(); });

	_continue->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { createPackageRequested(); });

	Bind(wxEVT_TIMER, [=](wxTimerEvent&) { updateLog(); });

	_close->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndDialog(wxOK); });
}

void CreateModPackageDialog::buildLayout()
{
	auto targetSizer = new wxBoxSizer(wxHORIZONTAL);
	targetSizer->Add(_targetStatic, wxSizerFlags(0).Align(wxALIGN_CENTER_VERTICAL).Border(wxALL, 4));
	targetSizer->Add(_targetDirectory, wxSizerFlags(1).Expand().Border(wxALL, 4));
	targetSizer->Add(_pickTargetDirectory, wxSizerFlags(0).Align(wxALIGN_CENTER_VERTICAL).Border(wxALL, 4));

	// auto sevenZipSwitchesSizer = new wxBoxSizer(wxHORIZONTAL);
	// sevenZipSwitchesSizer->Add(_sevenZipSwitchesStatic, wxSizerFlags(0).Align(wxALIGN_CENTER_VERTICAL));
	// sevenZipSwitchesSizer->Add(_sevenZipSwitchesEdit, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto leftGroupSizer = new wxStaticBoxSizer(_selectOptionsGroup, wxVERTICAL);
	leftGroupSizer->Add(_selectModsList, wxSizerFlags(1).Expand().Border(wxALL, 4));
	leftGroupSizer->Add(targetSizer, wxSizerFlags(0).Expand().Border(wxALL, 4));
	leftGroupSizer->Add(_packIntoSingleArchive, wxSizerFlags(0).Expand().Border(wxALL, 4));
	leftGroupSizer->Add(_packInBackground, wxSizerFlags(0).Expand().Border(wxALL, 4));
	// leftGroupSizer->Add(sevenZipSwitchesSizer, wxSizerFlags(0).Expand().Border(wxALL, 4));
	leftGroupSizer->Add(_continue, wxSizerFlags(0).Right().Border(wxALL, 4));

	auto rightGroupSizer = new wxStaticBoxSizer(_logGroup, wxVERTICAL);
	rightGroupSizer->Add(_logCtrl, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto contentSizer = new wxBoxSizer(wxHORIZONTAL);
	contentSizer->Add(leftGroupSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));
	contentSizer->Add(rightGroupSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto progressSizer = new wxBoxSizer(wxHORIZONTAL);
	progressSizer->AddStretchSpacer();
	progressSizer->Add(_close, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(contentSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));
	mainSizer->Add(progressSizer, wxSizerFlags(0).Expand().Border(wxALL, 4));

	this->SetSizer(mainSizer);
}

void CreateModPackageDialog::createPackageRequested()
{
	const auto& selected = _selectModsModel->getChecked();
	if (selected.empty())
		return;

	auto targetPath = fs::path(_targetDirectory->GetValue());
	if (targetPath.empty())
	{
		selectOutputDirectoryRequested();

		targetPath = fs::path(_targetDirectory->GetValue());
		if (targetPath.empty())
			return;
	}

	const bool packIntoSingleArchive = _packIntoSingleArchive->IsChecked();

	std::vector<PacketInfo> ordered;
	for (const auto& id : selected)
	{
		auto mod = _dataProvider.modData(id);

		std::optional<fs::path> icon;
		if (!mod.icon.empty())
			icon = (mod.data_path / mod.icon).lexically_normal();

		boost::system::error_code ec;

		ordered.emplace_back(fs::relative(mod.data_path, _basePath, ec),
			targetPath / (!packIntoSingleArchive ? generateArchiveName(mod.name) : fs::path("Mods.exe")),
			icon);
	}

	if (!validateOutputDirectory(ordered))
		return;

	_continue->Disable();

	_thread = std::jthread(std::bind_front(&CreateModPackageDialog::createPackageImpl, this), ordered,
		_basePath, packIntoSingleArchive, _packInBackground->IsChecked());

	_progressTimer.Start(1000 / 20);
}

void CreateModPackageDialog::selectOutputDirectoryRequested()
{
	EX_TRY;

	wxString    current = _targetDirectory->GetValue();
	wxDirDialog dd(this, wxASCII_STR(wxDirSelectorPromptStr), current, wxDD_DEFAULT_STYLE);

	if (dd.ShowModal() != wxID_OK)
		return;

	_targetDirectory->SetValue(dd.GetPath());

	EX_UNEXPECTED;
}

void CreateModPackageDialog::updateLog()
{
	std::vector<std::string> items;

	{
		std::lock_guard lg(_mutex);
		std::swap(items, _pendingLogEntries);
	}

	if (items.empty())
		return;

	const auto joined = boost::join(items, "");
	_logCtrl->AppendText(wxString::FromUTF8(joined));
}

void CreateModPackageDialog::createPackageImpl(std::stop_token token, std::vector<PacketInfo> ordered,
	fs::path basePath, bool packIntoSingleArchive, bool packInBackground)
{
	const auto sevenZipPath = fs::path(mm::SystemInfo::SevenZipDir);
	const auto tempPath     = _tempPath / fs::unique_path(L"sfx-%%%%-%%%%-%%%%");

	auto resetButtons = [&] {
		if (!token.stop_requested())
			CallAfter([=] { _continue->Enable(); });
	};

	if (packIntoSingleArchive)
	{
		// do something
	}

	for (const auto& [source, target, icon] : ordered)
	{
		if (token.stop_requested())
			break;

		if (!prepareSevenZipFiles(
				sevenZipPath, tempPath, !packIntoSingleArchive ? icon : std::optional<fs::path> {}))
		{
			resetButtons();
			return;
		}

		std::vector<std::wstring> args;
		args.emplace_back((tempPath / "7z.exe").wstring());
		args.emplace_back(L"a");
		args.emplace_back(target.wstring());

		if (!packIntoSingleArchive)
		{
			args.emplace_back(source.wstring());
		}
		else
		{
			for (const auto& item : ordered)
			{
				args.emplace_back(item.source.wstring());
			}
		}

		args.emplace_back(L"-bb1");
		args.emplace_back(L"-bd");
		args.emplace_back(L"-bsp1");
		args.emplace_back(L"-sfx7z.sfx");

		namespace bp = boost::process;

		if (packInBackground)
		{
			bp::child c(args, bp::start_dir = basePath);
			c.detach();

			if (!packIntoSingleArchive)
				continue;
			else
				break;
		}

		bp::ipstream out;
		bp::ipstream err;

		bp::child c(args, bp::std_out > out, bp::std_err > err, bp::start_dir = basePath,
			bp::windows::create_no_window);

		auto readStream = [this, &token](std::istream& is, const std::string& prefix) {
			std::string line;
			while (std::getline(is, line))
			{
				if (!token.stop_requested())
				{
					std::lock_guard lg(_mutex);
					_pendingLogEntries.emplace_back(prefix + line);
				}
			}
		};

		std::thread t_out([&, out = std::move(out)]() mutable { readStream(out, ""); });
		std::thread t_err([&, err = std::move(err)]() mutable { readStream(err, "[ERR] "); });

		{
			using namespace std::chrono_literals;

			while (c.running())
			{
				if (token.stop_requested())
				{
					c.terminate();
					break;
				}

				std::this_thread::sleep_for(20ms);
			}
		}

		if (!token.stop_requested())
		{
			if (c.exit_code())
			{
				//
			}
		}

		t_out.join();
		t_err.join();

		if (packIntoSingleArchive)
			break;
	}

	if (!token.stop_requested())
		resetButtons();

	if (!packInBackground)
	{
		boost::system::error_code ec;
		fs::remove_all(tempPath, ec);

		mm::refreshIcons();
	}
}
