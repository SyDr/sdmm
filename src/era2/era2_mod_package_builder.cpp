// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_mod_package_builder.hpp"

#include "domain/mod_data.hpp"

#include <windows.h>

#include <array>
#include <chrono>
#include <cstring>
#include <ctime>
#include <limits>
#include <system_error>

using namespace mm;

namespace
{
	constexpr auto markerFilename = "pid.txt";

	void throwLastError(const char* operation)
	{
		throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
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

	std::vector<unsigned char> readBinaryFile(const fs::path& path)
	{
		boost::nowide::ifstream input(path, std::ios::binary | std::ios::ate);
		if (!input)
			return {};

		const auto size = input.tellg();
		if (size <= 0)
			return {};

		std::vector<unsigned char> result(static_cast<std::size_t>(size));
		input.seekg(0);
		input.read(reinterpret_cast<char*>(result.data()), static_cast<std::streamsize>(size));
		if (!input)
			return {};

		return result;
	}

#pragma pack(push, 2)
	struct IconDirectory
	{
		WORD reserved;
		WORD type;
		WORD count;
	};

	struct IconDirectoryEntry
	{
		BYTE  width;
		BYTE  height;
		BYTE  colorCount;
		BYTE  reserved;
		WORD  planes;
		WORD  bitsPerPixel;
		DWORD bytesInResource;
		DWORD imageOffset;
	};

	struct GroupIconEntry
	{
		BYTE  width;
		BYTE  height;
		BYTE  colorCount;
		BYTE  reserved;
		WORD  planes;
		WORD  bitsPerPixel;
		DWORD bytesInResource;
		WORD  resourceId;
	};
#pragma pack(pop)

	bool updateIconResource(HANDLE update, const fs::path& iconPath)
	{
		const auto bytes = readBinaryFile(iconPath);
		if (bytes.size() < sizeof(IconDirectory))
			return false;

		IconDirectory header {};
		std::memcpy(&header, bytes.data(), sizeof(header));
		if (header.reserved != 0 || header.type != 1 || header.count == 0 || header.count > 256)
			return false;

		const auto tableSize = sizeof(IconDirectory) + header.count * sizeof(IconDirectoryEntry);
		if (bytes.size() < tableSize)
			return false;

		std::vector<IconDirectoryEntry> entries(header.count);
		std::memcpy(entries.data(), bytes.data() + sizeof(IconDirectory),
			entries.size() * sizeof(IconDirectoryEntry));

		for (const auto& entry : entries)
		{
			const auto end = static_cast<std::uint64_t>(entry.imageOffset) + entry.bytesInResource;
			if (entry.bytesInResource == 0 || end > bytes.size())
				return false;
		}

		std::vector<unsigned char> group(sizeof(IconDirectory) + entries.size() * sizeof(GroupIconEntry));
		std::memcpy(group.data(), &header, sizeof(header));

		for (std::size_t i = 0; i < entries.size(); ++i)
		{
			const auto&    source = entries[i];
			GroupIconEntry target { source.width, source.height, source.colorCount, source.reserved,
				source.planes, source.bitsPerPixel, source.bytesInResource, static_cast<WORD>(i + 1) };
			std::memcpy(
				group.data() + sizeof(IconDirectory) + i * sizeof(GroupIconEntry), &target, sizeof(target));

			if (!UpdateResourceW(update, RT_ICON, MAKEINTRESOURCEW(static_cast<WORD>(i + 1)),
					MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
					const_cast<unsigned char*>(bytes.data() + source.imageOffset), source.bytesInResource))
				throwLastError("UpdateResourceW(RT_ICON)");
		}

		if (!UpdateResourceW(update, RT_GROUP_ICON, MAKEINTRESOURCEW(1),
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), group.data(), static_cast<DWORD>(group.size())))
			throwLastError("UpdateResourceW(RT_GROUP_ICON)");

		return true;
	}

	void updateModArgument(HANDLE update, const std::wstring& directoryName)
	{
		const auto argument = L"\"" + directoryName + L"\"";
		if (argument.size() > std::numeric_limits<WORD>::max())
			throw std::length_error("Mod directory name is too long for the SFX resource");

		std::vector<WORD> stringBlock;
		stringBlock.reserve(argument.size() + 16);
		stringBlock.push_back(static_cast<WORD>(argument.size()));
		stringBlock.insert(stringBlock.end(), argument.begin(), argument.end());
		stringBlock.insert(stringBlock.end(), 15, 0);

		if (!UpdateResourceW(update, RT_STRING, MAKEINTRESOURCEW(251),
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), stringBlock.data(),
				static_cast<DWORD>(stringBlock.size() * sizeof(WORD))))
			throwLastError("UpdateResourceW(RT_STRING)");
	}

	void patchSfx(const fs::path& sfxPath, const ModData& mod)
	{
		HANDLE update = BeginUpdateResourceW(sfxPath.wstring().c_str(), FALSE);
		if (!update)
			throwLastError("BeginUpdateResourceW");

		try
		{
			updateModArgument(update, mod.data_path.filename().wstring());

			if (!mod.icon.empty())
				updateIconResource(update, (mod.data_path / mod.icon).lexically_normal());
		}
		catch (...)
		{
			EndUpdateResourceW(update, TRUE);
			throw;
		}

		if (!EndUpdateResourceW(update, FALSE))
			throwLastError("EndUpdateResourceW");
	}

	std::uint32_t readPid(const fs::path& marker)
	{
		boost::nowide::ifstream input(marker);
		std::uint32_t           pid = 0;
		input >> pid;
		return pid;
	}

	void writePid(const fs::path& marker, std::uint32_t pid)
	{
		boost::nowide::ofstream output(marker, std::ios::trunc);
		if (!output)
			throw std::runtime_error("Cannot create package job marker");

		output << pid << '\n';
		if (!output)
			throw std::runtime_error("Cannot update package job marker");
	}
}

