/*
 * Copyright (C) 2019-2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "../SoundHandler.h"
#include "image-utils/Color.h"
#include "RainmeterWrappers.h"
#include "image-utils/StripedImage.h"
#include "image-utils/ImageWriteHelper.h"
#include "image-utils/StripedImageFadeHelper.h"

namespace rxtd::audio_analyzer {
	class Spectrogram : public SoundHandler {
	public:

		struct Params {
		private:
			friend Spectrogram;

			double resolution{ };
			index length{ };
			index borderSize{ };
			string prefix = { };
			utils::Color borderColor{ };
			double fading{ };

			struct ColorDescription {
				float widthInverted{ };
				utils::Color color;

				friend bool operator==(const ColorDescription& lhs, const ColorDescription& rhs) {
					return lhs.widthInverted == rhs.widthInverted
						&& lhs.color == rhs.color;
				}

				friend bool operator!=(const ColorDescription& lhs, const ColorDescription& rhs) {
					return !(lhs == rhs);
				}
			};

			std::vector<float> colorLevels;
			std::vector<ColorDescription> colors;
			float colorMinValue{ };
			float colorMaxValue{ };
			utils::Color::Mode mixMode{ };

			bool stationary{ };

			//  autogenerated
			friend bool operator==(const Params& lhs, const Params& rhs) {
				return lhs.resolution == rhs.resolution
					&& lhs.length == rhs.length
					&& lhs.borderSize == rhs.borderSize
					&& lhs.prefix == rhs.prefix
					&& lhs.borderColor == rhs.borderColor
					&& lhs.fading == rhs.fading
					&& lhs.colorLevels == rhs.colorLevels
					&& lhs.colors == rhs.colors
					&& lhs.colorMinValue == rhs.colorMinValue
					&& lhs.colorMaxValue == rhs.colorMaxValue
					&& lhs.mixMode == rhs.mixMode
					&& lhs.stationary == rhs.stationary;
			}

			friend bool operator!=(const Params& lhs, const Params& rhs) {
				return !(lhs == rhs);
			}
		};

		struct Snapshot {
			index blockSize { };

			string prefix;

			utils::Vector2D<utils::IntColor> pixels;
			bool empty { };

			mutable utils::ImageWriteHelper writerHelper { };
			mutable bool writeNeeded { };

			mutable string filenameBuffer;
		};

	private:
		Params params;

		index blockSize{ };

		std::vector<utils::IntColor> stripBuffer{ };
		index counter = 0;
		mutable bool writeNeeded = false;

		index dataShortageEqSize{ };
		index overpushCount{ };
		bool lastDataIsZero{ };

		utils::StripedImage<utils::IntColor> image{ };
		utils::StripedImageFadeHelper sifh{ };
		utils::ImageWriteHelper writerHelper{ };

	public:
		[[nodiscard]]
		bool checkSameParams(const std::any& p) const override {
			return compareParamsEquals(params, p);
		}

		[[nodiscard]]
		ParseResult parseParams(
			const OptionMap& om, Logger& cl, const Rainmeter& rain,
			index legacyNumber
		) const override;

	protected:
		[[nodiscard]]
		ConfigurationResult vConfigure(const std::any& _params, Logger& cl, std::any& snapshotAny) override;

	public:
		void vProcess(array_view<float> wave, clock::time_point killTime) override;

		void vUpdateSnapshot(std::any& handlerSpecificData) const override;

	private:
		static void staticFinisher(const Snapshot& snapshot, const ExternCallContext& context);

		void pushStrip();

		void fillStrip(array_view<float> data, array_span<utils::IntColor> buffer) const;
		void fillStripMulticolor(array_view<float> data, array_span<utils::IntColor> buffer) const;

		static bool getProp(
			const Snapshot& snapshot,
			isview prop,
			utils::BufferPrinter& printer,
			const ExternCallContext& context
		);
	};
}
