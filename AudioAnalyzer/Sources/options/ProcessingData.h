﻿/*
 * Copyright (C) 2020-2021 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "HandlerPatchersInfo.h"
#include "audio-utils/filter-utils/FilterCascadeParser.h"
#include "sound-processing/Channel.h"
#include "sound-processing/sound-handlers/SoundHandlerBase.h"

namespace rxtd::audio_analyzer {
	struct ProcessingData {
		using Finisher = SoundHandlerBase::ExternalMethods::FinishMethodType;

		string rawFccDescription;
		audio_utils::FilterCascadeCreator fcc;

		index targetRate{};

		std::set<Channel> channels;
		HandlerPatchersInfo handlersInfo;

		// handlerName → finisher
		std::map<istring, Finisher, std::less<>> finishers;

		// autogenerated
		friend bool operator==(const ProcessingData& lhs, const ProcessingData& rhs) {
			return lhs.rawFccDescription == rhs.rawFccDescription
				&& lhs.fcc == rhs.fcc
				&& lhs.targetRate == rhs.targetRate
				&& lhs.channels == rhs.channels
				&& lhs.handlersInfo == rhs.handlersInfo
				&& lhs.finishers == rhs.finishers;
		}

		friend bool operator!=(const ProcessingData& lhs, const ProcessingData& rhs) {
			return !(lhs == rhs);
		}
	};
}
