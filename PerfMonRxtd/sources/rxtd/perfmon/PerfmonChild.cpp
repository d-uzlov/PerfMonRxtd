// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2018 buckb
// Copyright (C) 2018 Danil Uzlov

#include "PerfmonChild.h"

using rxtd::perfmon::PerfmonChild;
using rxtd::std_fixes::StringUtils;

PerfmonChild::PerfmonChild(Rainmeter&& _rain) : MeasureBase(std::move(_rain)) {
	parent = dynamic_cast<PerfmonParent*>(findParent());
	if (parent == nullptr) {
		logger.error(L"Invalid parent specified");
		throw std::runtime_error{ "" };
	}

	parser.setLogger(logger);
}

void PerfmonChild::vReload() {
	instanceIndex = parser.parse(rain.read(L"InstanceIndex"), L"InstanceIndex").valueOr(0);
	ref.counter = parser.parse(rain.read(L"CounterIndex"), L"CounterIndex").valueOr(0);
	ref.useOrigName = parser.parse(rain.read(L"SearchOriginalName"), L"SearchOriginalName").valueOr(false);
	ref.total = parser.parse(rain.read(L"Total"), L"Total").valueOr(false);
	ref.discarded = parser.parse(rain.read(L"Discarded"), L"Discarded").valueOr(false);


	instanceName = rain.read(L"InstanceName").asString();
	if (!ref.useOrigName) {
		StringUtils::makeUppercaseInPlace(instanceName);
	}
	ref.namePattern = MatchPattern{ instanceName };

	bool needReadRollupFunction = true;
	bool forceUseName = false;
	const auto type = rain.read(L"Type").asIString();
	if (type == L"GetInstanceCount") {
		logger.warning(L"Type 'GetInstanceCount' is deprecated, set to 'GetCount' with Total=1 and RollupFunction=Sum");
		ref.type = Reference::Type::eCOUNT;
		ref.total = true;
		ref.rollupFunction = RollupFunction::eSUM;
		needReadRollupFunction = false;
	} else if (type == L"GetCount")
		ref.type = Reference::Type::eCOUNT;
	else if (type == L"GetInstanceName") {
		logger.warning(L"Type 'GetInstanceName' is deprecated, set to 'GetCount' with Total=0");
		ref.type = Reference::Type::eCOUNT;
		ref.total = false;
		ref.rollupFunction = RollupFunction::eFIRST;
		resultStringType = ResultString::eDISPLAY_NAME;
		forceUseName = true;
	} else if (type == L"GetRawCounter") {
		ref.type = Reference::Type::eCOUNTER_RAW;
	} else if (type == L"GetFormattedCounter") {
		ref.type = Reference::Type::eCOUNTER_FORMATTED;
	} else if (type == L"GetExpression") {
		ref.type = Reference::Type::eEXPRESSION;
	} else if (type == L"GetRollupExpression") {
		ref.type = Reference::Type::eROLLUP_EXPRESSION;
	} else {
		logger.error(L"Type '{}' is invalid for child measure", type);
		setInvalid();
		return;
	}

	if (needReadRollupFunction) {
		auto rollupFunctionStr = rain.read(L"RollupFunction").asIString(L"Sum");
		if (rollupFunctionStr == L"Count") {
			logger.warning(L"RollupFunction 'Count' is deprecated, measure type set to 'GetCount'");
			ref.type = Reference::Type::eCOUNT;
		} else {
			auto typeOpt = parseEnum<RollupFunction>(rollupFunctionStr);
			if (typeOpt.has_value()) {
				ref.rollupFunction = typeOpt.value();
			} else {
				logger.error(L"RollupFunction '{}' is invalid, set to 'Sum'", rollupFunctionStr);
				ref.rollupFunction = RollupFunction::eSUM;
			}
		}
	}

	const auto resultStringStr = rain.read(L"ResultString").asIString(forceUseName ? L"DisplayName" : L"Number");
	if (resultStringStr == L"Number") {
		resultStringType = ResultString::eNUMBER;
	} else if (resultStringStr == L"OriginalInstanceName") {
		resultStringType = ResultString::eORIGINAL_NAME;
	} else if (resultStringStr == L"UniqueInstanceName") {
		resultStringType = ResultString::eUNIQUE_NAME;
	} else if (resultStringStr == L"DisplayInstanceName") {
		resultStringType = ResultString::eDISPLAY_NAME;
	} else if (resultStringStr == L"RollupInstanceName") {
		logger.warning(L"ResultString 'RollupInstanceName' is deprecated, set to 'DisplayName'");
		resultStringType = ResultString::eDISPLAY_NAME;
	} else {
		auto typeOpt = parseEnum<ResultString>(resultStringStr);
		if (typeOpt.has_value()) {
			resultStringType = typeOpt.value();
		} else {
			logger.error(L"ResultString '{}' is invalid, set to 'Number'", resultStringStr);
			resultStringType = ResultString::eNUMBER;
		}
	}

	setUseResultString(resultStringType != ResultString::eNUMBER);
}

double PerfmonChild::vUpdate() {
	return parent->getValues(ref, instanceIndex, resultStringType, stringValue);
}

void PerfmonChild::vUpdateString(string& resultStringBuffer) {
	resultStringBuffer = stringValue;
}
