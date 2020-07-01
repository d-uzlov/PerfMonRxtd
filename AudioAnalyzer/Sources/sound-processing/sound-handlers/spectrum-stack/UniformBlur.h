/*
 * Copyright (C) 2019 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "../SoundHandler.h"
#include "BandResampler.h"

namespace rxtd::audio_analyzer {
	class UniformBlur : public ResamplerProvider {
	public:
		struct Params {
		private:
			friend UniformBlur;

			istring resamplerId;

			double blurRadius;
			double blurRadiusAdaptation;
		};

	private:
		struct CascadeInfo {
			struct BandInfo {
				float weight { };
				float blurSigma { };
			};

			std::vector<float> magnitudes;
			std::vector<BandInfo> bandsInfo;

			void setSize(index size) {
				magnitudes.resize(size);
				bandsInfo.resize(size);
			}
		};

		class GaussianCoefficientsManager {
			// radius -> coefs vector
			std::unordered_map<index, std::vector<double>> blurCoefficients;
		public:
			const std::vector<double>& forRadius(index radius);

		private:
			static std::vector<double> generateGaussianKernel(index radius);
		};
		GaussianCoefficientsManager gcm;

		Params params { };

		index samplesPerSec { };

		const BandResampler* resampler = nullptr;
		std::vector<std::vector<float>> blurredValues;

		bool changed = true;
		bool valid = false;

		mutable string propString { };

	public:

		static std::optional<Params> parseParams(const utils::OptionMap& optionMap, utils::Rainmeter::Logger& cl);

		void setParams(Params _params);

		void setSamplesPerSec(index samplesPerSec) override;
		void reset() override;

		void process(const DataSupplier& dataSupplier) override;
		void processSilence(const DataSupplier& dataSupplier) override;
		void finish(const DataSupplier& dataSupplier) override;


		bool isValid() const override {
			return valid;
		}
		array_view<float> getData(layer_t layer) const override;
		layer_t getLayersCount() const override;

		const wchar_t* getProp(const isview& prop) const override;

		const BandResampler* getResampler() const override {
			if (!valid) {
				return nullptr;
			}

			return resampler;
		}

	private:
		void blurData(const SoundHandler& resampler);
	};
}