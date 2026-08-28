// SD Mod Manager feature regression tests

#include "stdafx.h"

#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "era2/era2_mod_data_loader.hpp"
#include "era2/era2_mod_package_builder.hpp"
#include "era2/era2_mod_package_installer.hpp"
#include "era2/era2_plugin_manager.hpp"
#include "interface/ii18n_service.hpp"
#include "interface/imod_manager.hpp"
#include "utility/fs_util.h"

#include <windows.h>

#include <boost/locale.hpp>

#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

using namespace mm;

namespace
{
	void expect(bool condition, const std::string& message)
	{
		if (!condition)
			throw std::runtime_error(message);
	}

	void writeBytes(const fs::path& path, const std::string& bytes)
	{
		create_directories(path.parent_path());
		boost::nowide::ofstream output(path, std::ios::binary | std::ios::trunc);
		output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
		expect(static_cast<bool>(output), "Could not write fixture: " + path.string());
	}

	void testPathExistenceCheck(const fs::path& root)
	{
		const auto output = root / "path-existence" / "new package.exe";
		create_directories(output.parent_path());

		boost::system::error_code error;
		expect(!pathExists(output, error), "A new package output was incorrectly reported as existing");
		expect(!error, "A missing package output was incorrectly reported as a filesystem error");

		writeBytes(output, "existing-package");
		expect(pathExists(output, error), "An existing package output was not detected");
		expect(!error, "An existing package output produced a filesystem error");
	}

	std::string readBytes(const fs::path& path)
	{
		boost::nowide::ifstream input(path, std::ios::binary);
		return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
	}

	struct TestModManager : IModManager
	{
		explicit TestModManager(fs::path modsRoot)
			: root(std::move(modsRoot))
		{
			reload();
		}

		void reload()
		{
			ModList                  loaded;
			std::vector<std::string> lines;
			boost::split(lines, readBytes(root / "list.txt"), boost::is_any_of("\r\n"));
			for (auto reverse = lines.rbegin(); reverse != lines.rend(); ++reverse)
			{
				auto value = *reverse;
				boost::trim(value);
				if (value.empty())
					continue;
				auto state = ModList::ModState::enabled;
				if (value.front() == '*')
				{
					value.erase(value.begin());
					state = ModList::ModState::disabled;
				}
				const auto id = boost::locale::fold_case(value);
				if (!loaded.managed(id))
					loaded.data.emplace_back(id, state);
			}

			boost::system::error_code ec;
			fs::directory_iterator    it(root, ec), end;
			for (; !ec && it != end; it.increment(ec))
				if (is_directory(it->path(), ec) && !ec)
				{
					const auto id =
						boost::locale::fold_case(boost::nowide::narrow(it->path().filename().wstring()));
					if (!loaded.managed(id))
						loaded.rest.emplace(id);
				}

			list = std::move(loaded);
			changed();
		}

		ModList const& mods() const override
		{
			return list;
		}

		void apply(const std::vector<std::string>& items, bool archiveOnDisable) override
		{
			list.apply(items, archiveOnDisable);
			notify();
		}

		void enable(const std::string& item) override
		{
			list.enable(item);
			notify();
		}

		void disable(const std::string& item) override
		{
			list.disable(item);
			notify();
		}

		void archive(const std::string& item) override
		{
			list.archive(item);
			notify();
		}

		void switchState(const std::string& item) override
		{
			list.switchState(item);
			notify();
		}

		void move(const std::string& from, const std::string& to) override
		{
			list.move(from, to);
			notify();
		}

		void moveUp(const std::string& item) override
		{
			list.moveUp(item);
			notify();
		}

		void moveDown(const std::string& item) override
		{
			list.moveDown(item);
			notify();
		}

		void remove(const std::string& item) override
		{
			list.remove(item);
			notify();
		}

		sigslot::signal<>& onListChanged() override
		{
			return changed;
		}

		void save() const
		{
			std::map<std::string, std::string> actualNames;
			boost::system::error_code          ec;
			fs::directory_iterator             it(root, ec), end;
			for (; !ec && it != end; it.increment(ec))
				if (is_directory(it->path(), ec) && !ec)
				{
					const auto value = boost::nowide::narrow(it->path().filename().wstring());
					actualNames[boost::locale::fold_case(value)] = value;
				}

			std::ostringstream output;
			for (auto reverse = list.data.rbegin(); reverse != list.data.rend(); ++reverse)
			{
				const auto found = actualNames.find(reverse->id);
				const auto value = found == actualNames.end() ? reverse->id : found->second;
				if (reverse->state == ModList::ModState::disabled)
					output << '*';
				output << value << '\n';
			}
			writeBytes(root / "list.txt", output.str());
		}

		void notify()
		{
			save();
			changed();
		}

		fs::path          root;
		ModList           list;
		sigslot::signal<> changed;
	};

	const ModPlugin& findPlugin(
		const std::vector<ModPlugin>& plugins, PluginLocation location, const std::string& canonical)
	{
		const auto found = std::ranges::find_if(plugins, [&](const ModPlugin& plugin) {
			return plugin.location == location && boost::iequals(plugin.canonical_filename, canonical);
		});
		expect(found != plugins.end(), "Plugin was not discovered: " + canonical);
		return *found;
	}

	void expectToggleError(PluginToggleErrorCode expected, const std::function<void()>& action)
	{
		try
		{
			action();
		}
		catch (const PluginToggleError& error)
		{
			expect(error.code() == expected, "Unexpected plugin toggle error code");
			return;
		}

		throw std::runtime_error("Expected plugin toggle error was not raised");
	}

