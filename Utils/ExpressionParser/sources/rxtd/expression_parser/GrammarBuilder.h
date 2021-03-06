﻿// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2021 Danil Uzlov

#pragma once

#include "GrammarDescription.h"

namespace rxtd::expression_parser {
	/// <summary>
	/// GrammarBuilder is a helper class for ASTParser class.
	/// It provides easy grammar creation.
	/// </summary>
	class GrammarBuilder {
	public:
		using OperatorInfo = GrammarDescription::OperatorInfo;
		using PrecedenceType = OperatorInfo::PrecedenceType;
		using SolveFunction = GrammarDescription::MainOperatorInfo::SolveFunction;

		class OperatorRepeatException : public std::runtime_error {
			sview reason;
		public:
			explicit OperatorRepeatException(sview reason) : runtime_error(""), reason(reason) {}

			[[nodiscard]]
			auto getReason() const {
				return reason;
			}
		};

	private:
		GrammarDescription grammar;
		PrecedenceType currentPrecedence = 0;

		std::set<sview> prefixOperators;
		std::set<sview> postfixOperators;
		std::set<sview> binaryOperators;
		std::set<sview> groupingOperatorsFirst;
		std::set<sview> groupingOperatorsSecond;

	public:
		/// <summary>
		/// Makes a simple grammar with unary +, -, binary +, -, *, /, ^
		/// </summary>
		static GrammarDescription makeSimpleMath();

		/// <summary>
		/// Increase precedence lever by 1.
		/// </summary>
		void increasePrecedence() {
			setPrecedence(currentPrecedence + 1);
		}

		/// <summary>
		/// Manually set a precedence level.
		/// If you are creating your grammar step by step
		/// then you are advised to use #increasePrecedence() instead.
		/// </summary>
		void setPrecedence(PrecedenceType value) {
			currentPrecedence = value;
		}

		void pushBinary(sview opValue, SolveFunction solveFunc, bool leftToRightAssociativity = true);

		void pushPrefix(sview opValue, SolveFunction solveFunc, bool leftToRightAssociativity = true);

		void pushPostfix(sview opValue, SolveFunction solveFunc, bool leftToRightAssociativity = true);

		void pushGrouping(sview first, sview second, sview separator);

		[[nodiscard]]
		const GrammarDescription& peekResult() const {
			return grammar;
		}

		[[nodiscard]]
		GrammarDescription takeResult() && {
			return std::exchange(grammar, {});
		}

	private:
		void push(sview opValue, OperatorInfo::Type type, SolveFunction func, bool leftToRightAssociativity);
	};
}
