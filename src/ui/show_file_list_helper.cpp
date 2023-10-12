// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "domain/mod_data.hpp"
#include "era2/era2_directory_structure.hpp"
#include "interface/imod_data_provider.hpp"
#include "select_mod_pairs_dialog.hpp"
#include "select_mods_dialog.hpp"
#include "show_file_list_dialog.hpp"
#include "show_file_list_helper.hpp"
#include "system_info.hpp"

#include <boost/range/algorithm_ext/erase.hpp>


#include <unordered_set>

using namespace mm;

namespace
{
	const std::unordered_set<std::wstring> pacExtensions = {
		L".lod",
		L".snd",
		L".vid",
		L".pac",
	};

	Era2DirectoryStructure removeDeadMods(Era2DirectoryStructure data)
	{
		for (size_t i = 0; i < data.modList.size();)
		{
			bool const hasFiles = std::any_of(data.entries.cbegin(), data.entries.cend(),
				[&](const std::vector<Era2DirectoryEntry>& item) { return item[i].any(); });

			if (hasFiles)
			{
				++i;
			}
			else
			{
				for (auto& item : data.entries)
					item.erase(item.begin() + i);
				data.modList.erase(data.modList.begin() + i);
			}
		}

		return data;
	}

	Era2DirectoryStructure listModFiles(
		std::unordered_set<wxString> const& mods, mm::IModDataProvider& dataProvider)
	{
		std::map<fs::path, size_t> temp;  // [path] -> index
		Era2DirectoryStructure                  result;

		auto fileIndex = [&](const fs::path& path) {
			auto it = temp.find(path);
			if (it == temp.cend())
			{
				result.fileList.emplace_back(path);
				result.entries.emplace_back(std::vector<Era2DirectoryEntry>(result.modList.size()));
				std::tie(it, std::ignore) = temp.emplace(path, result.fileList.size() - 1);
			}

			return it->second;
		};

		result.modList = { mods.cbegin(), mods.cend() };

		for (size_t i = 0; i < result.modList.size(); ++i)
		{
			const auto& item    = result.modList[i];
			auto        modData = dataProvider.modData(item);

			if (!exists(modData->data_path) || !is_directory(modData->data_path))
				continue;

			using rdi = fs::recursive_directory_iterator;
			for (auto it = rdi(modData->data_path), end = rdi(); it != end; ++it)
			{
				boost::system::error_code ec;

				const auto relative = fs::relative(it->path(), modData->data_path, ec);

				if (ec)
				{
					wxLogError(wxString("Can't make relative path for '%s'\r\n\r\n%s (code: %d)"_lng),
						it->path().wstring(),
						ec.message(), ec.value());
					continue;
				}

				const bool isFile = it->is_regular_file(ec);
				if (ec)
					wxLogError(wxString("Can't access '%s'\r\n\r\n%s (code: %d)"_lng), it->path().wstring(),
						ec.message(),
						ec.value());

				if (!isFile)
					continue;

				static const std::unordered_set<std::wstring> skip = {
					L"change.log",
					L"changelog.txt",
					L"description.txt",
					L"description_chn.txt",
					L"description_en.txt",
					L"description_ru.txt",
					L"description_rus.txt",
					L"icon.ico",
					L"mod.json",
					L"mod_info.ini",
					L"readme.txt",
					L"readme_cn.txt",
					L"readme_ru.txt",
				};

				if (skip.count(boost::to_lower_copy(it->path().filename().wstring())))
					continue;

				const auto index             = fileIndex(relative);
				result.entries[index][i].raw = true;

				if (!pacExtensions.count(boost::to_lower_copy(it->path().extension().wstring())))
					continue;

				boost::nowide::fstream        file(it->path(), std::fstream::in | std::fstream::binary);
				std::array<char, 4> lodSignature;

				file.read(lodSignature.data(), lodSignature.size());

				if (strcmp(lodSignature.data(), "LOD"))
					continue;

				file.seekp(4, std::ios_base::cur);

				uint32_t totalFiles = 0;
				file.read(reinterpret_cast<char*>(&totalFiles), sizeof(totalFiles));

				for (uint32_t j = 0; j < totalFiles; ++j)
				{
					file.seekp(92ll + 32ll * j, std::ios_base::beg);
					std::array<char, 16> filename;
					file.read(filename.data(), filename.size());

					const auto lodIndex             = fileIndex(relative.parent_path() / filename.data());
					result.entries[lodIndex][i].lod = true;
				}
			}
		}

		size_t current = 0;
		for (size_t i = 0; i < result.fileList.size(); ++i)
		{
			const auto number = std::count_if(result.entries[i].cbegin(), result.entries[i].cend(),
				[](const Era2DirectoryEntry& item) { return item.any(); });

			if (number > 1)
			{
				result.fileList[current] = result.fileList[i];
				result.entries[current]  = result.entries[i];
				++current;
			}
		}

		result.fileList.resize(current);
		result.entries.resize(current);

		return removeDeadMods(std::move(result));
	}

	std::vector<std::pair<wxString, wxString>> resolveCombinations(Era2DirectoryStructure const& data)
	{
		std::set<std::pair<wxString, wxString>> result;
		for (size_t i = 0; i < data.modList.size(); ++i)
			for (size_t j = i + 1; j < data.modList.size(); ++j)
				for (const auto& row : data.entries)
					if (row[i].any() && row[j].any())
					{
						result.emplace(data.modList[i], data.modList[j]);

						break;
					}

		return { result.begin(), result.end() };
	}

	Era2DirectoryStructure removeCombinationsIfPossible(
		Era2DirectoryStructure data, std::set<std::pair<wxString, wxString>> remove)
	{
		if (remove.empty())
			return data;

		size_t current = 0;
		for (size_t i = 0; i < data.fileList.size(); ++i)
		{
			bool const removeLine = [&] {
				for (size_t k = 0; k < data.entries[i].size(); ++k)
				{
					for (size_t j = k + 1; j < data.entries[i].size(); ++j)
					{
						if (data.entries[i][k].any() && data.entries[i][j].any() &&
							!remove.count({ data.modList[k], data.modList[j] }))
						{
							return false;
						}
					}
				}

				return true;
			}();

			if (!removeLine)
			{
				data.fileList[current] = data.fileList[i];
				data.entries[current]  = data.entries[i];
				++current;
			}
		}

		data.fileList.resize(current);
		data.entries.resize(current);

		return removeDeadMods(std::move(data));
	}
}

void mm::showModFileList(
	wxWindow& parent, Application& application, IModDataProvider& dataProvider, ModList list)
{
	SelectModsDialog smd(parent, application, dataProvider, list);
	if (smd.ShowModal() != wxID_OK)
		return;

	auto files = listModFiles(smd.getSelected(), dataProvider);

	SelectModPairsDialog smpd(parent, application, dataProvider, resolveCombinations(files));
	if (smpd.ShowModal() != wxID_OK)
		return;

	files = removeCombinationsIfPossible(std::move(files), smpd.getSelected());

	ShowFileListDialog dialog(&parent, application, dataProvider, std::move(files));
	dialog.ShowModal();
}
