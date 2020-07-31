/*
 * Copyright (C) 2019-2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include "SoundAnalyzer.h"

using namespace audio_analyzer;

void SoundAnalyzer::setFormat(index sampleRate, ChannelLayout _layout) {
	if (sourceSampleRate == sampleRate && layout == _layout) {
		return;
	}

	sourceSampleRate = sampleRate;
	layout = std::move(_layout);

	cph.setSourceRate(sampleRate);
	patchCH();
	updateHandlerSampleRate();
}

AudioChildHelper SoundAnalyzer::getAudioChildHelper() const {
	return AudioChildHelper{ channels };
}

void SoundAnalyzer::setParams(
	std::set<Channel> channelSetRequested,
	ParamParser::HandlerPatcherInfo handlerPatchers,
	double granularity
) {
	this->channelSetRequested = std::move(channelSetRequested);
	this->handlerPatchers = std::move(handlerPatchers);
	this->granularity = granularity;

	patchCH();
}

bool SoundAnalyzer::process(const ChannelMixer& mixer, clock::time_point killTime) {
	cph.reset();
	cph.setChannelMixer(mixer);
	dataSupplier.logger = logger;

	const index bufferSize = index(granularity * cph.getSampleRate());
	cph.setGrabBufferSize(bufferSize);

	for (auto& [channel, channelData] : channels) {
		cph.setCurrentChannel(channel);
		while (true) {
			auto wave = cph.grabNext();
			if (wave.empty()) {
				break;
			}

			dataSupplier.setWave(wave);

			for (auto& name : handlerPatchers.order) {
				auto& handler = channelData[name];
				if (!handler.wasValid) {
					continue;
				}

				handler.ptr->process(dataSupplier);

				handler.wasValid = handler.ptr->isValid();
				if (!handler.wasValid) {
					logger.error(L"handler '{}' was invalidated", name);
				}

				if (clock::now() > killTime) {
					return true;
				}
			}
		}
	}

	return false;
}

bool SoundAnalyzer::finishStandalone(clock::time_point killTime) {
	for (auto& [channel, channelData] : channels) {
		for (auto& name : handlerPatchers.order) {
			auto& handler = channelData[name];
			if (handler.wasValid && handler.ptr->isStandalone()) {
				handler.ptr->finish();
				handler.wasValid = handler.ptr->isValid();

				if (!handler.wasValid) {
					logger.error(L"handler '{}' was invalidated", name);
				}

				if (clock::now() > killTime) {
					return true;
				}
			}
		}
	}

	return false;
}

void SoundAnalyzer::resetValues() noexcept {
	for (auto& [channel, channelData] : channels) {
		for (auto& [name, handler] : channelData) {
			// order is not important
			handler.ptr->reset();
		}
	}
}

void SoundAnalyzer::updateHandlerSampleRate() noexcept {
	for (auto& [channel, channelData] : channels) {
		for (auto& [name, handler] : channelData) {
			// order is not important
			handler.ptr->setSamplesPerSec(cph.getSampleRate());
			handler.wasValid = handler.ptr->isValid();
		}
	}
}

void SoundAnalyzer::patchChannels() {
	// Delete not needed channels
	std::vector<Channel> toDelete;
	for (const auto& [channel, _] : channels) {
		const bool exists = channel == Channel::eAUTO || layout.contains(channel);
		const bool isRequested = channelSetRequested.count(channel) >= 1;

		if (!exists || !isRequested) {
			toDelete.push_back(channel);
		}
	}
	for (auto c : toDelete) {
		channels.erase(c);
	}

	std::set<Channel> channelsSet;
	// Create missing channels
	for (const auto channel : channelSetRequested) {
		const bool exists = channel == Channel::eAUTO || layout.contains(channel);
		if (exists) {
			channels[channel];
			channelsSet.insert(channel);
		}
	}
	cph.setChannels(channelsSet);
}

void SoundAnalyzer::patchHandlers() {
	for (auto& [channel, channelData] : channels) {
		ChannelData newData;
		HandlerFinderImpl hf;
		hf.setChannelData(newData);

		for (auto& handlerName : handlerPatchers.order) {
			auto& patcher = handlerPatchers.map[handlerName].patcher;
			auto& handlerInfo = channelData[handlerName];

			SoundHandler* ptr = handlerInfo.ptr.get();
			if (ptr != nullptr) {
				ptr->prePatch();
			}

			ptr = patcher(ptr, channel);
			if (ptr != handlerInfo.ptr.get()) {
				handlerInfo.ptr = std::unique_ptr<SoundHandler>(ptr);
			}

			auto cl = logger.context(L"Handler {}: ", handlerName);
			ptr->linkSources(hf, cl);
			if (!ptr->isValid()) {
				cl.error(L"invalid handler");
				continue;
			}

			handlerInfo.wasValid = true;
			newData[handlerName] = std::move(handlerInfo);
		}

		channelData = std::move(newData);
	}

	updateHandlerSampleRate();
}
