﻿/*
 * Copyright (C) 2019-2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once

#include "LogErrorHelper.h"
#include "TypeHolder.h"
#include "ParentHelper.h"

namespace rxtd::audio_analyzer {
	class AudioParent : public utils::ParentBase {
		using DeviceRequest = std::optional<CaptureManager::SourceDesc>;

		index legacyNumber{ };
		ParamParser paramParser;

		struct {
			NoArgLogErrorHelper generic;
			LogErrorHelper<istring> sourceTypeIsNotRecognized;
			LogErrorHelper<istring> unknownCommand;
			LogErrorHelper<istring> currentDeviceUnknownProp;
			LogErrorHelper<istring> unknownSectionVariable;
			LogErrorHelper<istring> legacy_invalidPort;

			LogErrorHelper<istring> processingNotFound;
			LogErrorHelper<istring> channelNotRecognized;
			LogErrorHelper<istring, istring> processingDoesNotHaveHandler;
			LogErrorHelper<istring, istring> processingDoesNotHaveChannel;
			LogErrorHelper<istring> legacy_handlerNotFound;
			LogErrorHelper<istring> handlerDoesNotHaveProps;
			LogErrorHelper<istring, istring> propNotFound;

			void setLogger(utils::Rainmeter::Logger logger) {
				generic.setLogger(logger);
				sourceTypeIsNotRecognized.setLogger(logger);
				unknownCommand.setLogger(logger);
				currentDeviceUnknownProp.setLogger(logger);
				unknownSectionVariable.setLogger(logger);
				legacy_invalidPort.setLogger(logger);
				processingNotFound.setLogger(logger);
				channelNotRecognized.setLogger(logger);
				processingDoesNotHaveHandler.setLogger(logger);
				processingDoesNotHaveChannel.setLogger(logger);
				legacy_handlerNotFound.setLogger(logger);
				handlerDoesNotHaveProps.setLogger(logger);
				propNotFound.setLogger(logger);
			}

			void reset() {
				// helpers that are commented don't need to be reset
				// because it doesn't make sense to repeat their messages after measure is reloaded

				// generic.reset();
				// sourceTypeIsNotRecognized.reset();
				// unknownCommand.reset();
				// currentDeviceUnknownProp.reset();
				// unknownSectionVariable.reset();
				// legacy_invalidPort.reset();

				processingNotFound.reset();
				channelNotRecognized.reset();
				processingDoesNotHaveHandler.reset();
				processingDoesNotHaveChannel.reset();
				legacy_handlerNotFound.reset();
				// handlerDoesNotHaveProps.reset();
				// propNotFound.reset();
			}
		} logHelpers;

		DeviceRequest requestedSource;
		ParentHelper helper;

		bool firstReload = true;

		// handlerName → handlerData for cleanup
		using ProcessingCleanersMap = std::map<istring, std::any, std::less<>>;
		using CleanersMap = std::map<istring, ProcessingCleanersMap, std::less<>>;

		bool cleanersExecuted = false;

		CleanersMap cleanersMap;

	public:
		explicit AudioParent(utils::Rainmeter&& rain);

	protected:
		void vReload() override;
		double vUpdate() override;
		void vCommand(isview bangArgs) override;
		void vResolve(array_view<isview> args, string& resolveBufferString) override;

	public:
		[[nodiscard]]
		double getValue(isview proc, isview id, Channel channel, index ind);

		index getLegacyNumber() const {
			return paramParser.getLegacyNumber();
		}

		// returns error message or empty string
		string checkHandler(isview procName, Channel channel, isview handlerName) const;

		isview legacy_findProcessingFor(isview handlerName) const;

	private:
		DeviceRequest readRequest() const;
		void resolveProp(array_view<isview> args, string& resolveBufferString);
		ProcessingCleanersMap createCleanersFor(const ParamParser::ProcessingData& pd) const;
		void runCleaners() const;
	};
}
