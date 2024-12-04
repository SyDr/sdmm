// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "show_file_list_dialog.hpp"

#include "application.h"
#include "domain/mod_data.hpp"
#include "icon_helper.hpp"
#include "interface/iapp_config.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/imod_data_provider.hpp"
#include "mod_list_model.h"
#include "type/embedded_icon.h"
#include "utility/sdlexcept.h"
#include "wx/data_view_multiple_icons_renderer.h"
#include "wx/priority_data_renderer.h"

#include <boost/algorithm/string.hpp>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dataview.h>
#include <wx/dir.h>
#include <wx/dirctrl.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <algorithm>

using namespace mm;

namespace
{
	const std::unordered_set<std::wstring> pacExtensions = {
		L".lod",
		L".snd",
		L".vid",
		L".pac",
	};

	Era2DirectoryStructure listModFiles(std::stop_token token, const std::vector<std::string>& mods,
		ShowFileListDialog::ShowGameFiles gameFiles, mm::IModDataProvider& dataProvider,
		const fs::path& basePath, std::mutex& mutex, std::string& progress)
	{
		std::map<fs::path, size_t> temp;  // [path] -> index
		Era2DirectoryStructure     result;
		result.mods = mods;

		auto fileIndex = [&](const fs::path& path, bool createNew = true) {
			auto it = temp.find(path);
			if (it == temp.cend())
			{
				if (!createNew)
					return size_t(-1);

				result.entries.emplace_back(Era2FileEntry());

				auto& entry    = result.entries.back();
				entry.filePath = path;
				entry.modPaths.resize(result.mods.size());

				std::tie(it, std::ignore) = temp.emplace(path, result.entries.size() - 1);
			}

			return it->second;
		};

		std::string progressBase;
		std::string progressDetails;

		auto reportProgress = [&mutex, &progress, &progressBase, &progressDetails]() {
			std::lock_guard lg(mutex);
			progress = progressBase;
			if (!progressDetails.empty())
				progress += ":" + progressDetails;
		};

		using rdi = fs::recursive_directory_iterator;
		for (size_t i = 0; i < result.mods.size(); ++i)
		{
			progressBase    = {};
			progressDetails = {};
			reportProgress();

			if (token.stop_requested())
				return {};

			const auto& mod     = result.mods[i];
			const auto& modData = dataProvider.modData(mod);

			if (!exists(modData.data_path) || !is_directory(modData.data_path))
				continue;

			for (auto it = rdi(modData.data_path), end = rdi(); it != end; ++it)
			{
				if (token.stop_requested())
					return {};

				boost::system::error_code ec;

				const auto relative = fs::relative(it->path(), modData.data_path, ec);

				if (ec)
				{
					wxLogError(wxString("Can't make relative path for '%s'\r\n\r\n%s (code: %d)"_lng),
						it->path().wstring(), wxString::FromUTF8(ec.message()), ec.value());
					continue;
				}

				progressBase = it->path().string();
				reportProgress();

				const bool isFile = it->is_regular_file(ec);
				if (ec)
					wxLogError(wxString("Can't access '%s'\r\n\r\n%s (code: %d)"_lng), it->path().wstring(),
						wxString::FromUTF8(ec.message()), ec.value());

				if (!isFile)
					continue;

				const auto index = fileIndex(relative);
				auto&      item  = result.entries[index];

				item.modPaths[i] = fs::relative(it->path(), basePath, ec).string();

				if (!pacExtensions.count(boost::to_lower_copy(it->path().extension().wstring())))
					continue;

				boost::nowide::fstream file(it->path(), std::fstream::in | std::fstream::binary);
				std::array<char, 4>    lodSignature = {};

				file.read(lodSignature.data(), lodSignature.size());

				if (strcmp(lodSignature.data(), "LOD"))
					continue;

				file.seekp(4, std::ios_base::cur);

				uint32_t totalFiles = 0;
				file.read(reinterpret_cast<char*>(&totalFiles), sizeof(totalFiles));

				for (uint32_t j = 0; j < totalFiles; ++j)
				{
					if (token.stop_requested())
						return {};

					file.seekp(92ll + 32ll * j, std::ios_base::beg);
					std::array<char, 16> filename = {};
					file.read(filename.data(), filename.size());

					progressDetails = filename.data();
					reportProgress();

					const auto lodIndex = fileIndex(relative.parent_path() / filename.data());
					auto&      subItem  = result.entries[lodIndex];

					if (subItem.modPaths[i].empty())
						subItem.modPaths[i] = fs::relative(it->path(), basePath, ec).string();
				}
			}
		}

		progressBase    = {};
		progressDetails = {};
		reportProgress();

		if (gameFiles == ShowFileListDialog::ShowGameFiles::none)
			return result;

		for (auto it = rdi(basePath), end = rdi(); it != end; ++it)
		{
			if (token.stop_requested())
				return {};

			if (it->path().filename() == "Mods")
				it.disable_recursion_pending();

			boost::system::error_code ec;
			const auto                relative = fs::relative(it->path(), basePath, ec);

			if (ec)
			{
				wxLogError(wxString("Can't make relative path for '%s'\r\n\r\n%s (code: %d)"_lng),
					it->path().wstring(), wxString::FromUTF8(ec.message()), ec.value());
				continue;
			}

			progressBase = it->path().string();
			reportProgress();

			const bool isFile = it->is_regular_file(ec);
			if (ec)
				wxLogError(wxString("Can't access '%s'\r\n\r\n%s (code: %d)"_lng), it->path().wstring(),
					wxString::FromUTF8(ec.message()), ec.value());

			if (!isFile)
				continue;

			auto index = fileIndex(relative, gameFiles == ShowFileListDialog::ShowGameFiles::all);
			if (index == size_t(-1) &&
				!pacExtensions.count(boost::to_lower_copy(it->path().extension().wstring())))
			{
				continue;
			}

			if (index != size_t(-1))
			{
				auto& item    = result.entries[index];
				item.gamePath = fs::relative(it->path(), basePath, ec).string();
			}

			if (!pacExtensions.count(boost::to_lower_copy(it->path().extension().wstring())))
				continue;

			boost::nowide::fstream file(it->path(), std::fstream::in | std::fstream::binary);
			std::array<char, 4>    lodSignature = {};

			file.read(lodSignature.data(), lodSignature.size());
			if (strcmp(lodSignature.data(), "LOD"))
				continue;

			file.seekp(4, std::ios_base::cur);

			uint32_t totalFiles = 0;
			file.read(reinterpret_cast<char*>(&totalFiles), sizeof(totalFiles));

			for (uint32_t j = 0; j < totalFiles; ++j)
			{
				if (token.stop_requested())
					return {};

				file.seekp(92ll + 32ll * j, std::ios_base::beg);
				std::array<char, 16> filename = {};
				file.read(filename.data(), filename.size());

				progressDetails = filename.data();
				reportProgress();

				const auto lodIndex = fileIndex(relative.parent_path() / filename.data(),
					gameFiles == ShowFileListDialog::ShowGameFiles::all);
				if (lodIndex == size_t(-1))
					continue;

				auto& subItem = result.entries[lodIndex];

				if (subItem.gamePath.empty())
					subItem.gamePath = fs::relative(it->path(), basePath, ec).string();
			}
		}

		progressBase    = {};
		progressDetails = {};
		reportProgress();

		return result;
	}
}

