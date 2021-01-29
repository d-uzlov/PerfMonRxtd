﻿/*
 * Copyright (C) 2019-2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include <set>
#include <functional>

#include "HandlerCacheHelper.h"
#include "rainmeter/Logger.h"
#include "rainmeter/Rainmeter.h"
#include "sound-processing/Channel.h"
#include "sound-processing/sound-handlers/SoundHandler.h"
#include "audio-utils/filter-utils/FilterCascadeParser.h"

namespace rxtd::audio_analyzer {
	class ParamParser {
		using Logger = ::rxtd::common::rainmeter::Logger;
		using Rainmeter = ::rxtd::common::rainmeter::Rainmeter;

	public:
		struct HandlerPatchersInfo {
			std::vector<istring> order;
			std::map<istring, PatchInfo, std::less<>> patchers;

			// autogenerated
			friend bool operator==(const HandlerPatchersInfo& lhs, const HandlerPatchersInfo& rhs) {
				return lhs.order == rhs.order
					&& lhs.patchers == rhs.patchers;
			}

			friend bool operator!=(const HandlerPatchersInfo& lhs, const HandlerPatchersInfo& rhs) {
				return !(lhs == rhs);
			}
		};

		struct ProcessingData {
			string rawFccDescription;
			audio_utils::FilterCascadeCreator fcc;

			index targetRate{};

			std::set<Channel> channels;
			HandlerPatchersInfo handlersInfo;

			// handlerName → finisher
			std::map<istring, SoundHandler::ExternalMethods::FinishMethodType, std::less<>> finishers;

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

		using ProcessingsInfoMap = std::map<istring, ProcessingData, std::less<>>;

	private:
		Rainmeter rain;

		bool unusedOptionsWarning = true;
		index defaultTargetRate = 44100;
		ProcessingsInfoMap parseResult;
		index legacyNumber = 0;
		HandlerCacheHelper hch;

		mutable bool anythingChanged = false;

	public:
		void setRainmeter(Rainmeter value) {
			rain = std::move(value);
			hch.setRain(rain);
		}

		// return true if there were any changes since last update, false if there were none
		bool parse(index legacyNumber, bool suppressLogger);

		[[nodiscard]]
		const auto& getParseResult() const {
			return parseResult;
		}

		[[nodiscard]]
		index getLegacyNumber() const {
			return legacyNumber;
		}

	private:
		void parseFilters(const utils::OptionMap& optionMap, ProcessingData& data, Logger& cl) const;
		void parseTargetRate(const utils::OptionMap& optionMap, ProcessingData& data, Logger& cl) const;

		[[nodiscard]]
		static bool checkListUnique(const utils::OptionList& list);

		void parseProcessing(sview name, Logger cl, ProcessingData& oldData);

		[[nodiscard]]
		std::set<Channel> parseChannels(const utils::OptionList& channelsStringList, Logger& logger) const;

		[[nodiscard]]
		HandlerPatchersInfo parseHandlers(const utils::OptionList& indices);
	};
}
