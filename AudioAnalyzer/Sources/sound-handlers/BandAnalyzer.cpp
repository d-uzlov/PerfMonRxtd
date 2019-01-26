﻿/*
 * Copyright (C) 2019 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include "BandAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <string_view>
#include "FftAnalyzer.h"
#include "FastMath.h"

#pragma warning(disable : 4458)
#pragma warning(disable : 26451)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)

using namespace std::string_literals;
using namespace std::literals::string_view_literals;

const std::vector<double>& rxaa::BandAnalyzer::GaussianCoefficientsManager::forSigma(double sigma) {
	int radius = static_cast<int>(std::lround(sigma * 3.0));
	if (radius < 1) {
		radius = 1;
	}
	auto &vec = gaussianBlurCoefficients[radius];
	if (!vec.empty()) {
		return vec;
	}

	vec = generateGaussianKernel(radius, sigma);
	return vec;
}

std::vector<double> rxaa::BandAnalyzer::GaussianCoefficientsManager::generateGaussianKernel(int radius, double sigma) {

	std::vector<double> kernel;
	kernel.resize(radius * 2ll + 1);

	double powerFactor = 1.0 / (2.0 * sigma * sigma);

	int r = -radius;
	double sum = 0.0;
	for (int i = 0; i < kernel.size(); i++) {
		double x = r;
		x *= x;
		const auto coef = std::exp(-x * powerFactor);
		kernel[i] = coef;
		sum += coef;
		r++;
	}
	const double sumInverse = 1.0 / sum;
	for (auto &c : kernel) {
		c *= sumInverse;
	}

	return kernel;
}

std::optional<rxaa::BandAnalyzer::Params> rxaa::BandAnalyzer::parseParams(const rxu::OptionParser::OptionMap& optionMap, rxu::Rainmeter::ContextLogger& cl, rxu::Rainmeter &rain) {
	Params params;
	params.fftId = optionMap.getCS(L"source"sv).asString();
	if (params.fftId.empty()) {
		cl.error(L"source not found");
		return std::nullopt;
	}

	auto freqListIndex = optionMap.get(L"freqList"sv).asString();
	if (freqListIndex.empty()) {
		cl.error(L"freqList not found");
		return std::nullopt;
	}

	rxu::OptionParser optionParser;
	const auto bounds = optionParser.asList(rain.readString(L"FreqList_"s += freqListIndex), L'|');
	rxu::Rainmeter::ContextLogger freqListLogger { rain.getLogger() };
	auto freqsOpt = parseFreqList(bounds, freqListLogger, rain);
	if (!freqsOpt.has_value()) {
		cl.error(L"freqList '{}' can't be parsed", freqListIndex);
		return std::nullopt;
	}
	params.bandFreqs = freqsOpt.value();
	if (params.bandFreqs.size() < 2) {
		cl.error(L"freqList '{}' must have >= 2 frequencies but only {} found", freqListIndex, params.bandFreqs.size());
		return std::nullopt;
	}

	params.minCascade = optionMap.get(L"cascadeMin"sv).asInt(0);
	params.maxCascade = optionMap.get(L"cascadeMax"sv).asInt(0);

	params.targetWeight = optionMap.get(L"targetWeight"sv).asFloat(1.5);
	params.minWeight = optionMap.get(L"minWeight"sv).asFloat(0.1);

	params.includeZero = optionMap.get(L"includeZero"sv).asBool(true);
	params.proportionalValues = optionMap.get(L"proportionalValues"sv).asBool(true);
	params.blurCascades = optionMap.get(L"blurCascades"sv).asBool(true);
	params.blurRadius = optionMap.get(L"blurRadius"sv).asFloat(1.0) * 0.25; // looks best at 0.25

	params.sensitivity = optionMap.get(L"sensitivity"sv).asFloat(35.0);
	params.offset = optionMap.get(L"offset"sv).asFloat(0.0);

	params.smoothingFactor = optionMap.get(L"smoothingFactor"sv).asInt(4);
	if (params.smoothingFactor <= 0) {
		cl.warning(L"smoothingFactor should be >= 1 but {} found, assume 1", params.smoothingFactor);
		params.smoothingFactor = 1;
	}

	auto smoothingCurveString = optionMap.getCS(L"smoothingCurve"sv).asString();
	params.exponentialFactor = 1;
	if (smoothingCurveString.empty() || smoothingCurveString == L"exponential"sv) {
		params.smoothingCurve = SmoothingCurve::EXPONENTIAL;
		params.exponentialFactor = optionMap.get(L"exponentialFactor"sv).asFloat(1.5);
	} else if (smoothingCurveString == L"flat"sv) {
		params.smoothingCurve = SmoothingCurve::FLAT;
	} else if (smoothingCurveString == L"linear"sv) {
		params.smoothingCurve = SmoothingCurve::LINEAR;
	} else {
		cl.warning(L"smoothingCurve '{}' now recognized, assume 'flat'", smoothingCurveString);
		params.smoothingCurve = SmoothingCurve::FLAT;
	}

	params.minFunction = optionMap.get(L"mixFunction").asString(L"product") == L"product"sv ? MixFunction::PRODUCT : MixFunction::AVERAGE;

	return params;
}

void rxaa::BandAnalyzer::setParams(Params params) {
	this->params = params;

	source = nullptr;

	if (params.bandFreqs.size() < 2u) {
		return;
	}

	const auto bandsCount = params.bandFreqs.size() - 1;

	bandFreqMultipliers.resize(bandsCount);
	double multipliersSum { };
	for (int i = 0; i < bandsCount; ++i) {
		bandFreqMultipliers[i] = std::log(params.bandFreqs[i + 1] - params.bandFreqs[i] + 1.0) / std::log(50.0);
		// bandFreqMultipliers[i] = params.bandFreqs[i + 1] - params.bandFreqs[i];
		multipliersSum += bandFreqMultipliers[i];
	}
	const double bandFreqMultipliersAverage = multipliersSum / bandsCount;
	const double multiplierCorrectingConstant = 1.0 / (bandFreqMultipliersAverage);
	for (double& multiplier : bandFreqMultipliers) {
		multiplier *= multiplierCorrectingConstant;
	}

	logNormalization = 20.0 / std::max(params.sensitivity, 0.1);

	values.resize(bandsCount);
	pastValues.resize(params.smoothingFactor);
	for (auto &v : pastValues) {
		v.resize(bandsCount);
	}

	analysisComputed = false;
}

void rxaa::BandAnalyzer::process(const DataSupplier& dataSupplier) {
	if (params.bandFreqs.size() < 2) {
		return;
	}

	source = dynamic_cast<const FftAnalyzer*>(dataSupplier.getHandler(params.fftId));
	next = true;
}

void rxaa::BandAnalyzer::processSilence(const DataSupplier& dataSupplier) {
	process(dataSupplier);
}

const double* rxaa::BandAnalyzer::getData() const {
	if (next) {
		updateValues();
		next = false;
	}
	return values.data();
}

size_t rxaa::BandAnalyzer::getCount() const {
	if (params.bandFreqs.size() < 2) {
		return 0;
	}
	return params.bandFreqs.size() - 1;
}

void rxaa::BandAnalyzer::setSamplesPerSec(uint32_t samplesPerSec) {
	this->samplesPerSec = samplesPerSec;
}

const wchar_t* rxaa::BandAnalyzer::getProp(const std::wstring_view& prop) {
	propString.clear();

	const auto bandsCount = params.bandFreqs.size() - 1;

	if (prop == L"bands count"sv) {
		propString = std::to_wstring(bandsCount);
	} else if (prop == L"cascade analysis"sv) {
		return analysis.analysisString.c_str();
	} else if (prop == L"min cascade used"sv) {
		propString = std::to_wstring(analysis.minCascadeUsed);
	} else if (prop == L"max cascade used"sv) {
		propString = std::to_wstring(analysis.maxCascadeUsed);
	} else {
		auto index = parseIndexProp(prop, L"lower bound"sv, bandsCount + 1);
		if (index == -2) {
			return L"0";
		}
		if (index >= 0) {
			if (index > 0) {
				index--;
			}
			propString = std::to_wstring(params.bandFreqs[index]);
			return propString.c_str();
		}

		index = parseIndexProp(prop, L"upper bound"sv, bandsCount + 1);
		if (index == -2) {
			return L"0";
		}
		if (index >= 0) {
			if (index > 0) {
				index--;
			}
			propString = std::to_wstring(params.bandFreqs[index + 1]);
			return propString.c_str();
		}

		index = parseIndexProp(prop, L"central frequency"sv, bandsCount + 1);
		if (index == -2) {
			return L"0";
		}
		if (index >= 0) {
			if (index > 0) {
				index--;
			}
			propString = std::to_wstring((params.bandFreqs[index] + params.bandFreqs[index + 1]) * 0.5);
			return propString.c_str();
		}

		return nullptr;
	}

	return propString.c_str();
}

void rxaa::BandAnalyzer::reset() {
	next = true;
}


void rxaa::BandAnalyzer::updateValues() const {
	if (params.bandFreqs.size() < 2u || source == nullptr) {
		return;
	}

	const auto fftBinsCount = source->getCount();
	const auto cascadesCount = source->getCascadesCount();

	const auto bandsCount = params.bandFreqs.size() - 1;

	unsigned cascadeIndexBegin = 1u;
	unsigned cascadeIndexEnd = cascadesCount + 1;
	if (analysisComputed) {
		cascadeIndexBegin = analysis.minCascadeUsed;
		cascadeIndexEnd = analysis.maxCascadeUsed + 1;
	} else {
		if (params.minCascade > 0) {
			if (cascadesCount >= static_cast<decltype(cascadesCount)>(params.minCascade)) {
				cascadeIndexBegin = params.minCascade;

				if (params.maxCascade >= params.minCascade && cascadesCount >= static_cast<decltype(cascadesCount)>(params.maxCascade)) {
					cascadeIndexEnd = params.maxCascade + 1;
				}
			} else {
				return;
			}
		}
	}
	cascadeIndexBegin--;
	cascadeIndexEnd--;

	bandInfo.resize(cascadeIndexEnd - cascadeIndexBegin);
	for (auto &vec : bandInfo) {
		vec.resize(bandsCount);
	}

	double binWidth = static_cast<double>(samplesPerSec) / (source->getFftSize() * std::pow(2u, cascadeIndexBegin));

	for (auto cascade = cascadeIndexBegin; cascade < cascadeIndexEnd; ++cascade) {
		unsigned binIndex = params.includeZero ? 0 : 1; // bin 0 is ~DC
		unsigned bandIndex = 0;

		double bandMinFreq = params.bandFreqs[0];
		double bandMaxFreq = params.bandFreqs[1];

		const auto fftData = source->getCascade(cascade);

		auto &cascadeBandInfo = bandInfo[cascade - cascadeIndexBegin];

		while (binIndex < fftBinsCount && bandIndex < bandsCount) {
			const double binUpperFreq = (binIndex + 0.5) * binWidth;
			if (binUpperFreq < bandMinFreq) {
				binIndex++;
				continue;
			}

			double weight = 1.0;
			const double binLowerFreq = (binIndex - 0.5) * binWidth;

			if (binLowerFreq < bandMinFreq) {
				weight -= (bandMinFreq - binLowerFreq) / binWidth;
			}
			if (bandMaxFreq < binUpperFreq) {
				weight -= (binUpperFreq - bandMaxFreq) / binWidth;
			}
			if (weight > 0) {
				const auto fftValue = fftData[binIndex];
				cascadeBandInfo[bandIndex].magnitude += fftValue * weight;
				cascadeBandInfo[bandIndex].weight += weight;
			}

			if (bandMaxFreq >= binUpperFreq) {
				binIndex++;
			} else {
				bandIndex++;
				if (bandIndex >= bandsCount) {
					break;
				}
				bandMinFreq = bandMaxFreq;
				bandMaxFreq = params.bandFreqs[bandIndex + 1];
			}
		}
		binWidth *= 0.5;
	}


	computeAnalysis(cascadeIndexBegin + 1, cascadeIndexEnd + 1);

	if (params.blurCascades) {
		cascadeTempBuffer.resize(bandsCount);
		for (auto cascade = cascadeIndexBegin; cascade < cascadeIndexEnd; ++cascade) {
			auto &cascadeBandInfo = bandInfo[cascade - cascadeIndexBegin];

			for (int band = 0; band < bandsCount; ++band) {
				double sigma = cascadeBandInfo[band].blurSigma;
				if (sigma == 0.0) {
					cascadeTempBuffer[band] = cascadeBandInfo[band].magnitude;
					continue;
				}
				auto &kernel = gcm.forSigma(sigma);
				int radius = kernel.size() >> 1;
				int bandStartIndex = band - radius;
				int kernelStartIndex = 0;
				if (bandStartIndex < 0) {
					kernelStartIndex = -bandStartIndex;
					bandStartIndex = 0;
				}
				double result = 0.0;
				int kernelIndex = kernelStartIndex;
				int bandIndex = bandStartIndex;
				while (true) {
					if (bandIndex >= static_cast<int>(bandsCount) || kernelIndex >= static_cast<int>(kernel.size())) {
						break;
					}
					result += kernel[kernelIndex] * cascadeBandInfo[bandIndex].magnitude;

					kernelIndex++;
					bandIndex++;
				}
				cascadeTempBuffer[band] = result;
			}
			for (int i = 0; i < bandsCount; ++i) {
				cascadeBandInfo[i].magnitude = cascadeTempBuffer[i];
			}
		}
	}


	constexpr double log10inverse = 0.30102999566398119521; // 1.0 / log2(10)

	pastValuesIndex++;
	if (pastValuesIndex >= params.smoothingFactor) {
		pastValuesIndex = 0;
	}

	for (auto band = 0llu; band < values.size(); ++band) {
		double weight = 0.0;
		unsigned cascadesSummed = 0u;

		double value;
		if (params.minFunction == MixFunction::PRODUCT) {
			value = 1.0;
		} else {
			value = 0.0;
		}

		for (auto cascade = cascadeIndexBegin; cascade < cascadeIndexEnd; ++cascade) {
			auto &info = bandInfo[cascade - cascadeIndexBegin][band];


			if (info.weight >= params.minWeight) {
				auto cascadeBandValue = info.magnitude;
				cascadeBandValue /= info.weight;

				if (params.minFunction == MixFunction::PRODUCT) {
					value *= cascadeBandValue;
				} else {
					value += cascadeBandValue;
				}

				weight += info.weight;
				cascadesSummed++;
			}

			info.reset();

			if (weight >= params.targetWeight) {
				break;
			}
		}

		if (cascadesSummed > 0) {
			if (params.minFunction == MixFunction::PRODUCT) {
				value = rxu::FastMath::pow(value, 1.0 / cascadesSummed);
			} else {
				value /= cascadesSummed;
			}
		}
		if (params.proportionalValues) {
			value *= bandFreqMultipliers[band];
		}

		pastValues[pastValuesIndex][band] = value;
	}

	if (params.smoothingFactor <= 1) {
		values = pastValues[0];
	} else {
		auto startPastIndex = pastValuesIndex + 1;
		if (startPastIndex >= params.smoothingFactor) {
			startPastIndex = 0;
		}
		switch (params.smoothingCurve) {
		case SmoothingCurve::FLAT:
		{
			for (unsigned band = 0; band < values.size(); ++band) {
				double outValue = 0.0;
				for (unsigned i = 0; i < params.smoothingFactor; ++i) {
					outValue += pastValues[i][band];
				}
				outValue /= params.smoothingFactor;
				values[band] = outValue;
			}
			break;
		}
		case SmoothingCurve::LINEAR:
		{
			for (unsigned band = 0; band < values.size(); ++band) {
				double outValue = 0.0;
				int smoothingWeight = 0;
				double valueWeight = 1;

				for (unsigned i = startPastIndex; i < params.smoothingFactor; ++i) {
					outValue += pastValues[i][band] * valueWeight;
					smoothingWeight += valueWeight;
					valueWeight++;
				}
				for (unsigned i = 0; i < startPastIndex; ++i) {
					outValue += pastValues[i][band] * valueWeight;
					smoothingWeight += valueWeight;
					valueWeight++;
				}

				outValue /= smoothingWeight;
				values[band] = outValue;
			}
			break;
		}
		case SmoothingCurve::EXPONENTIAL:
		{
			for (unsigned band = 0; band < values.size(); ++band) {
				double outValue = 0.0;
				int smoothingWeight = 0;
				double weight = 1;

				for (unsigned i = startPastIndex; i < params.smoothingFactor; ++i) {
					outValue += pastValues[i][band] * weight;
					smoothingWeight += weight;
					weight *= params.exponentialFactor;
				}
				for (unsigned i = 0; i < startPastIndex; ++i) {
					outValue += pastValues[i][band] * weight;
					smoothingWeight += weight;
					weight *= params.exponentialFactor;
				}

				outValue /= smoothingWeight;
				values[band] = outValue;
			}
			break;
		}
		default: std::terminate(); // should be unreachable statement
		}
	}

	for (unsigned i = 0; i < bandsCount; ++i) {
		double value = values[i];

		value = rxu::FastMath::log2(value) * log10inverse;
		value = value * logNormalization + 1.0;
		value += params.offset;
		values[i] = value;
	}
}

void rxaa::BandAnalyzer::computeAnalysis(unsigned startCascade, unsigned endCascade) const {
	if (analysisComputed) {
		return;
	}

	std::wostringstream out;

	out.precision(1);
	out << std::fixed;

	analysis.minCascadeUsed = -1;
	analysis.maxCascadeUsed = -1;

	for (auto band = 0llu; band < values.size(); ++band) {
		double weight = 0.0;
		int bandStartCascade = -1;
		int bandEndCascade = -1;

		for (auto cascade = startCascade; cascade < endCascade; ++cascade) {
			auto &info = bandInfo[cascade - startCascade][band];

			if (info.weight >= 1.0) {
				info.blurSigma = 0.0;
			} else if (info.weight >= std::numeric_limits<double>::epsilon()) {
				info.blurSigma = 1.0 / info.weight * params.blurRadius;
			} else {
				info.blurSigma = 0.0;
			}

			if (info.weight >= params.minWeight) {
				weight += info.weight;
				if (bandStartCascade < 0) {
					bandStartCascade = cascade;
				}
				bandEndCascade = cascade;

				if (analysis.minCascadeUsed < 0) {
					analysis.minCascadeUsed = cascade;
				} else {
					analysis.minCascadeUsed = std::min(static_cast<unsigned>(analysis.minCascadeUsed), cascade);
				}
				if (analysis.maxCascadeUsed < 0) {
					analysis.maxCascadeUsed = cascade;
				} else {
					analysis.maxCascadeUsed = std::max(static_cast<unsigned>(analysis.maxCascadeUsed), cascade);
				}
			}

			if (weight >= params.targetWeight) {
				break;
			}
		}

		if (bandEndCascade < 0) {
			bandEndCascade = endCascade - 1;
		}

		out << band;
		out << L":";
		out << weight;
		out << L":";
		out << bandStartCascade;
		out << L"-";
		out << bandEndCascade;
		out << L" ";
	}

	analysis.analysisString = out.str();

	analysisComputed = true;
}

std::optional<std::vector<double>> rxaa::BandAnalyzer::parseFreqList(rxu::OptionParser::OptionList bounds, rxu::Rainmeter::ContextLogger& cl, const rxu::Rainmeter &rain) {
	std::vector<double> freqs;

	rxu::OptionParser optionParser;
	for (auto bound : bounds) {
		if (bound.find(L"linear "sv) == 0 || bound.find(L"log "sv) == 0) {

			auto options = optionParser.asList(bound, L' ');
			if (options.size() != 4) {
				cl.error(L"linear/log must have 3 options (count, min, max)");
				return std::nullopt;
			}
			int count;
			std::wstringstream(std::wstring { options.get(1) }) >> count;

			if (count < 1) {
				cl.error(L"count must be >= 1");
				return std::nullopt;
			}

			const double min = options.getOption(2).asFloat();
			const double max = options.getOption(3).asFloat();
			if (min >= max) {
				cl.error(L"min must be < max");
				return std::nullopt;
			}

			if (bound.find(L"linear "sv) == 0) {
				const double delta = max - min;

				for (int i = 0; i <= count; ++i) {
					freqs.push_back(min + delta * i / count);
				}
			} else {
				// log
				const auto step = std::pow(2.0, std::log2(max / min) / count);
				double freq = min;
				freqs.push_back(freq);

				for (auto i = 0ll; i < count; ++i) {
					freq *= step;
					freqs.push_back(freq);
				}
			}
			continue;
		}
		if (bound.find(L"custom "sv) == 0) {
			auto options = optionParser.asList(bound, L' ');
			if (options.size() < 2) {
				cl.error(L"custom must have at least two frequencies specified but {} found", options.size());
				return std::nullopt;
			}
			for (auto i = 1u; i < options.size(); ++i) {
				freqs.push_back(options.getOption(i).asFloat());
			}
		}

		cl.error(L"unknown list type '{}'", bound);
		return std::nullopt;
	}

	std::sort(freqs.begin(), freqs.end());

	const double threshold = rain.readDouble(L"FreqSimThreshold", 0.07); // 0.07 is a random constant that I feel appropriate

	std::vector<double> result;
	result.reserve(freqs.size());
	double lastValue = -1;
	for (auto value : freqs) {
		if (value <= 0) {
			cl.error(L"frequency must be > 0 ({} found)", value);
			return std::nullopt;
		}
		if (std::abs(value - lastValue) < threshold) {
			lastValue = value;
			continue;
		}
		result.push_back(value);
		lastValue = value;
	}

	return result;
}
