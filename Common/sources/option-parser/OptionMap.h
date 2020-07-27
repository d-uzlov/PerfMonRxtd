﻿/*
 * Copyright (C) 2019 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "StringUtils.h"
#include "AbstractOption.h"
#include "Option.h"

namespace rxtd::utils {
	class OptionMap : public AbstractOption<OptionMap> {
	public:
		struct MapOptionInfo {
			SubstringViewInfo substringInfo { };
			bool touched = false;

			MapOptionInfo() = default;
			MapOptionInfo(SubstringViewInfo substringInfo) : substringInfo(substringInfo) { }
		};

	private:
		// For move and copy operations. 
		// String view would require much more hustle when moved with source than SubstringViewInfo
		std::map<SubstringViewInfo, SubstringViewInfo> paramsInfo { };

		// For fast search.
		mutable std::map<isview, MapOptionInfo> params { };

	public:
		OptionMap() = default;
		OptionMap(sview view, std::vector<wchar_t> &&source, std::map<SubstringViewInfo, SubstringViewInfo>&& paramsInfo);

		// Returns named option, search is case-insensitive.
		// Doesn't raise the "touched" flag on the option
		Option getUntouched(sview name) const;

		GhostOption get(sview name) const & {
			return get(name % ciView());
		}

		GhostOption get(isview name) const &;

		GhostOption get(const wchar_t* name) const & {
			return get(isview { name });
		}

		Option get(sview name) const && {
			return get(name);
		}
		
		Option get(isview name) const && {
			return get(name);
		}
		
		Option get(const wchar_t* name) const && {
			return get(name);
		}

		// returns true if option with such name exists.
		bool has(sview name) const {
			return has(name % ciView());
		}
		// returns true if option with such name exists.
		bool has(isview name) const;
		// returns true if option with such name exists.
		bool has(const wchar_t* name) const {
			return has(isview { name });
		}

		std::vector<isview> getListOfUntouched() const;
	private:
		void fillParams() const;
		MapOptionInfo* find(isview name) const;
	};
}