	struct MockI18n : II18nService
	{
		std::vector<std::string> available() const override
		{
			return {};
		}

		std::string get(const std::string& key) const override
		{
			return key == "message/status/no_description_available" ? "Localized no description" : key;
		}

		std::string category(const std::string& value) const override
		{
			return value;
		}

		std::string column(const std::string& value) const override
		{
			return value;
		}

		std::string languageName(const std::string& code) const override
		{
			return code;
		}

		std::vector<std::string> legacyCodes(const std::string& code) const override
		{
			if (code == "zh" || code == "zh_HK")
				return { "Cn_Simp", "zh_CN" };
			if (code == "en")
				return { "en_US" };
			return {};
		}
	};

	std::wstring quote(const std::wstring& value)
	{
		return L"\"" + value + L"\"";
	}

	DWORD runAndWait(
		const fs::path& executable, const std::wstring& arguments, const fs::path& workingDirectory)
	{
		std::wstring         command = quote(executable.wstring()) + L" " + arguments;
		std::vector<wchar_t> mutableCommand(command.begin(), command.end());
		mutableCommand.push_back(L'\0');

		STARTUPINFOW        startup { sizeof(startup) };
		PROCESS_INFORMATION process {};
		if (!CreateProcessW(executable.wstring().c_str(), mutableCommand.data(), nullptr, nullptr, FALSE,
				CREATE_NO_WINDOW, nullptr, workingDirectory.wstring().c_str(), &startup, &process))
			throw std::system_error(
				static_cast<int>(GetLastError()), std::system_category(), "CreateProcessW");

		CloseHandle(process.hThread);
		expect(WaitForSingleObject(process.hProcess, 120000) == WAIT_OBJECT_0, "Child process timed out");

		DWORD exitCode = 0;
		expect(GetExitCodeProcess(process.hProcess, &exitCode) != FALSE, "Could not read process exit code");
		CloseHandle(process.hProcess);
		return exitCode;
	}

	void createArchive(const fs::path& sevenZip, const fs::path& sourceRoot, const fs::path& output,
		const std::wstring& type, const fs::path& item, const std::wstring& extra = {})
	{
		boost::system::error_code ignored;
		remove(output, ignored);
		const auto arguments = L"a -y -bd -bso0 -bse1 " + type + L" " + extra + L" " +
							   quote(output.wstring()) + L" " + quote(item.wstring());
		expect(runAndWait(sevenZip, arguments, sourceRoot) == 0,
			"Could not create archive fixture: " + output.string());
	}

	std::size_t replaceArchivePath(std::string& archive, const std::string& from, const std::string& to)
	{
		expect(from.size() == to.size(), "Archive path patch must preserve the ZIP filename length");
		std::size_t replaced = 0;
		for (std::size_t position = 0; (position = archive.find(from, position)) != std::string::npos;)
		{
			archive.replace(position, from.size(), to);
			position += to.size();
			++replaced;
		}
		return replaced;
	}

	void waitForPackage(const ModPackageJob& job)
	{
		HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, job.process_id);
		expect(process != nullptr, "Could not open package process");
		expect(WaitForSingleObject(process, 120000) == WAIT_OBJECT_0, "Package process timed out");

		DWORD exitCode = 0;
		expect(GetExitCodeProcess(process, &exitCode) != FALSE, "Could not read package exit code");
		CloseHandle(process);
		expect(exitCode == 0, "7-Zip package process failed");
		expect(is_regular_file(job.output_path), "Package output was not created");

