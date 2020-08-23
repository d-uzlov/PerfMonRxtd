/*
 * Copyright (C) 2019-2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once

#include "TypeHolder.h"
#include "RainmeterWrappers.h"
#include "sound-processing/ProcessingManager.h"
#include "sound-processing/device-management/DeviceManager.h"
#include "sound-processing/ChannelProcessingHelper.h"
#include "windows-wrappers/IMMNotificationClientImpl.h"

namespace rxtd::audio_analyzer {
	class AudioParent : public utils::ParentBase {
		ParamParser paramParser;
		ChannelMixer channelMixer;
		DeviceManager deviceManager;
		std::map<istring, ProcessingManager, std::less<>> saMap;
		MyWaveFormat currentFormat{ };
		double computeTimeout = 0.0;
		double killTimeout = 0.0;

		utils::GenericComWrapper<utils::CMMNotificationClient> notificationClient;

	public:
		explicit AudioParent(utils::Rainmeter&& rain);
		~AudioParent() = default;

		/** This class is non copyable */
		AudioParent(const AudioParent& other) = delete;
		AudioParent(AudioParent&& other) = delete;
		AudioParent& operator=(const AudioParent& other) = delete;
		AudioParent& operator=(AudioParent&& other) = delete;

	protected:
		void vReload() override;
		double vUpdate() override;
		void vCommand(isview bangArgs) override;
		void vResolve(array_view<isview> args, string& resolveBufferString) override;

	public:
		[[nodiscard]]
		double getValue(isview proc, isview id, Channel channel, index ind) const;
		[[nodiscard]]
		double legacy_getValue(isview id, Channel channel, index ind) const;

		index getLegacyNumber() const {
			return paramParser.getLegacyNumber();
		}

		// returns error message or empty string
		string checkHandler(isview procName, Channel channel, isview handlerName) const {
			utils::BufferPrinter bp;

			const auto procDataIter = paramParser.getParseResult().find(procName);
			if (procDataIter == paramParser.getParseResult().end()) {
				bp.print(L"processing {} is not found", procName);
				return bp.getBufferPtr();
			}

			auto procData = procDataIter->second;
			auto& channels = procData.channels;
			if (channels.find(channel) == channels.end()) {
				bp.print(L"processing {} doesn't have channel {}", procName, channel.technicalName());
				return bp.getBufferPtr();
			}

			auto& handlerMap = procData.handlersInfo.patchers;
			if (handlerMap.find(handlerName) == handlerMap.end()) {
				bp.print(L"processing {} doesn't have handler {}", procName, handlerName);
				return bp.getBufferPtr();
			}

			return { };
		}

	private:
		void process();

		void patchSA(const ParamParser::ProcessingsInfoMap& procs);

		[[nodiscard]]
		std::pair<SoundHandler*, AudioChildHelper> findHandlerByName(isview name, Channel channel) const;

		void legacy_resolve(array_view<isview> args, string& resolveBufferString);
	};
}
