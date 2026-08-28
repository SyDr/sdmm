// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_mod_package_installer.hpp"

#include "domain/mod_list.hpp"
#include "era2_mod_data_loader.hpp"
#include "interface/ii18n_service.hpp"
#include "interface/imod_manager.hpp"
#include "utility/json_util.h"

#include <windows.h>

#include <boost/locale/conversion.hpp>

#include <array>
#include <cctype>
#include <chrono>
#include <map>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>

using namespace mm;

namespace
{
	constexpr auto journalFilename = "journal.json";
	constexpr auto ownerFilename   = "owner.pid";
	constexpr auto childFilename   = "child.pid";

	struct ArchiveEntry
	{
		std::string path;
		bool        directory = false;
		bool        encrypted = false;
		bool        link      = false;
	};

	struct ProcessResult
	{
		DWORD       exitCode = ERROR_GEN_FAILURE;
		std::string output;
	};

	struct ScopedHandle
	{
		HANDLE value = nullptr;

		ScopedHandle() = default;
		explicit ScopedHandle(HANDLE handle)
			: value(handle)
		{}

		~ScopedHandle()
		{
			if (value && value != INVALID_HANDLE_VALUE)
				CloseHandle(value);
		}

		ScopedHandle(const ScopedHandle&)            = delete;
		ScopedHandle& operator=(const ScopedHandle&) = delete;

		ScopedHandle(ScopedHandle&& other) noexcept
			: value(std::exchange(other.value, nullptr))
		{}

		ScopedHandle& operator=(ScopedHandle&& other) noexcept
		{
			if (this != &other)
			{
				if (value && value != INVALID_HANDLE_VALUE)
					CloseHandle(value);
				value = std::exchange(other.value, nullptr);
			}
			return *this;
		}
	};

	[[noreturn]] void throwLastError(const char* operation)
	{
		throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
	}

	std::string utf8(const fs::path& path)
	{
		return boost::nowide::narrow(path.wstring());
	}

	fs::path pathFromUtf8(const std::string& value)
	{
		return fs::path(boost::nowide::widen(value));
	}

	std::wstring quote(const std::wstring& value)
	{
		std::wstring result  = L"\"";
		std::size_t  slashes = 0;

		for (const wchar_t ch : value)
		{
			if (ch == L'\\')
			{
				++slashes;
				continue;
			}

			if (ch == L'\"')
				result.append(slashes * 2 + 1, L'\\');
			else
				result.append(slashes, L'\\');

			slashes = 0;
			result.push_back(ch);
		}

		result.append(slashes * 2, L'\\');
		result.push_back(L'\"');
		return result;
	}

	void writePid(const fs::path& path, DWORD pid)
	{
		boost::nowide::ofstream output(path, std::ios::trunc);
		if (!output)
			throw std::runtime_error("Cannot create package installer process marker");

		output << pid << '\n';
		if (!output)
			throw std::runtime_error("Cannot update package installer process marker");
	}

	DWORD readPid(const fs::path& path)
	{
		boost::nowide::ifstream input(path);
		DWORD                   result = 0;
		input >> result;
		return result;
	}

	bool processRunning(DWORD pid)
	{
		if (pid == 0)
			return false;

		ScopedHandle process(OpenProcess(SYNCHRONIZE, FALSE, pid));
		if (!process.value)
			return GetLastError() == ERROR_ACCESS_DENIED;

		return WaitForSingleObject(process.value, 0) == WAIT_TIMEOUT;
	}

