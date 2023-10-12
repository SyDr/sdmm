// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "shell_util.h"

#include <unordered_set>
#include <wx/app.h>
#include <wx/window.h>
#include <wtypes.h>

namespace
{
	std::unique_ptr<WCHAR[]> createBufferString(const std::unordered_set<wxString>& items)
	{
		const size_t size = std::accumulate(items.begin(), items.end(), 1, [](size_t current, const wxString& item) {
			return current + item.size() + 1;
		});

		auto result = std::make_unique<WCHAR[]>(size);

		int to = 0;

		for (const auto& item : items)
		{
			for (size_t j = 0; j < item.size(); ++to, ++j)
				*(result.get() + to) = item[j];

			*(result.get() + to) = '\0';
			++to;
		}
		*(result.get() + to) = '\0';

		return result;
	}
}

bool mm::shellRemove(const wxString& path)
{
	SHFILEOPSTRUCT sfo = {0};

	auto toRemove = createBufferString({ path });

	sfo.hwnd = wxTheApp->GetTopWindow()->GetHWND();
	sfo.wFunc = FO_DELETE;
	sfo.pFrom = toRemove.get();
	sfo.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_WANTNUKEWARNING;

	wxTheApp->GetTopWindow()->Disable(); // TODO: don't lock ui, if not needed
	int res = SHFileOperation(&sfo);
	wxTheApp->GetTopWindow()->Enable();

	return res == 0 && !sfo.fAnyOperationsAborted;
}
