// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Danil Uzlov

#pragma once
#include <array>
#include "rxtd/LinearInterpolator.h"
#include "rxtd/Logger.h"
#include "rxtd/option_parsing/OptionList.h"
#include "rxtd/option_parsing/OptionParser.h"

namespace rxtd::audio_analyzer::audio_utils {
	class CustomizableValueTransformer {
	public:
		using Option = option_parsing::Option;
		using OptionMap = option_parsing::OptionMap;
		using OptionParser = option_parsing::OptionParser;

		enum class TransformType {
			eDB,
			eMAP,
			eCLAMP,
		};

		struct TransformationInfo {
			TransformType type{};
			std::array<float, 2> args{};

			LinearInterpolator<float> interpolator;

			// autogenerated
			friend bool operator==(const TransformationInfo& lhs, const TransformationInfo& rhs) {
				return lhs.type == rhs.type
					&& lhs.args == rhs.args
					&& lhs.interpolator == rhs.interpolator;
			}

			friend bool operator!=(const TransformationInfo& lhs, const TransformationInfo& rhs) { return !(lhs == rhs); }
		};

		struct TransformationInfo2 {
			TransformType type{};
			std::array<float, 2> args{};

			LinearInterpolator<float> interpolator;
		};

	private:
		std::vector<TransformationInfo> transforms;

	public:
		CustomizableValueTransformer() = default;

		explicit CustomizableValueTransformer(std::vector<TransformationInfo> transformations) :
			transforms(std::move(transformations)) { }

		// autogenerated
		friend bool operator==(const CustomizableValueTransformer& lhs, const CustomizableValueTransformer& rhs) {
			return lhs.transforms == rhs.transforms;
		}

		friend bool operator!=(const CustomizableValueTransformer& lhs, const CustomizableValueTransformer& rhs) {
			return !(lhs == rhs);
		}

		[[nodiscard]]
		bool isEmpty() const {
			return transforms.empty();
		}

		[[nodiscard]]
		double apply(double value);

		[[nodiscard]]
		float apply(float value) {
			return static_cast<float>(apply(static_cast<double>(value)));
		}

		void applyToArray(array_view<float> source, array_span<float> dest);

		// can throw OptionParser::Exception
		[[nodiscard]]
		static CustomizableValueTransformer parse(sview transformDescription, OptionParser& parser, const Logger& cl);

	private:
		// can throw OptionParser::Exception
		[[nodiscard]]
		static TransformationInfo parseTransformation(const Option& nameOpt, const OptionMap& params, OptionParser& parser, const Logger& cl);
	};
}
