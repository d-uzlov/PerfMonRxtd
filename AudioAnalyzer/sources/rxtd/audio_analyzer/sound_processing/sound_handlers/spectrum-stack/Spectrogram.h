/*
 * Copyright (C) 2019-2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "rxtd/audio_analyzer/image_utils/Color.h"
#include "rxtd/audio_analyzer/image_utils/ImageWriteHelper.h"
#include "rxtd/audio_analyzer/image_utils/StripedImage.h"
#include "rxtd/audio_analyzer/image_utils/StripedImageFadeHelper.h"
#include "rxtd/audio_analyzer/sound_processing/sound_handlers/HandlerBase.h"
#include "rxtd/audio_utils/MinMaxCounter.h"

namespace rxtd::audio_analyzer::handler {
	class Spectrogram : public HandlerBase {
		using Color = image_utils::Color;
		using IntColor = image_utils::IntColor;
		template<typename T>
		using StripedImage = image_utils::StripedImage<T>;
		using ImageWriteHelper = image_utils::ImageWriteHelper;
		using StripedImageFadeHelper = image_utils::StripedImageFadeHelper;
		
		struct ColorDescription {
			float widthInverted{};
			Color color;

			friend bool operator==(const ColorDescription& lhs, const ColorDescription& rhs) {
				return lhs.widthInverted == rhs.widthInverted
					&& lhs.color == rhs.color;
			}

			friend bool operator!=(const ColorDescription& lhs, const ColorDescription& rhs) {
				return !(lhs == rhs);
			}
		};

		struct Params {
			double resolution{};
			index length{};
			index borderSize{};
			string folder = {};
			Color borderColor{};
			double fading{};

			std::vector<float> colorLevels;
			std::vector<ColorDescription> colors;
			Color::Mode mixMode{};

			bool stationary{};
			float silenceThreshold{};

			//  autogenerated
			friend bool operator==(const Params& lhs, const Params& rhs) {
				return lhs.resolution == rhs.resolution
					&& lhs.length == rhs.length
					&& lhs.borderSize == rhs.borderSize
					&& lhs.folder == rhs.folder
					&& lhs.borderColor == rhs.borderColor
					&& lhs.fading == rhs.fading
					&& lhs.colorLevels == rhs.colorLevels
					&& lhs.colors == rhs.colors
					&& lhs.mixMode == rhs.mixMode
					&& lhs.stationary == rhs.stationary
					&& lhs.silenceThreshold == rhs.silenceThreshold;
			}

			friend bool operator!=(const Params& lhs, const Params& rhs) {
				return !(lhs == rhs);
			}
		};

		struct Snapshot {
			index blockSize{};

			string prefix;

			Vector2D<IntColor> pixels;
			bool empty{};

			mutable ImageWriteHelper writerHelper{};
			mutable bool writeNeeded{};

			mutable string filenameBuffer;
		};

		class InputStripMaker {
			index counter{};
			index blockSize{};
			index chunkEquivalentWaveSize{};
			array_view<ColorDescription> colors;
			array_view<float> colorLevels;

			array_view<array_view<float>> chunks;
			std::vector<IntColor> buffer{};

		public:
			void setParams(
				index _blockSize, index _chunkEquivalentWaveSize, index bufferSize,
				array_view<ColorDescription> _colors, array_view<float> _colorLevels
			) {
				blockSize = _blockSize;
				chunkEquivalentWaveSize = _chunkEquivalentWaveSize;
				colors = _colors;
				colorLevels = _colorLevels;
				counter = 0;

				buffer.resize(static_cast<size_t>(bufferSize));
				std::fill(buffer.begin(), buffer.end(), colors[0].color.toIntColor());
			}

			[[nodiscard]]
			array_view<IntColor> getBuffer() const {
				return buffer;
			}

			void setChunks(array_view<array_view<float>> value) {
				chunks = value;
			}

			void next() {
				array_view<float> chunk;
				while (counter < blockSize && !chunks.empty()) {
					counter += chunkEquivalentWaveSize;
					chunk = chunks.front();
					chunks.remove_prefix(1);
				}

				if (counter >= blockSize) {
					counter -= blockSize;
				}

				if (!chunk.empty()) {
					if (colors.size() == 2) {
						// only use 2 colors
						fillStrip(chunk, buffer);
					} else {
						// many colors, but slightly slower
						fillStripMulticolor(chunk, buffer);
					}
				}
			}

		private:
			void fillStrip(array_view<float> data, array_span<IntColor> buffer) const;
			void fillStripMulticolor(array_view<float> data, array_span<IntColor> buffer) const;
		};

		Params params;

		index blockSize{};


		mutable bool imageHasChanged = false;

		audio_utils::MinMaxCounter minMaxCounter;

		StripedImage<IntColor> image{};
		StripedImageFadeHelper fadeHelper{};
		InputStripMaker ism;

	public:
		[[nodiscard]]
		bool vCheckSameParams(const ParamsContainer& p) const override {
			return compareParamsEquals(params, p);
		}

		[[nodiscard]]
		ParamsContainer vParseParams(
			const OptionMap& om, Logger& cl, const Rainmeter& rain,
			Version version
		) const override;

	protected:
		[[nodiscard]]
		ConfigurationResult vConfigure(const ParamsContainer& _params, Logger& cl, ExternalData& externalData) override;

	public:
		void vProcess(ProcessContext context, ExternalData& externalData) override;

	protected:
		ExternalMethods::GetPropMethodType vGetExt_getProp() const override {
			return wrapExternalGetProp<Snapshot, &getProp>();
		}

		ExternalMethods::FinishMethodType vGetExt_finish() const override {
			return wrapExternalFinish<Snapshot, &staticFinisher>();
		}

	private:
		static void staticFinisher(const Snapshot& snapshot, const ExternalMethods::CallContext& context);

		static bool getProp(
			const Snapshot& snapshot,
			isview prop,
			BufferPrinter& printer,
			const ExternalMethods::CallContext& context
		);
	};
}
