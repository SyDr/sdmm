// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "service/ii18n_service.hpp"

#include <unordered_map>
#include <wx/string.h>
#include <nlohmann/json.hpp>

namespace mm
{
	class IAppConfig;

	struct I18nService : II18nService
	{
		explicit I18nService(IAppConfig const& config);

		wxString category(wxString const& category) const override;
		wxString get(wxString const& key) const override;
		wxString languageName(wxString const& code) const override;

	private:
		void build_cache(nlohmann::json const& data, wxString const& prefix);

	private:
		const bool _assertAboutMissingEntries = false;

		std::map<wxString, wxString> _data;
	};
}
