// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Danil Uzlov

#include "CustomizableValueTransformer.h"

#include "rxtd/option_parsing/OptionList.h"
#include "rxtd/option_parsing/OptionMap.h"
#include "rxtd/option_parsing/OptionSequence.h"
#include "rxtd/std_fixes/MathBitTwiddling.h"

using CVT = rxtd::audio_analyzer::audio_utils::CustomizableValueTransformer;
using rxtd::std_fixes::MathBitTwiddling;
using rxtd::option_parsing::Option;
using rxtd::option_parsing::OptionMap;

double CVT::apply(double value) {
	for (auto& transform : transforms) {
		switch (transform.type) {
		case TransformType::eDB: {
			value = std::max<double>(value, 0.0);
			value = 10.0 * std::log10(value);
			break;
		}
		case TransformType::eMAP: {
			value = static_cast<double>(transform.interpolator.toValue(static_cast<float>(value)));
			break;
		}
		case TransformType::eCLAMP: {
			value = static_cast<double>(std::clamp(static_cast<float>(value), transform.args[0], transform.args[1]));
			break;
		}
		}
	}

	return value;
}

void CVT::applyToArray(array_view<float> source, array_span<float> dest) {
	const auto logCoef = std::log10(2.0f); // == log(2) / log(10)

	for (auto& transform : transforms) {
		switch (transform.type) {
		case TransformType::eDB: {
			auto sourceIter = source.begin();
			auto destIter = dest.begin();

			for (; sourceIter != source.end();
			       ++sourceIter, ++destIter) {
				float value = *sourceIter;
				float& destValue = *destIter;

				if (value <= 0.0f) {
					destValue = -std::numeric_limits<float>::infinity();
				} else {
					value = std::max(value, 0.0f);
					value = 10.0f * MathBitTwiddling::fastLog2(value) * logCoef;
					// value = 10.0 * std::log10(value);
					destValue = value;
				}
			}

			break;
		}
		case TransformType::eMAP: {
			auto sourceIter = source.begin();
			auto destIter = dest.begin();

			for (; sourceIter != source.end();
			       ++sourceIter, ++destIter) {
				float value = *sourceIter;
				float& destValue = *destIter;

				value = transform.interpolator.toValue(value);

				destValue = value;
			}

			break;
		}
		case TransformType::eCLAMP: {
			auto sourceIter = source.begin();
			auto destIter = dest.begin();

			for (; sourceIter != source.end();
			       ++sourceIter, ++destIter) {
				float value = *sourceIter;
				float& destValue = *destIter;

				value = std::clamp(value, transform.args[0], transform.args[1]);

				destValue = value;
			}

			break;
		}
		}

		source = dest;
	}
}

CVT CVT::parse(sview transformDescription, OptionParser& parser, const Logger& cl) {
	std::vector<TransformationInfo> transforms;

	for (auto element : Option{ transformDescription }.asSequence(L'(', L')', L',', false, cl)) {
		auto logger = cl.context(L"{}: ", element.name.asString());
		auto argsMap = element.args.asMap(L',', L' ');

		transforms.emplace_back(parseTransformation(element.name, argsMap, parser, logger));

		if (auto unused = argsMap.getListOfUntouched();
			!unused.empty()) {
			logger.warning(L"unused options: {}", unused);
		}
	}

	return CustomizableValueTransformer{ transforms };
}

CVT::TransformationInfo CVT::parseTransformation(const Option& nameOpt, const OptionMap& params, OptionParser& parser, const Logger& cl) {
	const auto transformName = nameOpt.asIString();

	if (transformName == L"db") {
		return parseDb(params, parser, cl);
	} else if (transformName == L"map") {
		return parseMap(params, parser, cl);
	} else if (transformName == L"clamp") {
		return parseClamp(params, parser, cl);
	}

	cl.error(L"transform type is not recognized: {}", transformName);
	throw option_parsing::OptionParser::Exception{};
}

CVT::TransformationInfo CVT::parseDb(const OptionMap& params, OptionParser& parser, const Logger& cl) {
	TransformationInfo tr{};
	tr.type = TransformType::eDB;
	return tr;
}

CVT::TransformationInfo CVT::parseMap(const OptionMap& params, OptionParser& parser, const Logger& cl) {
	TransformationInfo tr{};

	tr.type = TransformType::eMAP;

	auto sourceRangeOpt = params.get(L"from");
	if (sourceRangeOpt.empty()) {
		cl.error(L"from: value is required");
		throw option_parsing::OptionParser::Exception{};
	}

	auto sourceRange = sourceRangeOpt.asList(L':');
	if (sourceRange.size() != 2) {
		cl.error(L"from: need 2 values with syntax: \"<value1> : <value2>\", but found {} values: {}", sourceRange.size(), sourceRangeOpt);
		throw option_parsing::OptionParser::Exception{};
	}

	float linMin = parser.parse(sourceRange.get(0), L"from: first").as<float>();
	float linMax = parser.parse(sourceRange.get(1), L"from: second").as<float>();

	if (std::abs(linMin - linMax) < std::numeric_limits<float>::epsilon()) {
		cl.error(L"from: values must be distinct, but {} and {} found", linMin, linMax);
		throw option_parsing::OptionParser::Exception{};
	}

	float valMin = 0.0;
	float valMax = 1.0;

	if (auto destRange = params.get(L"to");
		!destRange.empty()) {
		auto range = destRange.asList(L':');
		if (range.size() != 2) {
			cl.error(L"need 2 params for target range but {} found", range.size());
			cl.error(L"to: need 2 values with syntax: \"<value1> : <value2>\", but found {} values: {}", range.size(), destRange);
			throw option_parsing::OptionParser::Exception{};
		}
		valMin = parser.parse(range.get(0), L"to: first").valueOr(valMin);
		valMax = parser.parse(range.get(1), L"to: second").valueOr(valMax);
	}

	tr.interpolator.setParams(linMin, linMax, valMin, valMax);

	return tr;
}

CVT::TransformationInfo CVT::parseClamp(const OptionMap& params, OptionParser& parser, const Logger& cl) {
	TransformationInfo tr{};

	tr.type = TransformType::eCLAMP;

	float min;
	float max;

	auto rangeOption = params.get(L"to");
	if (rangeOption.empty()) {
		min = 0.0f;
		max = 1.0f;
	} else {
		auto range = rangeOption.asList(L':');
		if (range.size() != 2) {
			cl.error(L"to: need 2 values with syntax: \"<min> : <max>\", but found {} values: {}", range.size(), rangeOption);
			throw option_parsing::OptionParser::Exception{};
		}

		min = parser.parse(range.get(0), L"to: first").as<float>();
		max = parser.parse(range.get(1), L"to: second").as<float>();

		if (min >= max) {
			cl.error(L"to: must be min < max, but {} >= {} was found", min, max);
			throw option_parsing::OptionParser::Exception{};
		}
	}

	tr.args[0] = min;
	tr.args[1] = max;

	return tr;
}