Era2ModPackageBuilder::Era2ModPackageBuilder(fs::path gameRoot, fs::path programPath)
	: _gameRoot(std::move(gameRoot))
	, _programPath(std::move(programPath))
	, _jobsRoot(fs::temp_directory_path() / "SD Mod Manager" / "package-jobs")
{
	cleanStaleJobs();
}

void Era2ModPackageBuilder::cleanStaleJobs() const
{
	boost::system::error_code ec;
	create_directories(_jobsRoot, ec);
	if (ec)
		return;

	fs::directory_iterator       it(_jobsRoot, ec);
	const fs::directory_iterator end;
	for (; !ec && it != end; it.increment(ec))
	{
		boost::system::error_code statusError;
		if (!is_directory(it->path(), statusError) || statusError)
			continue;

		const auto pid = readPid(it->path() / markerFilename);
		if (pid == 0)
		{
			boost::system::error_code timeError;
			const auto                lastWrite = last_write_time(it->path(), timeError);
			constexpr auto            stagingLifetime =
				std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(1)).count();
			if (!timeError && std::time(nullptr) - lastWrite < stagingLifetime)
				continue;
		}

		bool running = false;
		if (pid != 0)
		{
			if (HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid))
			{
				running = WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
				CloseHandle(process);
			}
			else if (GetLastError() == ERROR_ACCESS_DENIED)
			{
				running = true;
			}
		}

		if (!running)
			remove_all(it->path(), statusError);
	}
}

ModPackageJob Era2ModPackageBuilder::start(const ModData& mod, const fs::path& outputPath)
{
	if (mod.virtual_mod || !is_directory(mod.data_path))
		throw std::runtime_error("The selected mod directory does not exist");

	const auto source7z = _programPath / "7z";
	const auto jobPath  = _jobsRoot / fs::unique_path("job-%%%%-%%%%-%%%%-%%%%");
	const auto job7z    = jobPath / "7z";

	boost::system::error_code ec;
	create_directories(job7z, ec);
	if (ec)
		throw std::runtime_error("Cannot create temporary package job directory: " + ec.message());

	try
	{
		for (const auto* filename : { "7z.exe", "7z.dll", "7z.sfx" })
		{
			const auto source = source7z / filename;
			if (!is_regular_file(source))
				throw std::runtime_error("Required package tool is missing: " + source.string());

			copy_file(source, job7z / filename, fs::copy_options::overwrite_existing);
		}

		patchSfx(job7z / "7z.sfx", mod);

		if (!outputPath.parent_path().empty())
			create_directories(outputPath.parent_path());

		if (exists(outputPath))
			remove(outputPath);

		writePid(jobPath / markerFilename, 0);

		const auto   executable  = job7z / "7z.exe";
		const auto   archiveItem = fs::path("Mods") / mod.data_path.filename();
		std::wstring command     = quote(executable.wstring()) + L" a " + quote(outputPath.wstring()) + L" " +
							   quote(archiveItem.wstring()) + L" -sfx7z.sfx";
		std::vector<wchar_t> mutableCommand(command.begin(), command.end());
		mutableCommand.push_back(L'\0');

		STARTUPINFOW startup {};
		startup.cb          = sizeof(startup);
		startup.dwFlags     = STARTF_USESHOWWINDOW;
		startup.wShowWindow = SW_SHOWMINIMIZED;

		PROCESS_INFORMATION process {};
		if (!CreateProcessW(executable.wstring().c_str(), mutableCommand.data(), nullptr, nullptr, FALSE,
				CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP | CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED,
				nullptr, _gameRoot.wstring().c_str(), &startup, &process))
			throwLastError("CreateProcessW(7z.exe)");

		try
		{
			writePid(jobPath / markerFilename, process.dwProcessId);
			if (ResumeThread(process.hThread) == static_cast<DWORD>(-1))
				throwLastError("ResumeThread(7z.exe)");
		}
		catch (...)
		{
			TerminateProcess(process.hProcess, ERROR_CANCELLED);
			CloseHandle(process.hThread);
			CloseHandle(process.hProcess);
			throw;
		}

		CloseHandle(process.hThread);

		_monitors.emplace_back([processHandle = process.hProcess, jobPath](std::stop_token stop) {
			while (!stop.stop_requested())
			{
				const auto result = WaitForSingleObject(processHandle, 250);
				if (result == WAIT_OBJECT_0)
				{
					CloseHandle(processHandle);
					boost::system::error_code removeError;
					remove_all(jobPath, removeError);
					return;
				}

				if (result == WAIT_FAILED)
					break;
			}

			CloseHandle(processHandle);
		});

		return { process.dwProcessId, jobPath, outputPath };
	}
	catch (...)
	{
		boost::system::error_code removeError;
		remove_all(jobPath, removeError);
		throw;
	}
}