ShowFileListDialog::ShowFileListDialog(wxWindow* parent, IIconStorage& iconStorage,
	IModDataProvider& dataProvider, ModList list, const fs::path& basePath)
	: wxDialog(parent, wxID_ANY, "Mod file list"_lng, wxDefaultPosition, wxSize(1280, 720),
		  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, _iconStorage(iconStorage)
	, _dataProvider(dataProvider)
	, _selectModsModel(new ModListModel(dataProvider, iconStorage))
	, _basePath(basePath)
	, _mods(list)
{
	createControls();
	buildLayout();
	bindEvents();

	_selectModsModel->modList(_mods);
	_progressTimer.SetOwner(this);
}

void ShowFileListDialog::createControls()
{
	_selectOptionsGroup = new wxStaticBox(this, wxID_ANY, "Options"_lng);
	createSelectModsList();

	_showGameFiles    = new wxCheckBox(_selectOptionsGroup, wxID_ANY, "Include game files"_lng);
	_showGameFilesAll = new wxCheckBox(_selectOptionsGroup, wxID_ANY, "and include not overriden"_lng);
	_showGameFilesAll->Disable();

	_continue = new wxButton(_selectOptionsGroup, wxID_ANY, "Continue"_lng);

	_resultGroup = new wxStaticBox(this, wxID_ANY, "Results"_lng);

	createResultList();
	createDetailsList();

	_progressStatic = new wxStaticText(this, wxID_ANY, wxEmptyString);
	_close          = new wxButton(this, wxID_ANY, "Close"_lng);
}

void ShowFileListDialog::createResultList()
{
	_fileList = new wxDataViewListCtrl(_resultGroup, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxDV_HORIZ_RULES | wxDV_VERT_RULES | wxDV_ROW_LINES);
	_fileList->AppendTextColumn("Index"_lng, wxDATAVIEW_CELL_INERT, 60, wxALIGN_LEFT);
	_fileList->AppendIconTextColumn("Game"_lng, wxDATAVIEW_CELL_INERT, 50);

	auto r = new mmDataViewMultipleIconsRenderer();
	auto c = new wxDataViewColumn(
		"Mods"_lng, r, 2, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, wxCOL_RESIZABLE | wxCOL_SORTABLE);

	_fileList->AppendColumn(c);
	_fileList->AppendTextColumn("Path"_lng, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT,
		wxCOL_RESIZABLE | wxCOL_SORTABLE);
}

void ShowFileListDialog::createDetailsList()
{
	_detailsList = new wxDataViewListCtrl(_resultGroup, wxID_ANY, wxDefaultPosition, { 400, 150 },
		wxDV_HORIZ_RULES | wxDV_VERT_RULES | wxDV_ROW_LINES);
	_detailsList->AppendIconTextColumn("Mod"_lng, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);
	_detailsList->AppendTextColumn("Path"_lng, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_DEFAULT);
}

void ShowFileListDialog::createSelectModsList()
{
	_selectModsList = new wxDataViewCtrl(
		_selectOptionsGroup, wxID_ANY, wxDefaultPosition, { 400, 350 }, wxDV_ROW_LINES | wxDV_VERT_RULES);
	_selectModsList->AssociateModel(_selectModsModel.get());
	auto r0 =
		new wxDataViewToggleRenderer(wxDataViewToggleRenderer::GetDefaultType(), wxDATAVIEW_CELL_ACTIVATABLE);
	auto r1 = new mmPriorityDataRenderer();
	auto r2 = new wxDataViewIconTextRenderer();

	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r2->SetAlignment(wxALIGN_CENTER_VERTICAL);

	auto column0 = new wxDataViewColumn(L" ", r0, static_cast<unsigned int>(ModListModelColumn::checkbox),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column1 = new wxDataViewColumn(L" ", r1, static_cast<unsigned int>(ModListModelColumn::priority),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column2 = new wxDataViewColumn("Mod"_lng, r2, static_cast<unsigned int>(ModListModelColumn::name),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);

	_selectModsList->AppendColumn(column0);
	_selectModsList->AppendColumn(column1);
	_selectModsList->AppendColumn(column2);

	column1->SetSortOrder(true);

	_selectModsModel->modList(_mods);

	std::unordered_set<std::string> items;
	for (const auto& item : _mods.data)
		if (item.state == ModList::ModState::enabled)
			items.emplace(item.id);

	_selectModsModel->setChecked(items);
}

void ShowFileListDialog::bindEvents()
{
	_showGameFiles->Bind(
		wxEVT_CHECKBOX, [=](wxCommandEvent&) { _showGameFilesAll->Enable(_showGameFiles->IsChecked()); });

	_continue->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		_continue->Disable();
		loadData();
	});

	_fileList->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [=](wxDataViewEvent&) {
		const auto row = _fileList->GetSelectedRow();

		_detailsList->DeleteAllItems();

		if (row < 0 || row >= std::ssize(_data.entries))
			return;

		const auto& entry = _data.entries[row];

		for (size_t j = 0; j < entry.modPaths.size(); ++j)
		{
			if (entry.modPaths[j].empty())
				continue;

			const auto& mod = _dataProvider.modData(_data.mods[j]);

			wxVector<wxVariant> data;

			data.push_back(wxVariant(wxDataViewIconText(
				wxString::FromUTF8(mod.name), loadModIcon(_iconStorage, mod.data_path, mod.icon))));

			data.push_back(wxVariant(wxString::FromUTF8(entry.modPaths[j])));

			_detailsList->AppendItem(data);
		}
	});

	Bind(wxEVT_TIMER, [=](wxTimerEvent&) { updateProgress(); });

	_close->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndDialog(wxOK); });
}