		for (int attempt = 0; attempt < 100 && exists(job.job_directory); ++attempt)
			Sleep(50);
		expect(!exists(job.job_directory), "Completed package job directory was not cleaned");
	}

	std::vector<unsigned char> resourceBytes(const fs::path& modulePath, LPCWSTR type, WORD id)
	{
		HMODULE module = LoadLibraryExW(
			modulePath.wstring().c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
		expect(module != nullptr, "Could not load PE resources: " + modulePath.string());

		HRSRC resource =
			FindResourceExW(module, type, MAKEINTRESOURCEW(id), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
		if (!resource)
			resource = FindResourceW(module, MAKEINTRESOURCEW(id), type);
		expect(resource != nullptr, "Expected PE resource was not found");

		const auto size   = SizeofResource(module, resource);
		HGLOBAL    loaded = LoadResource(module, resource);
		const auto data   = static_cast<const unsigned char*>(LockResource(loaded));
		expect(data != nullptr && size > 0, "Expected PE resource was empty");

		std::vector<unsigned char> result(data, data + size);
		FreeLibrary(module);
		return result;
	}

	std::wstring firstStringResource(const fs::path& modulePath)
	{
		const auto data = resourceBytes(modulePath, RT_STRING, 251);
		expect(data.size() >= sizeof(WORD), "String resource is malformed");

		WORD size = 0;
		std::memcpy(&size, data.data(), sizeof(size));
		expect(data.size() >= sizeof(WORD) + size * sizeof(wchar_t), "String resource is truncated");
		return { reinterpret_cast<const wchar_t*>(data.data() + sizeof(WORD)), size };
	}

	std::vector<unsigned char> firstIcoImage(const fs::path& iconPath)
	{
		const auto bytes = readBytes(iconPath);
		expect(bytes.size() >= 22, "ICO fixture is too small");

		DWORD imageSize = 0;
		DWORD offset    = 0;
		std::memcpy(&imageSize, bytes.data() + 14, sizeof(imageSize));
		std::memcpy(&offset, bytes.data() + 18, sizeof(offset));
		expect(static_cast<std::uint64_t>(offset) + imageSize <= bytes.size(), "ICO fixture is malformed");

		return { reinterpret_cast<const unsigned char*>(bytes.data() + offset),
			reinterpret_cast<const unsigned char*>(bytes.data() + offset + imageSize) };
	}

	void testMetadataLoading(const fs::path& root)
	{
		const auto modPath = root / L"Metadata 模组";
		create_directories(modPath);
		writeBytes(modPath / "mod.json", R"json({
  "plugins": {
    "localized.era": {
      "caption": { "zh": "Current caption", "Cn_Simp": "First legacy caption", "zh_CN": "Second legacy caption", "en": "English caption" },
      "description": { "zh": "Current description", "Cn_Simp": "First legacy description", "zh_CN": "Second legacy description", "en": "English description" }
    },
    "second-legacy.dll": {
      "caption": { "zh_CN": "Second legacy only caption", "en": "English caption" },
      "description": { "zh_CN": "Second legacy only description", "en": "English description" }
    },
    "fallback.dll": {},
    "plain.bin": { "caption": "Plain caption", "description": "Plain description" },
    "legacy.dll": {
      "caption": { "en_US": "Legacy English caption" },
      "description": { "en_US": "Legacy English description" }
    }
  }
})json");

		MockI18n                    i18n;
		const std::set<std::string> empty;

		const auto current = Era2ModDataLoader::load("metadata", modPath, "zh", empty, empty, empty, i18n);
		expect(current.plugins.at("localized.era").caption == "Current caption",
			"Current language fallback failed");
		expect(current.plugins.at("second-legacy.dll").caption == "Second legacy only caption" &&
				   current.plugins.at("second-legacy.dll").description == "Second legacy only description",
			"Second legacy language fallback failed");

		const auto legacy = Era2ModDataLoader::load("metadata", modPath, "zh_HK", empty, empty, empty, i18n);
		expect(legacy.plugins.at("localized.era").caption == "First legacy caption",
			"Legacy language precedence failed");

		const auto english = Era2ModDataLoader::load("metadata", modPath, "de", empty, empty, empty, i18n);
		expect(english.plugins.at("localized.era").caption == "English caption", "English fallback failed");
		expect(english.plugins.at("fallback.dll").caption == "fallback.dll", "Filename fallback failed");
		expect(english.plugins.at("fallback.dll").description == "Localized no description",
			"Description fallback failed");
		expect(english.plugins.at("plain.bin").caption == "Plain caption", "Plain metadata failed");

		const auto legacyEnglish =
			Era2ModDataLoader::load("metadata", modPath, "en", empty, empty, empty, i18n);
		expect(legacyEnglish.plugins.at("legacy.dll").caption == "Legacy English caption" &&
				   legacyEnglish.plugins.at("legacy.dll").description == "Legacy English description",
			"Legacy AutoIt plugin metadata compatibility failed");

		const auto legacyPath = root / L"Legacy metadata 模组";
		writeBytes(legacyPath / "mod_info.ini", R"ini([info]
Caption.Cn_Simp=旧格式名称
Caption.zh_CN=不应优先使用
Caption.en=Legacy English name
Description File.zh_CN=说明.txt
Description File.en=readme.txt
Version=3.4.5
Author=Legacy Author
Homepage=https://example.invalid/legacy
Icon File=legacy.ico
Priority=17
Category=Gameplay
)ini");
		writeBytes(legacyPath / L"说明.txt", "Legacy localized description");
		const auto legacyIni =
			Era2ModDataLoader::load("legacy", legacyPath, "zh", empty, empty, empty, i18n);
		expect(legacyIni.legacy_format, "mod_info.ini was not marked as legacy metadata");
		expect(legacyIni.name == "旧格式名称" && legacyIni.description == fs::path(L"说明.txt"),
			"Legacy INI language fallback failed");
		expect(legacyIni.version == "3.4.5" && legacyIni.author == "Legacy Author" &&
				   legacyIni.homepage == "https://example.invalid/legacy" && legacyIni.icon == "legacy.ico" &&
				   legacyIni.priority == 17 && legacyIni.category == "gameplay",
			"Legacy INI metadata fields were not loaded");
	}

	void testPluginManager(const fs::path& root)
	{
		const auto modPath = root / L"Mods" / L"Unicode 模组";
		writeBytes(modPath / "eRaPlUgInS" / "alpha.era", "alpha-bytes");
		writeBytes(modPath / "eRaPlUgInS" / "beta.dll.off", "beta-disabled");
		writeBytes(modPath / "eRaPlUgInS" / "gamma.BIN", "gamma-bytes");
		writeBytes(modPath / "eRaPlUgInS" / L"ユニコード.era.off", "unicode-bytes");
		writeBytes(modPath / "eRaPlUgInS" / "artifact.pdb", "ignored");
		writeBytes(modPath / "eRaPlUgInS" / "artifact.lib", "ignored");
		writeBytes(modPath / "eRaPlUgInS" / "ignored.exe", "ignored");
		writeBytes(modPath / "eRaPlUgInS" / "zero.era", "");
		writeBytes(modPath / "eRaPlUgInS" / "nested" / "nested.era", "ignored");
		writeBytes(modPath / "eRaPlUgInS" / "beforewog" / "extensionless patch.off", "before-bytes");
		writeBytes(modPath / "eRaPlUgInS" / "AFTERWOG" / "arbitrary.patch", "after-bytes");

		ModData mod;
		mod.data_path = modPath;
		mod.dir       = boost::nowide::narrow(modPath.filename().wstring());
		mod.name      = "Unicode mod";
		mod.plugins.emplace("alpha.era", PluginMetadata { "Alpha caption", "Alpha description" });

		Era2PluginManager manager("No description fixture");
		auto              plugins = manager.list(mod);
		expect(plugins.size() == 6, "Unexpected discovered plugin count");

		auto alpha = findPlugin(plugins, PluginLocation::era_plugins, "alpha.era");
		expect(alpha.enabled && alpha.caption == "Alpha caption" && alpha.description == "Alpha description",
			"Plugin metadata or state failed");
		expect(!findPlugin(plugins, PluginLocation::era_plugins, "beta.dll").enabled,
			"Terminal .off detection failed");
		expect(!findPlugin(plugins, PluginLocation::era_plugins,
				   boost::nowide::narrow(std::wstring(L"ユニコード.era")))
				   .enabled,
			"Unicode plugin filename detection failed");
		expect(!findPlugin(plugins, PluginLocation::before_wog, "extensionless patch").enabled,
			"Extensionless patch detection failed");
		expect(findPlugin(plugins, PluginLocation::after_wog, "arbitrary.patch").enabled,
			"AfterWoG file detection failed");

		const auto alphaBytes = readBytes(alpha.current_path);
		try
		{
			manager.toggle(mod, alpha);
		}
		catch (const std::exception& error)
		{
			throw std::runtime_error(
				"Initial disable failed for '" + boost::nowide::narrow(alpha.current_path.wstring()) +
				"' -> '" + boost::nowide::narrow(alpha.counterpart_path.wstring()) + "': " + error.what());
		}
		expect(!exists(modPath / "eRaPlUgInS" / "alpha.era") &&
				   is_regular_file(modPath / "eRaPlUgInS" / "alpha.era.off"),
			"Disable rename failed");
		expect(readBytes(modPath / "eRaPlUgInS" / "alpha.era.off") == alphaBytes,
			"Disable rename changed file bytes");

		alpha.enabled = !alpha.enabled;
		std::swap(alpha.current_path, alpha.counterpart_path);
		expect(!alpha.enabled, "In-place plugin state did not refresh");
		manager.toggle(mod, alpha);
		expect(readBytes(modPath / "eRaPlUgInS" / "alpha.era") == alphaBytes,
			"In-place enable rename changed file bytes");

		writeBytes(modPath / "eRaPlUgInS" / "beta.dll", "beta-enabled");
		plugins              = manager.list(mod);
		const auto collision = findPlugin(plugins, PluginLocation::era_plugins, "beta.dll");
		expect(collision.collision, "Enabled/disabled collision was not detected");
		expectToggleError(PluginToggleErrorCode::collision, [&] { manager.toggle(mod, collision); });
		expect(readBytes(modPath / "eRaPlUgInS" / "beta.dll") == "beta-enabled" &&
				   readBytes(modPath / "eRaPlUgInS" / "beta.dll.off") == "beta-disabled",
			"Collision changed one of the files");

		const auto gamma = findPlugin(plugins, PluginLocation::era_plugins, "gamma.BIN");
		remove(gamma.current_path);
		expectToggleError(PluginToggleErrorCode::missing, [&] { manager.toggle(mod, gamma); });

		const auto after = findPlugin(plugins, PluginLocation::after_wog, "arbitrary.patch");
		rename(after.current_path, after.counterpart_path);
		expectToggleError(PluginToggleErrorCode::missing, [&] { manager.toggle(mod, after); });
		expect(is_regular_file(after.counterpart_path) && !exists(after.current_path),
			"Stale plugin state was unexpectedly toggled");

		const auto lockedPath = modPath / "eRaPlUgInS" / "locked.era";
		writeBytes(lockedPath, "locked-bytes");
		plugins           = manager.list(mod);
		const auto locked = findPlugin(plugins, PluginLocation::era_plugins, "locked.era");
		HANDLE     lock   = CreateFileW(lockedPath.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		expect(lock != INVALID_HANDLE_VALUE, "Could not lock permission fixture");
		expectToggleError(PluginToggleErrorCode::filesystem, [&] { manager.toggle(mod, locked); });
		CloseHandle(lock);
		expect(readBytes(lockedPath) == "locked-bytes" && !exists(fs::path(lockedPath.string() + ".off")),
			"Permission failure changed the plugin");

		mod.virtual_mod = true;
		expect(manager.list(mod).empty(), "Virtual mod unexpectedly exposed plugins");
	}

	fs::path testPackageBuilder(const fs::path& root, const fs::path& repository)
	{
		const auto gameRoot = root / L"ERA root with spaces Ω";
		const auto modPath  = gameRoot / "Mods" / L"Package 模组";
		writeBytes(modPath / "Data" / "nested.txt", "nested-data");
		writeBytes(modPath / "root.bin", "root-data");
		copy_file(repository / "icons" / "main-icon.ico", modPath / "custom.ico");

		ModData mod;
		mod.data_path = modPath;
		mod.dir       = boost::nowide::narrow(modPath.filename().wstring());
		mod.name      = "Package fixture";
		mod.icon      = "custom.ico";

		const auto jobsRoot  = fs::temp_directory_path() / "SD Mod Manager" / "package-jobs";
		const auto staleJob  = jobsRoot / fs::unique_path("stale-test-%%%%-%%%%");
		const auto activeJob = jobsRoot / fs::unique_path("active-test-%%%%-%%%%");
		writeBytes(staleJob / "pid.txt", "4294967295\n");
		writeBytes(activeJob / "pid.txt", std::to_string(GetCurrentProcessId()) + "\n");

		Era2ModPackageBuilder builder(gameRoot, repository);
		expect(!exists(staleJob), "Stale package job was not cleaned on startup");
		expect(exists(activeJob), "Cleanup disturbed a still-running package marker");
		remove_all(activeJob);
		const auto output = root / L"Output Ω with spaces.exe";
		writeBytes(output, "replace-me");
		const auto customJob = builder.start(mod, output);
		waitForPackage(customJob);

		expect(firstStringResource(output) == L"\"Package 模组\"", "Quoted SFX mod argument is incorrect");
		expect(resourceBytes(output, RT_ICON, 1) == firstIcoImage(modPath / "custom.ico"),
			"Custom SFX icon was not installed");

		const auto extracted = root / L"Extracted Ω";
		create_directories(extracted);
		const auto sevenZip = repository / "7z" / "7z.exe";
		const auto extractArguments =
			L"x " + quote(output.wstring()) + L" -o" + quote(extracted.wstring()) + L" -y";
		expect(
			runAndWait(sevenZip, extractArguments, repository) == 0, "Could not inspect generated package");
		expect(readBytes(extracted / "Mods" / L"Package 模组" / "Data" / "nested.txt") == "nested-data",
			"Nested package content is missing");
		expect(readBytes(extracted / "Mods" / L"Package 模组" / "root.bin") == "root-data",
			"Root package content is missing");
		expect(is_regular_file(extracted / "Mods" / L"Package 模组" / "custom.ico"),
			"Package did not preserve the mod icon file");

		std::size_t extractedFileCount = 0;
		for (const auto& entry : fs::recursive_directory_iterator(extracted))
			if (is_regular_file(entry.path()))
				++extractedFileCount;
		expect(extractedFileCount == 3, "Package content was incomplete or contained unexpected files");

		std::vector<fs::path> extractedRoots;
		for (const auto& entry : fs::directory_iterator(extracted))
			extractedRoots.emplace_back(entry.path().filename());
		expect(extractedRoots.size() == 1 && boost::iequals(extractedRoots.front().string(), "Mods"),
			"Package has content outside its single Mods root");

		writeBytes(modPath / "invalid.ico", "not-an-icon");
		mod.icon                 = "invalid.ico";
		const auto defaultOutput = root / L"Default icon package.exe";
		const auto defaultJob    = builder.start(mod, defaultOutput);
		waitForPackage(defaultJob);
		expect(resourceBytes(defaultOutput, RT_GROUP_ICON, 1) ==
				   resourceBytes(repository / "7z" / "7z.sfx", RT_GROUP_ICON, 1),
			"Invalid ICO did not preserve the default SFX icon");

		return output;
	}

	const PackagedMod& findPackagedMod(const ModPackageInspection& inspection, const std::string& directory)
	{
		const auto found = std::ranges::find_if(inspection.mods,
			[&](const PackagedMod& mod) { return boost::iequals(mod.directory, directory); });
		expect(found != inspection.mods.end(), "Packaged mod was not detected: " + directory);
		return *found;
	}

	std::vector<std::size_t> selectableMods(const ModPackageInspection& inspection)
	{
		std::vector<std::size_t> result;
		for (std::size_t index = 0; index < inspection.mods.size(); ++index)
			if (inspection.mods[index].selectable)
				result.emplace_back(index);
		return result;
	}

	void testPackageInstaller(const fs::path& root, const fs::path& repository, const fs::path& sfxPackage)
	{
		const auto sevenZip = repository / "7z" / "7z.exe";
		const auto sourceA  = root / L"Installer source A Ω";
		writeBytes(sourceA / "Mods" / "Existing Mod" / "mod.json", R"json({
  "name": { "en": "Existing packaged mod" },
  "description": { "en": "readme.txt" },
  "version": "0.5"
})json");
		writeBytes(sourceA / "Mods" / "Existing Mod" / "readme.txt", "Replacement description");
		writeBytes(sourceA / "Mods" / "Existing Mod" / "Data" / "payload.bin", "replacement-bytes");
		writeBytes(sourceA / "Mods" / L"New 模组" / "mod.json", R"json({
  "name": { "en": "New Unicode mod" },
  "description": { "en": "readme.md" },
  "version": "2.0"
})json");
		writeBytes(sourceA / "Mods" / L"New 模组" / "readme.md", "New Unicode description");
		writeBytes(sourceA / "Mods" / L"New 模组" / "payload.txt", "new-bytes");
		const auto archiveA = root / L"multi mod package Ω.7z";
		createArchive(sevenZip, sourceA, archiveA, L"-t7z", "Mods");

		const auto sourceB = root / L"Installer source B";
		writeBytes(sourceB / "Mods" / "Legacy Mod" / "mod_info.ini", R"ini([info]
Caption.en=Legacy package caption
Description File.en=legacy-readme.txt
Version=9.1
Author=Legacy Packager
)ini");
		writeBytes(sourceB / "Mods" / "Legacy Mod" / "legacy-readme.txt", "Legacy package description");
		writeBytes(sourceB / "Mods" / "Legacy Mod" / "legacy.bin", "legacy-bytes");
		const auto archiveB = root / "legacy package.zip";
		createArchive(sevenZip, sourceB, archiveB, L"-tzip", "Mods");

		std::vector<fs::path> validPackages { archiveA, archiveB, sfxPackage };
		const fs::path        rarTool = L"C:\\Program Files (x86)\\WinRAR\\Rar.exe";
		if (is_regular_file(rarTool))
		{
			const auto sourceRar = root / "Installer RAR source";
			writeBytes(sourceRar / "Mods" / "RAR Mod" / "mod.json",
				R"json({"name":{"en":"RAR package mod"},"version":"1.7"})json");
			writeBytes(sourceRar / "Mods" / "RAR Mod" / "rar.bin", "rar-bytes");
			const auto                archiveRar = root / "package fixture.rar";
			boost::system::error_code ignored;
			remove(archiveRar, ignored);
			expect(runAndWait(rarTool,
					   L"a -r -idq " + quote(archiveRar.wstring()) + L" " + quote(std::wstring(L"Mods")),
					   sourceRar) == 0,
				"Could not create RAR fixture");
			validPackages.emplace_back(archiveRar);
		}

		const auto gameRoot = root / L"Installer game Ω";
		writeBytes(gameRoot / "Mods" / "Existing Mod" / "mod.json",
			R"json({"name":{"en":"Existing installed mod"},"version":"1.0"})json");
		writeBytes(gameRoot / "Mods" / "Existing Mod" / "Data" / "payload.bin", "original-bytes");
		writeBytes(gameRoot / "Mods" / "Other Mod" / "mod.json",
			R"json({"name":{"en":"Unrelated installed mod"},"version":"4.0"})json");
		writeBytes(gameRoot / "Mods" / "Other Mod" / "payload.bin", "unrelated-bytes");
		writeBytes(gameRoot / "Mods" / "list.txt", "*Existing Mod\nOther Mod\n");

		MockI18n                i18n;
		TestModManager          manager(gameRoot / "Mods");
		const auto              previousExistingPosition = manager.mods().position("existing mod");
		const auto              previousOtherPosition    = manager.mods().position("other mod");
		Era2ModPackageInstaller installer(
			gameRoot, repository, "en", i18n, manager, [&] { manager.reload(); });

		auto inspection = installer.inspect(validPackages, [](const ModPackageProgress&) { return true; });
		expect(inspection.diagnostics.empty(), "Valid package batch produced diagnostics");
		expect(inspection.mods.size() == (is_regular_file(rarTool) ? 5u : 4u),
			"Not every mod in the valid package batch was detected");
		const auto& existing = findPackagedMod(inspection, "Existing Mod");
		expect(
			existing.installed && existing.metadata.version == "0.5" && existing.installed_version == "1.0",
			"Reinstall version comparison data is incorrect");
		const auto& legacy = findPackagedMod(inspection, "Legacy Mod");
		expect(legacy.metadata.name == "Legacy package caption" && legacy.metadata.version == "9.1" &&
				   legacy.description == "Legacy package description",
			"Legacy package preview metadata is incorrect");
		expect(findPackagedMod(inspection, boost::nowide::narrow(std::wstring(L"New 模组"))).description ==
				   "New Unicode description",
			"Unicode package metadata or description failed");

		const auto installed = installer.install(
			inspection, selectableMods(inspection), [](const ModPackageProgress&) { return true; });
		expect(installed.installed_mod_ids.size() == inspection.mods.size(),
			"Package installer did not commit every selected mod");
		expect(readBytes(gameRoot / "Mods" / "Existing Mod" / "Data" / "payload.bin") == "replacement-bytes",
			"Reinstall did not replace the existing mod contents");
		expect(readBytes(gameRoot / "Mods" / L"New 模组" / "payload.txt") == "new-bytes" &&
				   readBytes(gameRoot / "Mods" / "Legacy Mod" / "legacy.bin") == "legacy-bytes" &&
				   is_directory(gameRoot / "Mods" / L"Package 模组"),
			"New mods from 7z, ZIP, or SFX packages were not installed");
		if (is_regular_file(rarTool))
			expect(readBytes(gameRoot / "Mods" / "RAR Mod" / "rar.bin") == "rar-bytes",
				"RAR mod was not installed");
		expect(manager.mods().disabled("existing mod"), "Reinstall did not preserve the disabled state");
		expect(manager.mods().position("existing mod") == previousExistingPosition &&
				   manager.mods().position("other mod") == previousOtherPosition &&
				   readBytes(gameRoot / "Mods" / "Other Mod" / "payload.bin") == "unrelated-bytes",
			"Reinstall changed an existing mod priority or unrelated contents");
		expect(manager.mods().enabled(
				   boost::locale::fold_case(boost::nowide::narrow(std::wstring(L"New 模组")))) &&
				   manager.mods().enabled("legacy mod"),
			"New mods were not enabled");

		auto duplicate =
			installer.inspect({ archiveA, archiveA }, [](const ModPackageProgress&) { return true; });
		expect(!duplicate.mods.empty() && std::ranges::all_of(duplicate.mods,
											  [](const PackagedMod& mod) { return !mod.selectable; }),
			"Duplicate target directories were not rejected across the package batch");
		installer.discard(duplicate);

		const auto collisionSource = root / "Collision package source";
		writeBytes(collisionSource / "Mods" / "Collision Mod" / "payload.bin", "collision-package");
		const auto collisionArchive = root / "collision-package.7z";
		createArchive(sevenZip, collisionSource, collisionArchive, L"-t7z", "Mods");
		writeBytes(gameRoot / "Mods" / "Collision Mod", "target-file");
		auto collision =
			installer.inspect({ collisionArchive }, [](const ModPackageProgress&) { return true; });
		expect(collision.mods.size() == 1 && !collision.mods.front().selectable &&
				   std::ranges::any_of(collision.diagnostics,
					   [](const ModPackageDiagnostic& item) {
						   return item.code == ModPackageDiagnosticCode::target_collision;
					   }),
			"A file occupying a mod target was not rejected during review");
		installer.discard(collision);
		remove(gameRoot / "Mods" / "Collision Mod");

		const auto invalidSource = root / "Invalid package sources";
		writeBytes(invalidSource / "outside" / "Tools" / "bad.txt", "outside");
		const auto outsideArchive = root / "outside.zip";
		createArchive(sevenZip, invalidSource / "outside", outsideArchive, L"-tzip", "Tools");
		writeBytes(invalidSource / "direct" / "Mods" / "bad.txt", "direct");
		const auto directArchive = root / "direct.7z";
		createArchive(sevenZip, invalidSource / "direct", directArchive, L"-t7z", "Mods");
		writeBytes(invalidSource / "encrypted" / "Mods" / "Secret Mod" / "secret.bin", "secret");
		const auto encryptedArchive = root / "encrypted.zip";
		createArchive(sevenZip, invalidSource / "encrypted", encryptedArchive, L"-tzip", "Mods",
			L"-psecret -mem=AES256");
		const auto corruptArchive = root / "corrupt.rar";
		writeBytes(corruptArchive, "not-an-archive");
		writeBytes(invalidSource / "traversal" / "Mods" / "XX" / "evil.txt", "traversal");
		const auto traversalArchive = root / "traversal.zip";
		createArchive(sevenZip, invalidSource / "traversal", traversalArchive, L"-tzip",
			fs::path("Mods") / "XX" / "evil.txt");
		auto traversalBytes   = readBytes(traversalArchive);
		auto traversalPatches = replaceArchivePath(traversalBytes, "Mods/XX/evil.txt", "Mods/../evil.txt");
		traversalPatches += replaceArchivePath(traversalBytes, "Mods\\XX\\evil.txt", "Mods\\..\\evil.txt");
		expect(traversalPatches >= 2, "Could not patch traversal ZIP fixture filenames");
		writeBytes(traversalArchive, traversalBytes);

		writeBytes(invalidSource / "invalid-name" / "Mods" / "Safe" / "file.txt", "invalid-name");
		const auto invalidNameArchive = root / "invalid-name.zip";
		createArchive(sevenZip, invalidSource / "invalid-name", invalidNameArchive, L"-tzip",
			fs::path("Mods") / "Safe" / "file.txt");
		auto invalidNameBytes = readBytes(invalidNameArchive);
		auto invalidNamePatches =
			replaceArchivePath(invalidNameBytes, "Mods/Safe/file.txt", "Mods/CON /file.txt");
		invalidNamePatches +=
			replaceArchivePath(invalidNameBytes, "Mods\\Safe\\file.txt", "Mods\\CON \\file.txt");
		expect(invalidNamePatches >= 2, "Could not patch invalid-name ZIP fixture filenames");
		writeBytes(invalidNameArchive, invalidNameBytes);

		auto invalid = installer.inspect({ outsideArchive, directArchive, encryptedArchive, corruptArchive,
											 traversalArchive, invalidNameArchive },
			[](const ModPackageProgress&) { return true; });
		expect(invalid.mods.empty() && invalid.diagnostics.size() >= 6,
			"Unsafe, encrypted, or corrupt packages were not all rejected");
		auto hasDiagnostic = [&](ModPackageDiagnosticCode code) {
			return std::ranges::any_of(
				invalid.diagnostics, [&](const ModPackageDiagnostic& item) { return item.code == code; });
		};
		expect(hasDiagnostic(ModPackageDiagnosticCode::outside_mods) &&
				   hasDiagnostic(ModPackageDiagnosticCode::direct_mods_file) &&
				   hasDiagnostic(ModPackageDiagnosticCode::encrypted_content) &&
				   hasDiagnostic(ModPackageDiagnosticCode::corrupt_archive) &&
				   hasDiagnostic(ModPackageDiagnosticCode::unsafe_path) &&
				   hasDiagnostic(ModPackageDiagnosticCode::invalid_windows_name),
			"Invalid package diagnostics did not identify every rejected content class");
		installer.discard(invalid);

		int  scanCallbacks = 0;
		bool scanCancelled = false;
		try
		{
			[[maybe_unused]] const auto cancelledInspection = installer.inspect(
				{ archiveA }, [&](const ModPackageProgress&) { return ++scanCallbacks < 2; });
		}
		catch (const ModPackageOperationCancelled&)
		{
			scanCancelled = true;
		}
		expect(scanCancelled, "Package inspection cancellation was not honoured");

		auto stagingCancellation =
			installer.inspect({ archiveA }, [](const ModPackageProgress&) { return true; });
		const auto beforeCancelList = readBytes(gameRoot / "Mods" / "list.txt");
		const auto beforeCancelData = readBytes(gameRoot / "Mods" / "Existing Mod" / "Data" / "payload.bin");
		bool       stagingCancelled = false;
		try
		{
			[[maybe_unused]] const auto cancelledInstall = installer.install(stagingCancellation,
				selectableMods(stagingCancellation), [](const ModPackageProgress& state) {
					return state.phase != ModPackageProgressPhase::staging;
				});
		}
		catch (const ModPackageOperationCancelled&)
		{
			stagingCancelled = true;
		}
		installer.discard(stagingCancellation);
		expect(stagingCancelled && readBytes(gameRoot / "Mods" / "list.txt") == beforeCancelList &&
				   readBytes(gameRoot / "Mods" / "Existing Mod" / "Data" / "payload.bin") == beforeCancelData,
			"Staging cancellation changed the live game");

		const auto rollbackGame = root / "Rollback game";
		writeBytes(rollbackGame / "Mods" / "Existing Mod" / "Data" / "payload.bin", "rollback-original");
		writeBytes(rollbackGame / "Mods" / "Existing Mod" / "mod.json",
			R"json({"name":{"en":"Rollback original"},"version":"1.0"})json");
		writeBytes(rollbackGame / "Mods" / "list.txt", "*Existing Mod\n");
		TestModManager          rollbackManager(rollbackGame / "Mods");
		bool                    failNextReload = false;
		Era2ModPackageInstaller rollbackInstaller(rollbackGame, repository, "en", i18n, rollbackManager, [&] {
			if (failNextReload)
			{
				failNextReload = false;
				throw std::runtime_error("simulated reload failure");
			}
			rollbackManager.reload();
		});
		auto                    rollbackInspection =
			rollbackInstaller.inspect({ archiveA }, [](const ModPackageProgress&) { return true; });
		failNextReload                = true;
		bool rollbackFailedAsExpected = false;
		try
		{
			[[maybe_unused]] const auto failedInstall = rollbackInstaller.install(rollbackInspection,
				selectableMods(rollbackInspection), [](const ModPackageProgress&) { return true; });
		}
		catch (const std::runtime_error&)
		{
			rollbackFailedAsExpected = true;
		}
		expect(rollbackFailedAsExpected &&
				   readBytes(rollbackGame / "Mods" / "Existing Mod" / "Data" / "payload.bin") ==
					   "rollback-original" &&
				   !exists(rollbackGame / "Mods" / L"New 模组") &&
				   readBytes(rollbackGame / "Mods" / "list.txt") == "*Existing Mod\n",
			"A commit failure did not restore the whole selected batch");

		const auto recoveryGame = root / "Recovery game";
		const auto recoveryJob  = recoveryGame / ".sdmm-import-jobs" / "inspection-recovery";
		writeBytes(recoveryGame / "Mods" / "Existing Mod" / "payload.bin", "interrupted-replacement");
		writeBytes(recoveryGame / "Mods" / "list.txt", "Existing Mod\n");
		writeBytes(recoveryJob / "transaction" / "rollback" / "0" / "payload.bin", "recovery-original");
		writeBytes(recoveryJob / "transaction" / "list.txt.snapshot", "*Existing Mod\n");
		writeBytes(recoveryJob / "transaction" / "journal.json", R"json({
  "version": 1,
  "state": "prepared",
  "list_existed": true,
  "items": [{ "directory": "Existing Mod", "had_existing": true }]
})json");
		writeBytes(recoveryJob / "owner.pid", "4294967295\n");
		TestModManager          recoveryManager(recoveryGame / "Mods");
		Era2ModPackageInstaller recoveryInstaller(
			recoveryGame, repository, "en", i18n, recoveryManager, [&] { recoveryManager.reload(); });
		expect(readBytes(recoveryGame / "Mods" / "Existing Mod" / "payload.bin") == "recovery-original" &&
				   readBytes(recoveryGame / "Mods" / "list.txt") == "*Existing Mod\n" &&
				   recoveryManager.mods().disabled("existing mod") && !exists(recoveryJob),
			"Interrupted transaction recovery did not restore the previous installation");
	}
}

