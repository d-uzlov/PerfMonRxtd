/*
 * Copyright (C) 2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "DownsampleHelper.h"
#include "filter-utils/LogarithmicIRF.h"
#include "FFT.h"

namespace rxtd::audio_utils {
	class FftCascade {
	public:
		struct Params {
			index fftSize;
			index samplesPerSec;

			double legacy_attackTime;
			double legacy_decayTime;

			index inputStride;
			bool legacy_correctZero;
		};

	private:
		class RingBuffer {
			std::vector<float> buffer;
			index endOffset = 0;
			index takenOffset = 0;

			DownsampleHelperFixed<5, 2> downsampleHelper;

		public:
			// returns part of the wave that didn't fit info the buffer
			[[nodiscard]]
			array_view<float> fill(array_view<float> wave);

			// returns part of the wave that didn't fit info the buffer
			[[nodiscard]]
			array_view<float> fillResampled(array_view<float> wave);

			void shift(index stride);

			[[nodiscard]]
			bool isFull() const {
				return endOffset == index(buffer.size());
			}

			[[nodiscard]]
			array_view<float> getBuffer() const {
				return buffer;
			}

			[[nodiscard]]
			array_view<float> take();

			void setSize(index value) {
				buffer.resize(value);
				reset();
			}

			void reset() {
				endOffset = 0;
				takenOffset = 0;
				downsampleHelper.reset();
			}
		};

		FftCascade* successorPtr{ };
		FFT* fftPtr{ };

		Params params{ };
		index cascadeIndex{ };

		RingBuffer buffer;
		LogarithmicIRF filter{ };
		std::vector<float> values;
		float legacy_dc{ };
		bool hasChanges = false;

	public:
		void setParams(Params _params, FFT* _fftPtr, FftCascade* _successorPtr, index _cascadeIndex);
		void process(array_view<float> wave);
		void reset();

		[[nodiscard]]
		bool grabChanges() {
			const bool result = hasChanges;
			hasChanges = false;
			return result;
		}

		[[nodiscard]]
		array_view<float> getValues() const {
			return values;
		}

		[[nodiscard]]
		double legacy_getDC() const {
			return legacy_dc;
		}

	private:
		void resampleResult();
		void doFft();
	};
}
