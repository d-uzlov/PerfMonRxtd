/*
 * Copyright (C) 2019-2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include <unordered_map>
#include "array_view.h"

namespace rxtd::audio_utils {
	class GaussianCoefficientsManager {
		// radius -> coefs vector
		std::unordered_map<index, std::vector<double>> blurCoefficients;
		
	public:
		array_view<double> forRadius(index radius) {
			auto& vec = blurCoefficients[radius];
			if (vec.empty()) {
				vec = generateGaussianKernel(radius);
			}

			return vec;
		}

	private:
		static std::vector<double> generateGaussianKernel(index radius);
	};
}
