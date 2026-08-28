// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_plugin_manager.hpp"

#include "domain/mod_data.hpp"

#include <windows.h>

#include <boost/locale/conversion.hpp>

#include <array>
#include <map>
#include <system_error>

using namespace mm;

namespace
{
	struct LocationDefinition
	{
		PluginLocation location;
		std::string    directory;
		bool           restrictedExtensions;
	};

	constexpr std::array locations = {
		LocationDefinition { PluginLocation::era_plugins, "", true },
		LocationDefinition { PluginLocation::before_wog, "BeforeWoG", false },
		LocationDefinition { PluginLocation::after_wog, "AfterWoG", false },
	};

	bool isDisabledFilename(const std::string& filename)
	{
		return boost::iends_with(filename, ".off");
	}

	std::string canonicalFilename(const std::string& filename)
	{
		return isDisabledFilename(filename) ? filename.substr(0, filename.size() - 4) : filename;
	}

	bool supportedRootPlugin(const std::string& canonical)
	{
		return boost::iends_with(canonical, ".era") || boost::iends_with(canonical, ".dll") ||
			   boost::iends_with(canonical, ".bin");
	}

	std::string filenameUtf8(const fs::path& path)
	{
		return boost::nowide::narrow(path.filename().wstring());
	}

	fs::path pathFromUtf8(const std::string& value)
	{
		return fs::path(boost::nowide::widen(value));
	}

	bool pathIsMissing(DWORD error)
	{
		return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
	}

	[[noreturn]] void throwToggleFilesystemError(DWORD error)
	{
		throw PluginToggleError(
			PluginToggleErrorCode::filesystem, std::system_category().message(static_cast<int>(error)));
	}

	fs::path findLocationDirectory(const fs::path& modPath, const std::string& expected)
	{
		if (!is_directory(modPath))
			return {};

		const auto                   foldedExpected = boost::locale::fold_case(expected);
		boost::system::error_code    ec;
		fs::directory_iterator       it(modPath, ec);
		const fs::directory_iterator end;

		for (; !ec && it != end; it.increment(ec))
		{
			boost::system::error_code statusError;
			if (!is_directory(it->path(), statusError) || statusError)
				continue;

			if (boost::locale::fold_case(filenameUtf8(it->path())) == foldedExpected)
				return it->path();
		}

		return {};
	}

	const PluginMetadata* findMetadata(const ModData& mod, const std::string& canonical)
	{
		if (const auto it = mod.plugins.find(canonical); it != mod.plugins.end())
			return &it->second;

		const auto folded = boost::locale::fold_case(canonical);
		for (const auto& [filename, metadata] : mod.plugins)
			if (boost::locale::fold_case(filename) == folded)
				return &metadata;

		return nullptr;
	}

	struct DiscoveredPlugin
	{
		std::string canonical;
		fs::path    enabledPath;
		fs::path    disabledPath;
	};

	void discoverLocation(const ModData& mod, const fs::path& pluginRoot,
		const LocationDefinition& definition, const std::string& noDescription,
		std::vector<ModPlugin>& result)
	{
		const auto directory = definition.directory.empty()
								   ? pluginRoot
								   : findLocationDirectory(pluginRoot, definition.directory);
		if (directory.empty())
			return;

		std::map<std::string, DiscoveredPlugin> discovered;

		boost::system::error_code    ec;
		fs::directory_iterator       it(directory, ec);
		const fs::directory_iterator end;
		for (; !ec && it != end; it.increment(ec))
		{
			boost::system::error_code statusError;
			if (!is_regular_file(it->path(), statusError) || statusError)
				continue;

			boost::system::error_code sizeError;
			if (file_size(it->path(), sizeError) == 0 || sizeError)
				continue;

			const auto filename  = filenameUtf8(it->path());
			const auto canonical = canonicalFilename(filename);
			if (canonical.empty() || (definition.restrictedExtensions && !supportedRootPlugin(canonical)))
				continue;

			auto& item     = discovered[boost::locale::fold_case(canonical)];
			item.canonical = canonical;
			if (isDisabledFilename(filename))
				item.disabledPath = it->path();
			else
				item.enabledPath = it->path();
		}

		for (const auto& discoveredEntry : discovered)
		{
			const auto& item      = discoveredEntry.second;
			const bool  enabled   = !item.enabledPath.empty();
			const bool  collision = enabled && !item.disabledPath.empty();
			const auto  current   = enabled ? item.enabledPath : item.disabledPath;
			const auto  other     = enabled ? directory / pathFromUtf8(item.canonical + ".off")
											: directory / pathFromUtf8(item.canonical);

			ModPlugin plugin;
			plugin.location           = definition.location;
			plugin.canonical_filename = item.canonical;
			plugin.current_path       = current;
			plugin.counterpart_path   = other;
			plugin.enabled            = enabled;
			plugin.collision          = collision;

			const auto metadata = findMetadata(mod, item.canonical);
			plugin.caption      = metadata && !metadata->caption.empty() ? metadata->caption : item.canonical;
			plugin.description =
				metadata && !metadata->description.empty() ? metadata->description : noDescription;

			result.emplace_back(std::move(plugin));
		}
	}
}