	void writeJsonAtomic(const fs::path& path, const nlohmann::json& data)
	{
		const auto temporary = fs::path(path.wstring() + L".tmp");
		{
			boost::nowide::ofstream output(temporary, std::ios::binary | std::ios::trunc);
			if (!output)
				throw std::runtime_error("Cannot create transaction journal");
			output << data.dump(2);
			if (!output)
				throw std::runtime_error("Cannot update transaction journal");
		}

		if (!MoveFileExW(temporary.wstring().c_str(), path.wstring().c_str(),
				MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			boost::system::error_code ignored;
			remove(temporary, ignored);
			throwLastError("MoveFileExW(transaction journal)");
		}
	}

	void reportProgress(const ModPackageProgressCallback& callback, ModPackageProgressPhase phase,
		std::size_t current, std::size_t total, const std::string& item, bool cancellable = true)
	{
		if (!callback)
			return;

		const bool keepGoing = callback(ModPackageProgress { phase, current, total, item, cancellable });
		if (cancellable && !keepGoing)
			throw ModPackageOperationCancelled();
	}

	ProcessResult runSevenZip(const fs::path& executable, const std::wstring& arguments,
		const fs::path& workingDirectory, const fs::path& childMarker,
		const ModPackageProgressCallback& progress, ModPackageProgressPhase phase, std::size_t current,
		std::size_t total, const std::string& item)
	{
		SECURITY_ATTRIBUTES security {};
		security.nLength        = sizeof(security);
		security.bInheritHandle = TRUE;

		HANDLE readRaw  = nullptr;
		HANDLE writeRaw = nullptr;
		if (!CreatePipe(&readRaw, &writeRaw, &security, 0))
			throwLastError("CreatePipe(7-Zip)");

		ScopedHandle readPipe(readRaw);
		ScopedHandle writePipe(writeRaw);
		if (!SetHandleInformation(readPipe.value, HANDLE_FLAG_INHERIT, 0))
			throwLastError("SetHandleInformation(7-Zip)");
		ScopedHandle nullInput(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			&security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
		if (nullInput.value == INVALID_HANDLE_VALUE)
			throwLastError("CreateFileW(NUL)");

		std::wstring         command = quote(executable.wstring()) + L" " + arguments;
		std::vector<wchar_t> mutableCommand(command.begin(), command.end());
		mutableCommand.push_back(L'\0');

		STARTUPINFOW startup {};
		startup.cb          = sizeof(startup);
		startup.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		startup.wShowWindow = SW_HIDE;
		startup.hStdOutput  = writePipe.value;
		startup.hStdError   = writePipe.value;
		startup.hStdInput   = nullInput.value;

		PROCESS_INFORMATION processRaw {};
		if (!CreateProcessW(executable.wstring().c_str(), mutableCommand.data(), nullptr, nullptr, TRUE,
				CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, nullptr, workingDirectory.wstring().c_str(),
				&startup, &processRaw))
			throwLastError("CreateProcessW(7-Zip)");

		ScopedHandle process(processRaw.hProcess);
		ScopedHandle thread(processRaw.hThread);
		writePipe = ScopedHandle();
		try
		{
			writePid(childMarker, processRaw.dwProcessId);
		}
		catch (...)
		{
			TerminateProcess(process.value, ERROR_CANCELLED);
			WaitForSingleObject(process.value, 5000);
			throw;
		}

		ProcessResult          result;
		std::array<char, 8192> buffer {};
		auto                   lastProgress = std::chrono::steady_clock::now() - std::chrono::seconds(1);

		try
		{
			bool finished = false;
			while (!finished)
			{
				DWORD available = 0;
				if (!PeekNamedPipe(readPipe.value, nullptr, 0, nullptr, &available, nullptr))
				{
					if (GetLastError() != ERROR_BROKEN_PIPE)
						throwLastError("PeekNamedPipe(7-Zip)");
					available = 0;
				}

				while (available > 0)
				{
					DWORD      read   = 0;
					const auto amount = static_cast<DWORD>(std::min<std::size_t>(buffer.size(), available));
					if (!ReadFile(readPipe.value, buffer.data(), amount, &read, nullptr))
					{
						if (GetLastError() == ERROR_BROKEN_PIPE)
							break;
						throwLastError("ReadFile(7-Zip)");
					}
					result.output.append(buffer.data(), read);
					available -= std::min(available, read);
					if (read == 0)
						break;
				}

				const auto wait = WaitForSingleObject(process.value, 50);
				if (wait == WAIT_OBJECT_0)
					finished = true;
				else if (wait == WAIT_FAILED)
					throwLastError("WaitForSingleObject(7-Zip)");

				const auto now = std::chrono::steady_clock::now();
				if (now - lastProgress >= std::chrono::milliseconds(100))
				{
					lastProgress = now;
					if (progress && !progress(ModPackageProgress { phase, current, total, item, true }))
					{
						TerminateProcess(process.value, ERROR_CANCELLED);
						WaitForSingleObject(process.value, 5000);
						throw ModPackageOperationCancelled();
					}
				}
			}

			for (;;)
			{
				DWORD available = 0;
				if (!PeekNamedPipe(readPipe.value, nullptr, 0, nullptr, &available, nullptr) ||
					available == 0)
					break;

				DWORD      read   = 0;
				const auto amount = static_cast<DWORD>(std::min<std::size_t>(buffer.size(), available));
				if (!ReadFile(readPipe.value, buffer.data(), amount, &read, nullptr) || read == 0)
					break;
				result.output.append(buffer.data(), read);
			}

			if (!GetExitCodeProcess(process.value, &result.exitCode))
				throwLastError("GetExitCodeProcess(7-Zip)");
		}
		catch (...)
		{
			if (WaitForSingleObject(process.value, 0) == WAIT_TIMEOUT)
			{
				TerminateProcess(process.value, ERROR_CANCELLED);
				WaitForSingleObject(process.value, 5000);
			}
			boost::system::error_code ignored;
			remove(childMarker, ignored);
			throw;
		}

		boost::system::error_code ignored;
		remove(childMarker, ignored);
		return result;
	}

	std::vector<ArchiveEntry> parseListing(const std::string& output)
	{
		std::vector<ArchiveEntry>          result;
		std::istringstream                 stream(output);
		std::string                        line;
		std::map<std::string, std::string> block;

		auto finishBlock = [&] {
			const auto path = block.find("Path");
			if (path != block.end() && !path->second.empty())
			{
				ArchiveEntry entry;
				entry.path      = path->second;
				entry.directory = block["Folder"] == "+";
				if (!entry.directory)
				{
					const auto attributes = block.find("Attributes");
					entry.directory =
						attributes != block.end() && attributes->second.find('D') != std::string::npos;
				}
				entry.encrypted = block["Encrypted"] == "+";
				for (const auto* key : { "Symbolic Link", "Hard Link", "Reparse" })
				{
					const auto found = block.find(key);
					if (found != block.end() && !found->second.empty() && found->second != "-")
						entry.link = true;
				}
				const auto mode = block.find("Mode");
				if (mode != block.end() && !mode->second.empty())
				{
					entry.directory = entry.directory || mode->second.front() == 'd';
					entry.link      = entry.link || mode->second.front() == 'l';
				}
				result.emplace_back(std::move(entry));
			}
			block.clear();
		};

		while (std::getline(stream, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			if (line.empty())
			{
				finishBlock();
				continue;
			}

			const auto separator = line.find(" = ");
			if (separator != std::string::npos)
				block[line.substr(0, separator)] = line.substr(separator + 3);
		}
		finishBlock();
		return result;
	}

	bool supportedPackage(const fs::path& path)
	{
		auto extension = utf8(path.extension());
		boost::algorithm::to_lower(extension);
		return extension == ".exe" || extension == ".rar" || extension == ".zip" || extension == ".7z";
	}

	bool isReservedWindowsName(const std::string& value)
	{
		auto stem = value.substr(0, value.find('.'));
		boost::algorithm::to_upper(stem);
		if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL")
			return true;
		if (stem.size() == 4 && (stem.starts_with("COM") || stem.starts_with("LPT")) && stem[3] >= '1' &&
			stem[3] <= '9')
			return true;
		return false;
	}

	bool validWindowsComponent(const std::string& value)
	{
		if (value.empty() || value == "." || value == ".." || value.back() == ' ' || value.back() == '.')
			return false;

		if (isReservedWindowsName(value))
			return false;

		for (const unsigned char character : value)
			if (character < 32 ||
				std::string_view("<>:\"/\\|?*").find(static_cast<char>(character)) != std::string_view::npos)
				return false;

		try
		{
			if (boost::nowide::widen(value).size() > 255)
				return false;
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	bool splitArchivePath(
		const ArchiveEntry& entry, std::vector<std::string>& parts, ModPackageDiagnosticCode& error)
	{
		auto value = entry.path;
		std::replace(value.begin(), value.end(), '/', '\\');
		while (!value.empty() && value.back() == '\\')
			value.pop_back();

		if (value.empty() || value.front() == '\\' || value.find(':') != std::string::npos)
		{
			error = ModPackageDiagnosticCode::unsafe_path;
			return false;
		}

		boost::split(parts, value, boost::is_any_of("\\"), boost::token_compress_off);
		if (parts.empty())
		{
			error = ModPackageDiagnosticCode::unsafe_path;
			return false;
		}

		for (const auto& part : parts)
		{
			if (part == "." || part == "..")
			{
				error = ModPackageDiagnosticCode::unsafe_path;
				return false;
			}
			if (!validWindowsComponent(part))
			{
				error = ModPackageDiagnosticCode::invalid_windows_name;
				return false;
			}
		}

		if (!boost::iequals(parts.front(), "Mods"))
		{
			error = ModPackageDiagnosticCode::outside_mods;
			return false;
		}

		if (parts.size() == 1)
		{
			if (!entry.directory)
			{
				error = ModPackageDiagnosticCode::direct_mods_file;
				return false;
			}
			return true;
		}

		if (parts.size() == 2 && !entry.directory)
		{
			error = ModPackageDiagnosticCode::direct_mods_file;
			return false;
		}

		return true;
	}

	bool validateEntries(const std::vector<ArchiveEntry>& entries, const fs::path& package,
		std::vector<ModPackageDiagnostic>& diagnostics, std::map<std::string, std::string>& modNames)
	{
		if (entries.empty())
		{
			diagnostics.push_back({ package, ModPackageDiagnosticCode::no_mods, {} });
			return false;
		}

		for (const auto& entry : entries)
		{
			if (entry.encrypted)
			{
				diagnostics.push_back({ package, ModPackageDiagnosticCode::encrypted_content, entry.path });
				return false;
			}
			if (entry.link)
			{
				diagnostics.push_back({ package, ModPackageDiagnosticCode::link_entry, entry.path });
				return false;
			}

			std::vector<std::string> parts;
			ModPackageDiagnosticCode error = ModPackageDiagnosticCode::unsafe_path;
			if (!splitArchivePath(entry, parts, error))
			{
				diagnostics.push_back({ package, error, entry.path });
				return false;
			}

			if (parts.size() >= 2)
			{
				const auto folded = boost::locale::fold_case(parts[1]);
				const auto found  = modNames.find(folded);
				if (found != modNames.end() && found->second != parts[1])
				{
					diagnostics.push_back({ package, ModPackageDiagnosticCode::duplicate_target, parts[1] });
					return false;
				}
				modNames[folded] = parts[1];
			}
		}

		if (modNames.empty())
		{
			diagnostics.push_back({ package, ModPackageDiagnosticCode::no_mods, {} });
			return false;
		}

		return true;
	}

	bool isReparsePoint(const fs::path& path)
	{
		const auto attributes = GetFileAttributesW(path.wstring().c_str());
		return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
	}

	bool validateExtracted(const fs::path& extracted, const fs::path& package,
		std::vector<ModPackageDiagnostic>& diagnostics, std::map<std::string, fs::path>& modDirectories)
	{
		std::vector<ArchiveEntry>        entries;
		boost::system::error_code        ec;
		fs::recursive_directory_iterator it(extracted, ec), end;
		for (; !ec && it != end; it.increment(ec))
		{
			if (isReparsePoint(it->path()))
			{
				diagnostics.push_back({ package, ModPackageDiagnosticCode::link_entry,
					utf8(fs::relative(it->path(), extracted)) });
				return false;
			}

			const auto status = symlink_status(it->path(), ec);
			if (ec || (!is_directory(status) && !is_regular_file(status)))
			{
				diagnostics.push_back({ package, ModPackageDiagnosticCode::invalid_extracted_content,
					utf8(fs::relative(it->path(), extracted)) });
				return false;
			}

			entries.push_back(
				{ utf8(fs::relative(it->path(), extracted)), is_directory(status), false, false });
		}

		if (ec)
		{
			diagnostics.push_back(
				{ package, ModPackageDiagnosticCode::invalid_extracted_content, ec.message() });
			return false;
		}

		std::map<std::string, std::string> listedNames;
		if (!validateEntries(entries, package, diagnostics, listedNames))
			return false;

		fs::path               modsDirectory;
		fs::directory_iterator direct(extracted, ec), directEnd;
		for (; !ec && direct != directEnd; direct.increment(ec))
			if (is_directory(direct->path(), ec) && boost::iequals(utf8(direct->path().filename()), "Mods"))
			{
				modsDirectory = direct->path();
				break;
			}

		if (modsDirectory.empty())
		{
			diagnostics.push_back({ package, ModPackageDiagnosticCode::no_mods, {} });
			return false;
		}

		fs::directory_iterator mods(modsDirectory, ec), modsEnd;
		for (; !ec && mods != modsEnd; mods.increment(ec))
		{
			if (!is_directory(mods->path(), ec) || ec)
				continue;
			const auto name                                = utf8(mods->path().filename());
			modDirectories[boost::locale::fold_case(name)] = mods->path();
		}

		if (ec || modDirectories.empty())
		{
			diagnostics.push_back(
				{ package, ModPackageDiagnosticCode::no_mods, ec ? ec.message() : std::string() });
			return false;
		}

		return true;
	}

	std::string safeDescription(const ModData& data, const II18nService& i18n)
	{
		if (data.description.empty() || data.description.is_absolute())
			return i18n.get("message/status/no_description_available");

		for (const auto& part : data.description)
			if (part == "..")
				return i18n.get("message/status/no_description_available");

		const auto                path = (data.data_path / data.description).lexically_normal();
		boost::system::error_code ec;
		if (!is_regular_file(path, ec) || ec || file_size(path, ec) > 1024 * 1024 || ec)
			return i18n.get("message/status/no_description_available");

		boost::nowide::ifstream input(path, std::ios::binary);
		std::ostringstream      output;
		output << input.rdbuf();
		const auto result = output.str();
		return result.empty() ? i18n.get("message/status/no_description_available") : result;
	}

	std::map<std::string, fs::path> installedDirectories(const fs::path& modsRoot)
	{
		std::map<std::string, fs::path> result;
		boost::system::error_code       ec;
		fs::directory_iterator          it(modsRoot, ec), end;
		for (; !ec && it != end; it.increment(ec))
			if (is_directory(it->path(), ec) && !ec)
			{
				const auto name                        = utf8(it->path().filename());
				result[boost::locale::fold_case(name)] = it->path();
			}
		return result;
	}

	void copyDirectory(const fs::path& source, const fs::path& target,
		const ModPackageProgressCallback& progress, std::size_t current, std::size_t total,
		const std::string& item)
	{
		if (!is_directory(source) || isReparsePoint(source))
			throw std::runtime_error("Package staging source is no longer a regular directory");

		create_directories(target);
		boost::system::error_code        ec;
		fs::recursive_directory_iterator it(source, ec), end;
		for (; !ec && it != end; it.increment(ec))
		{
			reportProgress(progress, ModPackageProgressPhase::staging, current, total, item);
			if (isReparsePoint(it->path()))
				throw std::runtime_error("A link appeared in the staged package contents");

			const auto relative = fs::relative(it->path(), source, ec);
			if (ec)
				break;
			const auto destination = target / relative;
			const auto status      = symlink_status(it->path(), ec);
			if (ec)
				break;
			if (is_directory(status))
				create_directories(destination, ec);
			else if (is_regular_file(status))
				copy_file(it->path(), destination, fs::copy_options::overwrite_existing, ec);
			else
				throw std::runtime_error("Unsupported filesystem entry appeared while staging a mod");
			if (ec)
				break;
		}
		if (ec)
			throw std::runtime_error("Cannot stage mod package contents: " + ec.message());
	}

	bool directChildOf(const fs::path& child, const fs::path& parent)
	{
		return child.parent_path() == parent && child.filename().wstring().starts_with(L"inspection-");
	}

	nlohmann::json readJournal(const fs::path& path)
	{
		return loadJsonFromFile(path, true);
	}

	void restoreTransaction(const fs::path& gameRoot, const fs::path& job, const nlohmann::json& journal)
	{
		const auto transaction = job / "transaction";
		const auto modsRoot    = gameRoot / "Mods";
		const auto items       = journal.find("items");
		if (items == journal.end() || !items->is_array())
			throw std::runtime_error("The interrupted package transaction journal is malformed");

		for (std::size_t reverse = items->size(); reverse > 0; --reverse)
		{
			const auto& item = (*items)[reverse - 1];
			if (!item.is_object() || !item.contains("directory") || !item["directory"].is_string() ||
				!item.contains("had_existing") || !item["had_existing"].is_boolean())
				throw std::runtime_error("The interrupted package transaction journal is malformed");

			const auto directory = item["directory"].get<std::string>();
			if (!validWindowsComponent(directory))
				throw std::runtime_error(
					"The interrupted package transaction contains an unsafe target name");

			const auto target      = modsRoot / pathFromUtf8(directory);
			const auto backup      = transaction / "rollback" / std::to_string(reverse - 1);
			const auto staged      = transaction / "staged" / std::to_string(reverse - 1);
			const bool hadExisting = item["had_existing"].get<bool>();

			boost::system::error_code ec;
			if (hadExisting)
			{
				if (exists(backup, ec) && !ec)
				{
					remove_all(target, ec);
					ec.clear();
					rename(backup, target, ec);
					if (ec)
						throw std::runtime_error("Cannot restore replaced mod directory: " + ec.message());
				}
			}
			else if (!exists(staged, ec) && exists(target, ec) && !ec)
			{
				remove_all(target, ec);
				if (ec)
					throw std::runtime_error("Cannot remove interrupted new mod directory: " + ec.message());
			}
		}

		const auto                listPath     = modsRoot / "list.txt";
		const auto                listSnapshot = transaction / "list.txt.snapshot";
		const bool                listExisted  = journal.value("list_existed", false);
		boost::system::error_code ec;
		if (listExisted)
		{
			if (!is_regular_file(listSnapshot, ec) || ec)
				throw std::runtime_error("The interrupted transaction list snapshot is missing");
			copy_file(listSnapshot, listPath, fs::copy_options::overwrite_existing, ec);
			if (ec)
				throw std::runtime_error("Cannot restore Mods/list.txt: " + ec.message());
		}
		else
		{
			remove(listPath, ec);
		}
	}
}

Era2ModPackageInstaller::Era2ModPackageInstaller(fs::path gameRoot, fs::path programPath,
	std::string preferredLanguage, const II18nService& i18nService, IModManager& modManager,
	std::function<void()> reload)
	: _gameRoot(std::move(gameRoot))
	, _programPath(std::move(programPath))
	, _modsRoot(_gameRoot / "Mods")
	, _jobsRoot(_gameRoot / ".sdmm-import-jobs")
	, _sevenZip(_programPath / "7z" / "7z.exe")
	, _preferredLanguage(std::move(preferredLanguage))
	, _i18nService(i18nService)
	, _modManager(modManager)
	, _reload(std::move(reload))
{
	recoverInterruptedJobs();
}

void Era2ModPackageInstaller::recoverInterruptedJobs()
{
	boost::system::error_code ec;
	bool                      restored = false;
	create_directories(_jobsRoot, ec);
	if (ec)
		throw std::runtime_error("Cannot create package transaction directory: " + ec.message());
	if (const auto attributes = GetFileAttributesW(_jobsRoot.wstring().c_str());
		attributes != INVALID_FILE_ATTRIBUTES)
		SetFileAttributesW(_jobsRoot.wstring().c_str(), attributes | FILE_ATTRIBUTE_HIDDEN);

	fs::directory_iterator it(_jobsRoot, ec), end;
	for (; !ec && it != end; it.increment(ec))
	{
		if (!is_directory(it->path(), ec) || ec)
			continue;

		const auto job = it->path();
		if (processRunning(readPid(job / childFilename)) || processRunning(readPid(job / ownerFilename)))
			continue;

		const auto journalPath = job / "transaction" / journalFilename;
		if (is_regular_file(journalPath, ec) && !ec)
		{
			const auto journal = readJournal(journalPath);
			if (!journal.is_object())
				throw std::runtime_error(
					"Cannot read an interrupted package transaction journal: " + utf8(journalPath));
			if (journal.value("state", std::string()) != "committed")
			{
				restoreTransaction(_gameRoot, job, journal);
				restored = true;
			}
		}

		remove_all(job, ec);
		if (ec)
			throw std::runtime_error("Cannot clean a completed package transaction: " + ec.message());
	}

	if (ec)
		throw std::runtime_error("Cannot inspect package transaction directory: " + ec.message());

	if (restored)
		_reload();
}

ModPackageInspection Era2ModPackageInstaller::inspect(
	const std::vector<fs::path>& packages, const ModPackageProgressCallback& progress)
{
	if (!is_regular_file(_sevenZip))
		throw std::runtime_error("Bundled 7-Zip is missing: " + utf8(_sevenZip));

	ModPackageInspection result;
	result.job_directory = _jobsRoot / fs::unique_path("inspection-%%%%-%%%%-%%%%-%%%%");
	create_directories(result.job_directory / "extracted");
	writePid(result.job_directory / ownerFilename, GetCurrentProcessId());

	try
	{
		const auto                  installed = installedDirectories(_modsRoot);
		const std::set<std::string> empty;

		for (std::size_t index = 0; index < packages.size(); ++index)
		{
			const auto& package = packages[index];
			const auto  item    = utf8(package.filename());
			reportProgress(progress, ModPackageProgressPhase::scanning, index, packages.size(), item);

			if (!supportedPackage(package))
			{
				result.diagnostics.push_back({ package, ModPackageDiagnosticCode::unsupported_type, {} });
				continue;
			}
			if (!is_regular_file(package))
			{
				result.diagnostics.push_back({ package, ModPackageDiagnosticCode::missing_file, {} });
				continue;
			}

			const auto listArguments = L"l -slt -ba -sccUTF-8 -- " + quote(package.wstring());
			const auto listed =
				runSevenZip(_sevenZip, listArguments, _programPath, result.job_directory / childFilename,
					progress, ModPackageProgressPhase::scanning, index, packages.size(), item);
			if (listed.exitCode != 0)
			{
				result.diagnostics.push_back(
					{ package, ModPackageDiagnosticCode::corrupt_archive, listed.output });
				continue;
			}

			const auto                         entries = parseListing(listed.output);
			std::map<std::string, std::string> listedModNames;
			if (!validateEntries(entries, package, result.diagnostics, listedModNames))
				continue;

			const auto extractPath = result.job_directory / "extracted" / std::to_string(index);
			create_directories(extractPath);
			reportProgress(progress, ModPackageProgressPhase::extracting, index, packages.size(), item);
			const auto extractArguments = L"x -y -bd -bso0 -bse1 -bsp0 -sccUTF-8 -o" +
										  quote(extractPath.wstring()) + L" -- " + quote(package.wstring());
			const auto extracted =
				runSevenZip(_sevenZip, extractArguments, _programPath, result.job_directory / childFilename,
					progress, ModPackageProgressPhase::extracting, index, packages.size(), item);
			if (extracted.exitCode != 0)
			{
				result.diagnostics.push_back(
					{ package, ModPackageDiagnosticCode::extraction_failed, extracted.output });
				boost::system::error_code ignored;
				remove_all(extractPath, ignored);
				continue;
			}

			reportProgress(progress, ModPackageProgressPhase::validating, index, packages.size(), item);
			std::map<std::string, fs::path> extractedMods;
			if (!validateExtracted(extractPath, package, result.diagnostics, extractedMods))
			{
				boost::system::error_code ignored;
				remove_all(extractPath, ignored);
				continue;
			}

			for (const auto& [id, directory] : extractedMods)
			{
				PackagedMod packaged;
				packaged.package_path        = package;
				packaged.extracted_directory = directory;
				packaged.directory           = utf8(directory.filename());
				packaged.metadata            = Era2ModDataLoader::load(
					id, directory, _preferredLanguage, empty, empty, empty, _i18nService);
				packaged.description = safeDescription(packaged.metadata, _i18nService);

				boost::system::error_code targetError;
				const auto                targetPath = _modsRoot / directory.filename();
				if (exists(targetPath, targetError) && !targetError && !is_directory(targetPath, targetError))
				{
					packaged.selectable = false;
					packaged.selected   = false;
					result.diagnostics.push_back(
						{ package, ModPackageDiagnosticCode::target_collision, packaged.directory });
				}

				if (const auto existing = installed.find(id); existing != installed.end())
				{
					packaged.installed           = true;
					packaged.installed_directory = existing->second;
					packaged.installed_version   = Era2ModDataLoader::load(
						id, existing->second, _preferredLanguage, empty, empty, empty, _i18nService)
													 .version;
				}

				result.mods.emplace_back(std::move(packaged));
			}
		}

		std::map<std::string, std::vector<std::size_t>> targets;
		for (std::size_t index = 0; index < result.mods.size(); ++index)
			targets[boost::locale::fold_case(result.mods[index].directory)].push_back(index);

		for (const auto& [target, indices] : targets)
		{
			if (indices.size() < 2)
				continue;
			for (const auto index : indices)
			{
				result.mods[index].selectable = false;
				result.mods[index].selected   = false;
				result.diagnostics.push_back({ result.mods[index].package_path,
					ModPackageDiagnosticCode::duplicate_target, result.mods[index].directory });
			}
		}

		reportProgress(progress, ModPackageProgressPhase::validating, packages.size(), packages.size(), {});
		return result;
	}
	catch (...)
	{
		boost::system::error_code ignored;
		remove_all(result.job_directory, ignored);
		throw;
	}
}

ModPackageInstallResult Era2ModPackageInstaller::install(const ModPackageInspection& inspection,
	const std::vector<std::size_t>& selectedMods, const ModPackageProgressCallback& progress)
{
	if (!directChildOf(inspection.job_directory, _jobsRoot) || !is_directory(inspection.job_directory))
		throw std::runtime_error("Package inspection data is unavailable");
	if (selectedMods.empty())
		throw std::runtime_error("No mods were selected for installation");

	std::set<std::size_t> uniqueIndices;
	for (const auto index : selectedMods)
	{
		if (index >= inspection.mods.size() || !inspection.mods[index].selectable ||
			!uniqueIndices.emplace(index).second)
			throw std::runtime_error("The package installation selection is invalid");
	}

	const auto transaction  = inspection.job_directory / "transaction";
	const auto stagedRoot   = transaction / "staged";
	const auto rollbackRoot = transaction / "rollback";
	create_directories(stagedRoot);
	create_directories(rollbackRoot);

	nlohmann::json journal  = nlohmann::json::object();
	journal["version"]      = 1;
	journal["state"]        = "preparing";
	journal["list_existed"] = is_regular_file(_modsRoot / "list.txt");
	journal["items"]        = nlohmann::json::array();

	std::vector<std::string> targetNames;
	std::vector<bool>        hadExisting;
	try
	{
		std::size_t ordinal = 0;
		for (const auto index : selectedMods)
		{
			const auto& packaged = inspection.mods[index];
			const auto  targetName =
				packaged.installed ? utf8(packaged.installed_directory.filename()) : packaged.directory;
			if (!validWindowsComponent(targetName))
				throw std::runtime_error("A selected mod has an invalid target directory name");

			const auto target = _modsRoot / pathFromUtf8(targetName);
			if (exists(target) && !is_directory(target))
				throw std::runtime_error("A file already occupies the mod target path: " + targetName);

			copyDirectory(packaged.extracted_directory, stagedRoot / std::to_string(ordinal), progress,
				ordinal, selectedMods.size(), targetName);

			targetNames.emplace_back(targetName);
			hadExisting.emplace_back(exists(target));
			journal["items"].push_back(
				nlohmann::json { { "directory", targetName }, { "had_existing", hadExisting.back() } });
			++ordinal;
		}

		const auto listPath = _modsRoot / "list.txt";
		if (is_regular_file(listPath))
			copy_file(listPath, transaction / "list.txt.snapshot", fs::copy_options::overwrite_existing);

		journal["state"] = "prepared";
		writeJsonAtomic(transaction / journalFilename, journal);
	}
	catch (...)
	{
		boost::system::error_code ignored;
		remove_all(transaction, ignored);
		throw;
	}

	try
	{
		std::string appendAfter;
		if (!_modManager.mods().data.empty())
			appendAfter = _modManager.mods().data.back().id;

		reportProgress(progress, ModPackageProgressPhase::committing, 0, selectedMods.size(), {}, false);
		for (std::size_t ordinal = 0; ordinal < selectedMods.size(); ++ordinal)
		{
			const auto target = _modsRoot / pathFromUtf8(targetNames[ordinal]);
			const auto backup = rollbackRoot / std::to_string(ordinal);
			const auto staged = stagedRoot / std::to_string(ordinal);

			if (hadExisting[ordinal])
				rename(target, backup);
			rename(staged, target);
		}

		_reload();
		for (std::size_t ordinal = 0; ordinal < selectedMods.size(); ++ordinal)
		{
			if (hadExisting[ordinal])
				continue;

			const auto id = boost::locale::fold_case(targetNames[ordinal]);
			_modManager.enable(id);
			if (!appendAfter.empty() && appendAfter != id)
				_modManager.move(id, appendAfter);
			appendAfter = id;
		}

		journal["state"] = "committed";
		writeJsonAtomic(transaction / journalFilename, journal);

		ModPackageInstallResult result;
		for (const auto& target : targetNames)
			result.installed_mod_ids.emplace_back(boost::locale::fold_case(target));

		boost::system::error_code ignored;
		remove_all(inspection.job_directory, ignored);
		return result;
	}
	catch (...)
	{
		const auto original = std::current_exception();
		try
		{
			reportProgress(
				progress, ModPackageProgressPhase::rolling_back, 0, selectedMods.size(), {}, false);
			restoreTransaction(_gameRoot, inspection.job_directory, journal);
			_reload();
			boost::system::error_code ignored;
			remove_all(inspection.job_directory, ignored);
		}
		catch (...)
		{
			throw std::runtime_error(
				"Package installation failed and automatic rollback could not be completed. "
				"The transaction journal was retained at: " +
				utf8(transaction / journalFilename));
		}
		std::rethrow_exception(original);
	}
}

void Era2ModPackageInstaller::discard(const ModPackageInspection& inspection) noexcept
{
	if (!directChildOf(inspection.job_directory, _jobsRoot))
		return;

	boost::system::error_code ignored;
	remove_all(inspection.job_directory, ignored);
}
