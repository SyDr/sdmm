// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "sfx_util.hpp"

#include "fs_util.h"
#include "utility/sdlexcept.h"

/// https://devblogs.microsoft.com/oldnewthing/20120720-00/?p=7083
/// Raymond Chen
/// The format of icon resources

using namespace mm;

namespace
{
#pragma pack(push, 2)
	struct ICONDIR
	{
		WORD idReserved = 0;
		WORD idType     = 0;
		WORD idCount    = 0;

		// idEntries -> idCount * 16
	};

	struct ICONDIRENTRY
	{
		BYTE  bWidth        = 0;
		BYTE  bHeight       = 0;
		BYTE  bColorCount   = 0;
		BYTE  bReserved     = 0;
		WORD  wPlanes       = 0;
		WORD  wBitCount     = 0;
		DWORD dwBytesInRes  = 0;
		DWORD dwImageOffset = 0;
	};

	struct RESDIR
	{
		/// @note don't care for cursor
		BYTE  Width        = 0;
		BYTE  Height       = 0;
		BYTE  ColorCount   = 0;
		BYTE  reserved     = 0;
		WORD  Planes       = 0;
		WORD  BitCount     = 0;
		DWORD BytesInRes   = 0;
		WORD  IconCursorId = 0;
	};
#pragma pack(pop)

	bool updateIconImpl(HANDLE hUpdate, const fs::path& icon)
	{
		std::vector<unsigned char> iconData = readBinary(icon);

		if (iconData.size() < sizeof(ICONDIR))
			return false;

		ICONDIR header;
		std::memcpy(&header, iconData.data(), sizeof(header));

		if (header.idCount < 1)
			return false;

		if (iconData.size() < sizeof(ICONDIR) + header.idCount * sizeof(ICONDIRENTRY))
			return false;

		std::vector<unsigned char> group(sizeof(ICONDIR) + header.idCount * sizeof(RESDIR));
		std::memcpy(group.data(), &header, sizeof(header));

		for (WORD i = 0; i < header.idCount; ++i)
		{
			ICONDIRENTRY entry;
			std::memcpy(&entry, iconData.data() + sizeof(ICONDIR) + i * sizeof(ICONDIRENTRY), sizeof(entry));

			if (entry.dwImageOffset < sizeof(ICONDIR) || entry.dwImageOffset >= iconData.size() ||
				entry.dwBytesInRes == 0 || entry.dwImageOffset + entry.dwBytesInRes > iconData.size())
			{
				return false;
			}

			/// @note
			/// sfx exe is already prepared:
			/// there is only one group of icons
			/// and it contains one icon
			/// and it's id is 1

			if (!UpdateResource(hUpdate, RT_ICON, MAKEINTRESOURCE(i + 1u),
					MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), iconData.data() + entry.dwImageOffset,
					entry.dwBytesInRes))
			{
				/// @todo GetLastError();
				return false;
			}

			RESDIR groupHeader = { .Width = entry.bWidth,
				.Height                   = entry.bHeight,
				.ColorCount               = entry.bColorCount,
				.reserved                 = 0,
				.Planes                   = entry.wPlanes,
				.BitCount                 = entry.wBitCount,
				.BytesInRes               = entry.dwBytesInRes,
				.IconCursorId             = i + 1u };

			std::memcpy(
				group.data() + sizeof(ICONDIR) + i * sizeof(RESDIR), &groupHeader, sizeof(groupHeader));
		}

		if (!UpdateResource(hUpdate, RT_GROUP_ICON, MAKEINTRESOURCE(1),
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), group.data(), group.size()))
		{
			/// @todo GetLastError();
			return false;
		}

		return true;
	}
}

bool sfx::updateData(const fs::path& path, const fs::path& icon)
{
	HANDLE hUpdate = BeginUpdateResource(path.wstring().c_str(), FALSE);

	if (!hUpdate)
		return false;

	if (!updateIconImpl(hUpdate, icon))
		return false;

	if (!EndUpdateResource(hUpdate, FALSE))
		return false;

	return true;
}