Era2PluginManager::Era2PluginManager(std::string noDescriptionText)
	: _noDescriptionText(std::move(noDescriptionText))
{}

std::vector<ModPlugin> Era2PluginManager::list(const ModData& mod) const
{
	std::vector<ModPlugin> result;
	if (mod.virtual_mod)
		return result;

	const auto pluginRoot = findLocationDirectory(mod.data_path, "EraPlugins");
	if (pluginRoot.empty())
		return result;

	for (const auto& location : locations)
		discoverLocation(mod, pluginRoot, location, _noDescriptionText, result);

	std::ranges::sort(result, [](const ModPlugin& lhs, const ModPlugin& rhs) {
		if (lhs.location != rhs.location)
			return lhs.location < rhs.location;

		return boost::locale::fold_case(lhs.caption) < boost::locale::fold_case(rhs.caption);
	});

	return result;
}

void Era2PluginManager::toggle(const ModData& mod, const ModPlugin& plugin)
{
	const auto current         = list(mod);
	const auto foldedCanonical = boost::locale::fold_case(plugin.canonical_filename);
	const auto found           = std::ranges::find_if(current, [&](const ModPlugin& item) {
		return item.location == plugin.location &&
			   boost::locale::fold_case(item.canonical_filename) == foldedCanonical;
	});

	if (found == current.end())
		throw PluginToggleError(PluginToggleErrorCode::missing, plugin.canonical_filename);

	if (found->collision)
		throw PluginToggleError(PluginToggleErrorCode::collision, plugin.canonical_filename);
	if (found->enabled != plugin.enabled || found->current_path != plugin.current_path)
		throw PluginToggleError(PluginToggleErrorCode::missing, plugin.canonical_filename);

	const auto sourceAttributes = GetFileAttributesW(found->current_path.wstring().c_str());
	if (sourceAttributes == INVALID_FILE_ATTRIBUTES)
	{
		const auto error = GetLastError();
		if (!pathIsMissing(error))
			throwToggleFilesystemError(error);

		throw PluginToggleError(
			PluginToggleErrorCode::missing, boost::nowide::narrow(found->current_path.wstring()));
	}
	if ((sourceAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		throw PluginToggleError(
			PluginToggleErrorCode::missing, boost::nowide::narrow(found->current_path.wstring()));

	const auto counterpartAttributes = GetFileAttributesW(found->counterpart_path.wstring().c_str());
	if (counterpartAttributes != INVALID_FILE_ATTRIBUTES)
		throw PluginToggleError(
			PluginToggleErrorCode::collision, boost::nowide::narrow(found->counterpart_path.wstring()));
	if (const auto error = GetLastError(); !pathIsMissing(error))
		throwToggleFilesystemError(error);

	if (!MoveFileW(found->current_path.wstring().c_str(), found->counterpart_path.wstring().c_str()))
	{
		const auto error = GetLastError();
		if (error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS)
			throw PluginToggleError(
				PluginToggleErrorCode::collision, boost::nowide::narrow(found->counterpart_path.wstring()));
		if (pathIsMissing(error))
			throw PluginToggleError(
				PluginToggleErrorCode::missing, boost::nowide::narrow(found->current_path.wstring()));

		throwToggleFilesystemError(error);
	}
}