int wmain(int argc, wchar_t* argv[])
{
	try
	{
		boost::locale::generator localeGenerator;
		std::locale::global(localeGenerator(""));

		expect(argc == 2, "Usage: feature_tests.exe <repository-root>");
		const fs::path repository(argv[1]);
		expect(is_directory(repository / "7z"), "Repository 7z payload is missing");

		const auto root = fs::temp_directory_path() / fs::unique_path("sdmm-feature-tests-%%%%-%%%%-%%%%");
		create_directories(root);
		try
		{
			testPathExistenceCheck(root);
			testMetadataLoading(root);
			testPluginManager(root);
			const auto sfxPackage = testPackageBuilder(root, repository);
			testPackageInstaller(root, repository, sfxPackage);
		}
		catch (...)
		{
			std::wcerr << L"Fixture retained for diagnosis: " << root.wstring() << L'\n';
			throw;
		}

		if (GetEnvironmentVariableW(L"SDMM_KEEP_TEST_FIXTURES", nullptr, 0) != 0)
		{
			std::wcout << L"Fixture retained by request: " << root.wstring() << L'\n';
		}
		else
		{
			boost::system::error_code ec;
			remove_all(root, ec);
		}
		std::cout << "All feature regression tests passed.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "Feature regression test failure: " << error.what() << '\n';
		return 1;
	}
}
