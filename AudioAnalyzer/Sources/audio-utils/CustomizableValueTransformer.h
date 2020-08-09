/*
 * Copyright (C) 2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include <array>
#include "RainmeterWrappers.h"
#include "filter-utils/LogarithmicIRF.h"
#include "LinearInterpolator.h"
#include "array_view.h"

namespace rxtd::audio_utils {
	class CustomizableValueTransformer {
	public:
		enum class TransformType {
			eFILTER,
			eDB,
			eMAP,
			eCLAMP,
		};

		struct TransformationInfo {
			TransformType type{ };
			std::array<float, 2> args{ };
			std::vector<float> pastFilterValues;

			union {
				LogarithmicIRF filter{ };
				utils::LinearInterpolatorF interpolator;
			} state{ };

			// autogenerated
			friend bool operator==(const TransformationInfo& lhs, const TransformationInfo& rhs) {
				return lhs.type == rhs.type;
			}

			friend bool operator!=(const TransformationInfo& lhs, const TransformationInfo& rhs) {
				return !(lhs == rhs);
			}
		};

	private:
		std::vector<TransformationInfo> transforms;
		index historyWidth = 0;
		bool _hasState = false;

	public:
		CustomizableValueTransformer() = default;
		~CustomizableValueTransformer() = default;

		explicit CustomizableValueTransformer(std::vector<TransformationInfo> transformations);

		CustomizableValueTransformer(const CustomizableValueTransformer& other) = default;
		CustomizableValueTransformer(CustomizableValueTransformer&& other) noexcept = default;
		CustomizableValueTransformer& operator=(const CustomizableValueTransformer& other) = default;
		CustomizableValueTransformer& operator=(CustomizableValueTransformer&& other) noexcept = default;

		// autogenerated
		friend bool operator==(const CustomizableValueTransformer& lhs, const CustomizableValueTransformer& rhs) {
			return lhs.transforms == rhs.transforms;
		}

		friend bool operator!=(const CustomizableValueTransformer& lhs, const CustomizableValueTransformer& rhs) {
			return !(lhs == rhs);
		}

		[[nodiscard]]
		bool hasState() const {
			return _hasState;
		}

		[[nodiscard]]
		bool isEmpty() const {
			return transforms.empty();
		}

		[[nodiscard]]
		float apply(float value);

		void applyToArray(array_view<float> source, array_span<float> dest);

		void setParams(index samplesPerSec, index blockSize);

		void resetState();

		void setHistoryWidth(index value);
	};

	class TransformationParser {
		using Transformation = CustomizableValueTransformer::TransformationInfo;
		using TransformType = CustomizableValueTransformer::TransformType;

	public:
		[[nodiscard]]
		static CustomizableValueTransformer parse(sview transformDescription, utils::Rainmeter::Logger& cl);

	private:
		[[nodiscard]]
		static std::optional<Transformation> parseTransformation(utils::OptionList list, utils::Rainmeter::Logger& cl);
	};
}
