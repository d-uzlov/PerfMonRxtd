/*
 * Copyright (C) 2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include "ProcessingOrchestrator.h"

#include "MapUtils.h"

using namespace audio_analyzer;

void ProcessingOrchestrator::exchangeData(DataSnapshot& snapshot) {
	std::swap(snapshot, dataSnapshot);
}

void ProcessingOrchestrator::patch(const ParamParser::ProcessingsInfoMap& patches, index legacyNumber) {
	utils::MapUtils::intersectKeyCollection(saMap, patches);
	utils::MapUtils::intersectKeyCollection(dataSnapshot, patches);

	for (const auto&[name, data] : patches) {
		auto& sa = saMap[name];
		sa.setLogger(logger);
		sa.setParams(data, legacyNumber, currentFormat.samplesPerSec, currentFormat.channelLayout);
	}
}

void ProcessingOrchestrator::setFormat(index samplesPerSec, ChannelLayout channelLayout) {
	for (auto& [name, sa] : saMap) {
		sa.updateFormat(samplesPerSec, channelLayout);
		sa.configureSnapshot(dataSnapshot[name]);
	}
	currentFormat.samplesPerSec = samplesPerSec;
	currentFormat.channelLayout = std::move(channelLayout);
}

void ProcessingOrchestrator::process(const ChannelMixer& channelMixer) {
	using clock = std::chrono::high_resolution_clock;
	static_assert(clock::is_steady);

	const auto processBeginTime = clock::now();

	const std::chrono::duration<float, std::milli> processMaxDuration{ killTimeoutMs };
	const auto killTime = clock::now() + std::chrono::duration_cast<std::chrono::milliseconds>(processMaxDuration);

	bool killed = false;
	for (auto& [name, sa] : saMap) {
		killed = sa.process(channelMixer, killTime);
		if (killed) {
			break;
		}
	}

	const auto processEndTime = clock::now();

	const auto processDuration = std::chrono::duration<double, std::milli>{ processEndTime - processBeginTime }.count();

	if (killed) {
		logger.error(L"handler processing was killed on timeout after {} m, on stage 1", processDuration);
		return;
	}

	for (auto& [name, sa] : saMap) {
		sa.updateSnapshot(dataSnapshot[name]);
	}

	if (computeTimeoutMs >= 0 && processDuration > computeTimeoutMs) {
		logger.debug(
			L"processing overhead {} ms over specified {} ms",
			processDuration - computeTimeoutMs,
			computeTimeoutMs
		);
	}
}

void ProcessingOrchestrator::configureSnapshot(DataSnapshot& snapshot) const {
	snapshot = dataSnapshot;
}
