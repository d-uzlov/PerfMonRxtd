﻿// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2019 Danil Uzlov

#pragma once
#include "rxtd/audio_analyzer/sound_processing/sound_handlers/HandlerBase.h"
#include "rxtd/fft_utils/FftCascade.h"
#include "rxtd/fft_utils/RealFft.h"
#include "rxtd/fft_utils/WindowFunctionHelper.h"

namespace rxtd::audio_analyzer::handler {
	class FftAnalyzer : public HandlerBase {
		using WCF = fft_utils::WindowFunctionHelper::WindowCreationFunc;

		struct Params {
		private:
			friend FftAnalyzer;

			double binWidth{};
			double overlap{};

			index cascadesCount{};

			double randomTest{};
			double randomDuration{};

			string wcfDescription{};
			WCF createWindow{};

			// autogenerated
			friend bool operator==(const Params& lhs, const Params& rhs) {
				return lhs.binWidth == rhs.binWidth
					&& lhs.overlap == rhs.overlap
					&& lhs.cascadesCount == rhs.cascadesCount
					&& lhs.randomTest == rhs.randomTest
					&& lhs.randomDuration == rhs.randomDuration
					&& lhs.wcfDescription == rhs.wcfDescription;
			}

			friend bool operator!=(const Params& lhs, const Params& rhs) {
				return !(lhs == rhs);
			}
		};

		struct Snapshot {
			index fftSize{};
			index sampleRate{};
			index cascadesCount{};
		};

		Params params{};

		index fftSize = 0;
		index inputStride = 0;

		index randomBlockSize = 0;
		index randomCurrentOffset = 0;

		enum class RandomState { eON, eOFF } randomState{ RandomState::eON };

		std::vector<fft_utils::FftCascade> cascades{};

		fft_utils::RealFft fft{};

	public:
		[[nodiscard]]
		bool vCheckSameParams(const ParamsContainer& p) const override {
			return compareParamsEquals(params, p);
		}

		[[nodiscard]]
		ParamsContainer vParseParams(ParamParseContext& context) const noexcept(false) override;

	protected:
		[[nodiscard]]
		ConfigurationResult vConfigure(const ParamsContainer& _params, Logger& cl, ExternalData& externalData) override;

	public:
		index getFftSize() const {
			return fftSize;
		}

		void vProcess(ProcessContext context, ExternalData& externalData) override;

	protected:
		ExternalMethods::GetPropMethodType vGetExt_getProp() const override {
			return wrapExternalGetProp<Snapshot, &getProp>();
		}

	private:
		static bool getProp(
			const Snapshot& snapshot,
			isview prop,
			const ExternalMethods::CallContext& context
		);

		void processRandom(index waveSize, clock::time_point killTime);
	};
}
