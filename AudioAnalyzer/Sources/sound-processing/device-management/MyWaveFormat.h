/*
 * Copyright (C) 2019 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "../Channel.h"
#include "windows-wrappers/WaveFormat.h"

namespace rxtd::audio_analyzer {
	struct MyWaveFormat {
		index samplesPerSec = 0;
		index channelsCount = 0;
		utils::WaveDataFormat format = utils::WaveDataFormat::eINVALID;
		ChannelLayout channelLayout { };
	};

}
