﻿/*
 * Copyright (C) 2020-2021 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "audio-utils/filter-utils/FilterCascadeParser.h"
#include "sound-processing/Channel.h"
#include "sound-processing/sound-handlers/ExternalMethods.h"

namespace rxtd::audio_analyzer {
	struct ProcessingData {
		using Finisher = handler::ExternalMethods::FinishMethodType;

		struct FilterInfo {
			string raw;
			audio_utils::FilterCascadeCreator creator;

			friend bool operator==(const FilterInfo& lhs, const FilterInfo& rhs) {
				return lhs.raw == rhs.raw;
			}

			friend bool operator!=(const FilterInfo& lhs, const FilterInfo& rhs) { return !(lhs == rhs); }
		};

		FilterInfo filter;
		index targetRate{};
		std::vector<Channel> channels;
		std::vector<istring> handlerOrder;
		std::map<istring, HandlerInfo, std::less<>> handlers;

		// autogenerated
		friend bool operator==(const ProcessingData& lhs, const ProcessingData& rhs) {
			return lhs.filter == rhs.filter
				&& lhs.targetRate == rhs.targetRate
				&& lhs.channels == rhs.channels
				&& lhs.handlerOrder == rhs.handlerOrder
				&& lhs.handlers == rhs.handlers;
		}

		friend bool operator!=(const ProcessingData& lhs, const ProcessingData& rhs) { return !(lhs == rhs); }
	};
}
