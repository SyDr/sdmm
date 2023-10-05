// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/ii18n_service.hpp"

#include <unordered_map>
#include <wx/string.h>
#include <nlohmann/json.hpp>

namespace mm
{
	struct IAppConfig;

	struct I18nService : II18nService
	{
		explicit I18nService(IAppConfig const& config);

		wxString category(const wxString& category) const override;
		wxString get(const wxString& key) const override;
		wxString languageName(const wxString& code) const override;

	private:
		void build_cache(nlohmann::json const& data, const wxString& prefix);

	private:
		std::map<wxString, wxString> _data;
	};
}
