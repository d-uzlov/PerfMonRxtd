// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Danil Uzlov

#include "BQFilterBuilder.h"
#include "rxtd/std_fixes/MyMath.h"

using rxtd::filter_utils::BQFilterBuilder;
using rxtd::filter_utils::BiQuadIIR;
using rxtd::std_fixes::MyMath;

// Based on formulas from Audio-EQ-Cookbook

BiQuadIIR BQFilterBuilder::createHighShelf(double samplingFrequency, double q, double centralFrequency, double dbGain) {
	if (samplingFrequency == 0.0 || q <= 0.0) {
		return {};
	}

	double digitalFreq = 2.0 * centralFrequency / samplingFrequency;
	digitalFreq = std::clamp(digitalFreq, 0.01, 1.0 - 0.01);
	const double w0 = MyMath::pi<double>() * digitalFreq;

	const double a = std::pow(10, dbGain / 40);
	const double alpha = std::sin(w0) / (2 * q);

	return {
		(a + 1) - (a - 1) * std::cos(w0) + 2 * std::sqrt(a) * alpha,
		2 * ((a - 1) - (a + 1) * std::cos(w0)),
		(a + 1) - (a - 1) * std::cos(w0) - 2 * std::sqrt(a) * alpha,
		a * ((a + 1) + (a - 1) * std::cos(w0) + 2 * std::sqrt(a) * alpha),
		-2 * a * ((a - 1) + (a + 1) * std::cos(w0)),
		a * ((a + 1) + (a - 1) * std::cos(w0) - 2 * std::sqrt(a) * alpha),
	};
}

BiQuadIIR BQFilterBuilder::createLowShelf(double samplingFrequency, double q, double centralFrequency, double dbGain) {
	if (samplingFrequency == 0.0 || q <= 0.0) {
		return {};
	}

	double digitalFreq = 2.0 * centralFrequency / samplingFrequency;
	digitalFreq = std::clamp(digitalFreq, 0.01, 1.0 - 0.01);
	const double w0 = MyMath::pi<double>() * digitalFreq;

	const double a = std::pow(10, dbGain / 40);
	const double alpha = std::sin(w0) / (2 * q);

	return {
		(a + 1) + (a - 1) * std::cos(w0) + 2 * std::sqrt(a) * alpha,
		-2 * ((a - 1) + (a + 1) * std::cos(w0)),
		(a + 1) + (a - 1) * std::cos(w0) - 2 * std::sqrt(a) * alpha,
		a * ((a + 1) - (a - 1) * std::cos(w0) + 2 * std::sqrt(a) * alpha),
		2 * a * ((a - 1) - (a + 1) * std::cos(w0)),
		a * ((a + 1) - (a - 1) * std::cos(w0) - 2 * std::sqrt(a) * alpha),
	};
}

BiQuadIIR BQFilterBuilder::createHighPass(double samplingFrequency, double q, double centralFrequency) {
	if (samplingFrequency == 0.0 || q <= 0.0) {
		return {};
	}

	double digitalFreq = 2.0 * centralFrequency / samplingFrequency;
	digitalFreq = std::clamp(digitalFreq, 0.01, 1.0 - 0.01);
	const double w0 = MyMath::pi<double>() * digitalFreq;

	const double alpha = std::sin(w0) / (2 * q);

	return {
		1 + alpha,
		-2 * std::cos(w0),
		1 - alpha,
		(1 + std::cos(w0)) / 2,
		-(1 + std::cos(w0)),
		(1 + std::cos(w0)) / 2,
	};
}

BiQuadIIR BQFilterBuilder::createLowPass(double samplingFrequency, double q, double centralFrequency) {
	if (samplingFrequency == 0.0 || q <= 0.0) {
		return {};
	}

	double digitalFreq = 2.0 * centralFrequency / samplingFrequency;
	digitalFreq = std::clamp(digitalFreq, 0.01, 1.0 - 0.01);
	const double w0 = MyMath::pi<double>() * digitalFreq;

	const double alpha = std::sin(w0) / (2 * q);

	return {
		1 + alpha,
		-2 * std::cos(w0),
		1 - alpha,
		(1 + std::cos(w0)) / 2,
		(1 + std::cos(w0)),
		(1 + std::cos(w0)) / 2,
	};
}

BiQuadIIR BQFilterBuilder::createPeak(double samplingFrequency, double q, double centralFrequency, double dbGain) {
	if (samplingFrequency == 0.0 || q <= 0.0) {
		return {};
	}

	double digitalFreq = 2.0 * centralFrequency / samplingFrequency;
	digitalFreq = std::clamp(digitalFreq, 0.01, 1.0 - 0.01);
	const double w0 = MyMath::pi<double>() * digitalFreq;

	const double a = std::pow(10, dbGain / 40);
	const double alpha = std::sin(w0) / (2 * q);

	return {
		1 + alpha / a,
		-2 * std::cos(w0),
		1 - alpha / a,
		1 + alpha * a,
		-2 * std::cos(w0),
		1 - alpha * a,
	};
}