void ShowFileListDialog::buildLayout()
{
	auto showGameFilesSizer = new wxBoxSizer(wxHORIZONTAL);
	showGameFilesSizer->Add(_showGameFiles, wxSizerFlags(0).Expand().Border(wxALL, 4));
	showGameFilesSizer->Add(_showGameFilesAll, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto leftGroupSizer = new wxStaticBoxSizer(_selectOptionsGroup, wxVERTICAL);
	leftGroupSizer->Add(_selectModsList, wxSizerFlags(1).Expand().Border(wxALL, 4));
	leftGroupSizer->Add(showGameFilesSizer, wxSizerFlags(0));
	leftGroupSizer->Add(_continue, wxSizerFlags(0).Right().Border(wxALL, 4));

	auto rightGroupSizer = new wxStaticBoxSizer(_resultGroup, wxVERTICAL);
	rightGroupSizer->Add(_fileList, wxSizerFlags(1).Expand().Border(wxALL, 4));
	rightGroupSizer->Add(_detailsList, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto contentSizer = new wxBoxSizer(wxHORIZONTAL);
	contentSizer->Add(leftGroupSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));
	contentSizer->Add(rightGroupSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto progressSizer = new wxBoxSizer(wxHORIZONTAL);
	progressSizer->Add(_progressStatic, wxSizerFlags(1).Expand().Border(wxALL, 4));
	progressSizer->Add(_close, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(contentSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));
	mainSizer->Add(progressSizer, wxSizerFlags(0).Expand().Border(wxALL, 4));

	this->SetSizer(mainSizer);
}

void ShowFileListDialog::loadData()
{
	_fileList->DeleteAllItems();
	_detailsList->DeleteAllItems();

	const auto& selected = _selectModsModel->getChecked();

	std::vector<std::string> ordered;
	for (const auto& item : _mods.data)
		if (selected.contains(item.id))
			ordered.emplace_back(item.id);

	for (const auto& item : _mods.rest)
		if (selected.contains(item))
			ordered.emplace_back(item);

	_thread = std::jthread(std::bind_front(&ShowFileListDialog::doLoadData, this), ordered,
		_showGameFiles->IsChecked()
			? _showGameFilesAll->IsChecked() ? ShowGameFiles::all : ShowGameFiles::overriden_only
			: ShowGameFiles::none);

	_progressTimer.Start(1000 / 10);
}

void ShowFileListDialog::doLoadData(
	std::stop_token token, std::vector<std::string> ordered, ShowGameFiles gameFiles)
{
	_data = listModFiles(token, ordered, gameFiles, _dataProvider, _basePath, _progressMutex, _progress);

	if (!token.stop_requested())
	{
		CallAfter([=] {
			_progressTimer.Stop();
			_progressStatic->SetLabelText(wxEmptyString);
			ShowFileListDialog::fillData(gameFiles);
			_continue->Enable();
		});
	}
}

void ShowFileListDialog::fillData(ShowGameFiles gameFiles)
{
	_fileList->DeleteAllItems();

	size_t index = 0;
	for (size_t i = 0; i < _data.entries.size(); ++i)
	{
		const auto& entry = _data.entries[i];

		wxVector<wxVariant> data;
		data.push_back(wxVariant(wxString(std::to_wstring(++index))));
		data.push_back(wxVariant(
			wxDataViewIconText(L"", _iconStorage.get(gameFiles != ShowGameFiles::none
														 ? !entry.gamePath.empty() ? embedded_icon::tick_green
																				   : embedded_icon::cross_gray
														 : embedded_icon::question))));
		wxVariant v;
		v.NullList();

		for (size_t j = 0; j < entry.modPaths.size(); ++j)
		{
			if (!entry.modPaths[j].empty())
			{
				const auto& mod = _dataProvider.modData(_data.mods[j]);

				v.Append(wxVariant(loadModIcon(_iconStorage, mod.data_path, mod.icon)));
			}
			else
				v.Append(wxVariant(_iconStorage.get(embedded_icon::cross_gray)));
		}

		data.push_back(v);
		data.push_back(wxVariant(wxString::FromUTF8(entry.filePath.string())));

		_fileList->AppendItem(data);
	}
}

void ShowFileListDialog::updateProgress()
{
	if (!_thread.joinable())
	{
		_progressTimer.Stop();
		_progressStatic->SetLabelText(wxEmptyString);
	}
	else
	{
		std::lock_guard lg(_progressMutex);
		_progressStatic->SetLabelText(wxString::FromUTF8(_progress));
	}
}
