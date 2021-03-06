// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2021 Danil Uzlov

#include <CppUnitTest.h>

#include "rxtd/fft_utils/RealFft.h"
#include "rxtd/fft_utils/WindowFunctionHelper.h"
#include "rxtd/std_fixes/MyMath.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using rxtd::std_fixes::MyMath;

namespace rxtd::test::fft_utils {
	using namespace rxtd::fft_utils;
	TEST_CLASS(RealFft_test) {
		static_assert(std::is_same<float, RealFft::scalar_type>::value);
		RealFft fft;

		std::vector<float> wave;
		std::vector<float> frequencies;

	public:
		TEST_METHOD(Test_32_1) {
			testForward(32, 1, 1.0e-6f);
		}

		TEST_METHOD(Test_96_5) {
			testForward(96, 5, 1.0e-5f);
		}

		TEST_METHOD(Test_1024_20) {
			testForward(1024, 20, 1.0e-5f);
		}

		TEST_METHOD(Test_1024_125) {
			testForward(1024, 125, 1.0e-4f);
		}

		TEST_METHOD(Test_1024_511) {
			testForward(1024, 511, 1.0e-4f);
		}

	private:
		void doForward(index size, index desiredWaveSinCount) {
			wave.resize(static_cast<size_t>(size));
			generateSinWave(wave, static_cast<float>(desiredWaveSinCount));

			fft.setParams(size, std::vector<float>{});
			fft.process(wave);

			frequencies.resize(static_cast<size_t>(size) / 2);
			fft.fillMagnitudes(frequencies);
		}

		void testForward(index size, index desiredWaveSinCount, float tolerance) {
			Assert::IsTrue(desiredWaveSinCount > 0);
			Assert::IsTrue(desiredWaveSinCount < size / 2);

			doForward(size, desiredWaveSinCount);

			auto freqs = array_span{ frequencies };
			for (index i = 0; i < freqs.size(); i++) {
				if (i == desiredWaveSinCount) {
					Assert::AreEqual(1.0f, freqs[i], tolerance);
				} else {
					Assert::AreEqual(0.0f, freqs[i], tolerance);
				}
			}
		}

		static void generateSinWave(array_span<float> array, float count) {
			Assert::IsTrue(count >= 1.0f);

			for (index i = 0; i < array.size(); i++) {
				array[i] = std::sin(count * static_cast<float>(i) * MyMath::pi<float>() * 2.0f / static_cast<float>(array.size()));
			}
		}
	};
}
