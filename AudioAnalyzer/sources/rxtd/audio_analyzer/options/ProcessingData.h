﻿// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Danil Uzlov

#pragma once
#include "rxtd/audio_analyzer/sound_processing/Channel.h"
#include "rxtd/audio_analyzer/sound_processing/sound_handlers/ExternalMethods.h"
#include "rxtd/filter_utils/FilterCascadeParser.h"

namespace rxtd::audio_analyzer::options {
	struct ProcessingData {
		using Finisher = handler::ExternalMethods::FinishMethodType;

		struct FilterInfo {
			string raw;
			filter_utils::FilterCascadeCreator creator;

			friend bool operator==(const FilterInfo& lhs, const FilterInfo& rhs) {
				return lhs.raw == rhs.raw;
			}

			friend bool operator!=(const FilterInfo& lhs, const FilterInfo& rhs) { return !(lhs == rhs); }
		};

		FilterInfo filter;
		index targetRate{};
		std::vector<Channel> channels;
		istring handlersRaw;
		std::vector<istring> handlerOrder;
		std::map<istring, HandlerInfo, std::less<>> handlers;

		// autogenerated
		friend bool operator==(const ProcessingData& lhs, const ProcessingData& rhs) {
			return lhs.filter == rhs.filter
				&& lhs.targetRate == rhs.targetRate
				&& lhs.channels == rhs.channels
				&& lhs.handlersRaw == rhs.handlersRaw
				&& lhs.handlers == rhs.handlers;
		}

		friend bool operator!=(const ProcessingData& lhs, const ProcessingData& rhs) { return !(lhs == rhs); }
	};
}
