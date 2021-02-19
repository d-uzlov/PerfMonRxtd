/*
 * Copyright (C) 2020-2021 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "sound-processing/Channel.h"

namespace rxtd::audio_analyzer::wasapi_wrappers {
	struct WaveFormat {
		index samplesPerSec = 0;
		index channelsCount = 0;
		ChannelLayout channelLayout{};

		// autogenerated
		friend bool operator==(const WaveFormat& lhs, const WaveFormat& rhs) {
			return lhs.samplesPerSec == rhs.samplesPerSec
				&& lhs.channelsCount == rhs.channelsCount
				&& lhs.channelLayout == rhs.channelLayout;
		}

		friend bool operator!=(const WaveFormat& lhs, const WaveFormat& rhs) {
			return !(lhs == rhs);
		}
	};

	class FormatException : public std::runtime_error {
	public:
		explicit FormatException() : runtime_error("audio format can't be parsed") {}
	};
}
