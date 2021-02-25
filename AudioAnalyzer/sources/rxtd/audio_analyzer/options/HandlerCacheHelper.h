/*
 * Copyright (C) 2020-2021 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "HandlerInfo.h"
#include "rxtd/rainmeter/Logger.h"
#include "rxtd/rainmeter/Rainmeter.h"
#include "rxtd/audio_analyzer/sound_processing/sound_handlers/HandlerBase.h"

namespace rxtd::audio_analyzer::options {
	class HandlerCacheHelper {
		using Logger = rainmeter::Logger;
		using Rainmeter = rainmeter::Rainmeter;
		using OptionMap = option_parsing::OptionMap;

		struct MapValue {
			bool updated = false;
			HandlerInfo info;
		};

		using PatchersMap = std::map<istring, MapValue, std::less<>>;
		PatchersMap patchersCache;
		Rainmeter rain;
		bool anythingChanged = false;
		bool unusedOptionsWarning = false;
		Version version{};

	public:
		void setRain(Rainmeter value) {
			rain = std::move(value);
		}

		void setUnusedOptionsWarning(bool value) {
			unusedOptionsWarning = value;
		}

		void setVersion(Version value) {
			version = value;
		}

		void reset() {
			for (auto& [name, info] : patchersCache) {
				info.updated = false;
			}

			anythingChanged = false;
		}

		bool isAnythingChanged() const {
			return anythingChanged;
		}

		[[nodiscard]]
		HandlerInfo* getHandlerInfo(const istring& name, Logger& cl);

	private:
		[[nodiscard]]
		MapValue parseHandler(sview name, MapValue val, Logger& cl);

		[[nodiscard]]
		handler::HandlerBase::HandlerMetaInfo createHandlerPatcher(const OptionMap& optionMap, Logger& cl) const;

		void readRawDescription2(isview type, const OptionMap& optionMap, string& rawDescription2) const;
	};
}