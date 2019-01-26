/* 
 * Copyright (C) 2018-2019 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include <cwctype>
#include <algorithm>

#include "expressions.h"

#include <string>
#include <string_view>
#include "StringUtils.h"

#pragma warning(disable : 4458)
#pragma warning(disable : 4244)

using namespace std::string_literals;
using namespace std::literals::string_view_literals;

void rxpm::ExpressionTreeNode::simplify() {
	if (type == ExpressionType::NUMBER ||
		type == ExpressionType::REF) {
		return;
	}

	bool isConst = true;
	for (ExpressionTreeNode& node : nodes) {
		node.simplify();
		if (node.type != ExpressionType::NUMBER) {
			isConst = false;
		}
	}
	if (!isConst) {
		return;
	}

	switch (type) {
	case ExpressionType::SUM:
	{
		double value = 0;
		for (ExpressionTreeNode& node : nodes) {
			value += node.number;
		}
		nodes.clear();
		number = value;
		type = ExpressionType::NUMBER;
		return;
	}
	case ExpressionType::DIFF:
	{
		double value = nodes[0].number;
		for (unsigned int i = 1; i < nodes.size(); i++) {
			value -= nodes[i].number;
		}
		nodes.clear();
		number = value;
		type = ExpressionType::NUMBER;
		return;
	}
	case ExpressionType::INVERSE:
	{
		number = -nodes[0].number;
		nodes.clear();
		type = ExpressionType::NUMBER;
		return;
	}
	case ExpressionType::MULT:
	{
		double value = 1;
		for (ExpressionTreeNode& node : nodes) {
			value *= node.number;
		}
		nodes.clear();
		number = value;
		type = ExpressionType::NUMBER;
		return;
	}
	case ExpressionType::DIV:
	{
		double value = nodes[0].number;
		for (unsigned int i = 1; i < nodes.size(); i++) {
			const double nodeValue = nodes[i].number;
			if (nodeValue == 0) {
				value = 0.0;
				break;
			}
			value /= nodeValue;
		}
		nodes.clear();
		number = value;
		type = ExpressionType::NUMBER;
		return;
	}
	case ExpressionType::POWER:
	{
		double value = nodes[0].number;
		value = std::pow(value, nodes[1].number);
		nodes.clear();
		number = value;
		type = ExpressionType::NUMBER;
		return;
	}
	default:;
	}
}
int rxpm::ExpressionTreeNode::maxExpRef() const {
	int max = -1;
	if (type == ExpressionType::REF && ref.type == ReferenceType::EXPRESSION) {
		max = ref.counter;
	} else {
		for (const ExpressionTreeNode& node : nodes) {
			max = std::max(max, node.maxExpRef());
		}
	}
	return max;
}

int rxpm::ExpressionTreeNode::maxRUERef() const {
	int max = -1;
	if (type == ExpressionType::REF && ref.type == ReferenceType::ROLLUP_EXPRESSION) {
		max = ref.counter;
	} else {
		for (const ExpressionTreeNode& node : nodes) {
			max = std::max(max, node.maxRUERef());
		}
	}
	return max;
}

void rxpm::ExpressionTreeNode::processRefs(void(*handler)(Reference&)) {
	if (type == ExpressionType::REF) {
		handler(ref);
	} else {
		for (auto& node : nodes) {
			node.processRefs(handler);
		}
	}
}

rxpm::ExpressionParser::Lexer::Lexer(std::wstring_view source) : source(source) {

}

rxpm::ExpressionParser::Lexer::Lexeme rxpm::ExpressionParser::Lexer::next() {
	skipSpaces();

	if (position >= source.length()) {
		return Lexeme(LexemeType::END, { });
	}

	if (isSymbol(source[position])) {
		const wchar_t value = source[position];
		LexemeType type = LexemeType::UNKNOWN;
		position++;
		switch (value) {
		case L'(':
			type = LexemeType::PAR_OPEN;
			break;
		case L')':
			type = LexemeType::PAR_CLOSE;
			break;
		case L'[':
			type = LexemeType::BR_OPEN;
			break;
		case L']':
			type = LexemeType::BR_CLOSE;
			break;
		case L'+':
			type = LexemeType::PLUS;
			break;
		case L'-':
			type = LexemeType::MINUS;
			break;
		case L'*':
			type = LexemeType::MULT;
			break;
		case L'/':
			type = LexemeType::DIV;
			break;
		case L'^':
			type = LexemeType::POWER;
			break;
		case L'\0':
			type = LexemeType::END;
			break;
		case L'.':
			type = LexemeType::DOT;
			break;
		case L'#':
			type = LexemeType::HASH;
			break;
		default:
			type = LexemeType::UNKNOWN;
			break;
		}
		return Lexeme(type, std::to_wstring(value));
	}

	if (std::iswdigit(source[position])) {
		return Lexeme(LexemeType::NUMBER, readNumber());
	}

	if (std::iswalpha(source[position])) {
		return Lexeme(LexemeType::WORD, readWord());
	}

	Lexeme result(LexemeType::UNKNOWN, std::to_wstring(source[position]));
	position++;
	return result;
}

std::wstring_view rxpm::ExpressionParser::Lexer::getUntil(const wchar_t stop1, const wchar_t stop2) {
	const auto startPos = position;
	int i = 0;
	while (position + i < source.length()) {
		const wchar_t c1 = source[position + i];
		if (c1 != stop1 && c1 != stop2 && c1 != L'\0') {
			i++;
		} else {
			break;
		}
	}
	position += i;
	return source.substr(startPos, i);
}

void rxpm::ExpressionParser::Lexer::skipSpaces() {
	while (position < source.length() && std::iswspace(source[position])) {
		position++;
	}
}

bool rxpm::ExpressionParser::Lexer::isSymbol(const wchar_t c) {
	return
		c == L'\0' ||
		c == L'(' ||
		c == L')' ||
		c == L'+' ||
		c == L'-' ||
		c == L'*' ||
		c == L'/' ||
		c == L'^' ||
		c == L'[' ||
		c == L']' ||
		c == L'.' ||
		c == L'#';
}

std::wstring_view rxpm::ExpressionParser::Lexer::readWord() {
	const auto startPos = position;
	int i = 0;
	while (position + i < source.length()) {
		const wchar_t c1 = source[position + i];
		if (std::iswalpha(c1)) {
			i++;
		} else {
			break;
		}
	}
	position += i;
	return source.substr(startPos, i);
}
std::wstring_view rxpm::ExpressionParser::Lexer::readNumber() {
	const auto startPos = position;
	int i = 0;
	while (position + i < source.length()) {
		const wchar_t c1 = source[position + i];
		if (std::iswdigit(c1)) {
			i++;
		} else {
			break;
		}
	}
	position += i;
	return source.substr(startPos, i);
}

rxpm::ExpressionParser::ExpressionParser(std::wstring source) : source(std::move(source)), lexer(this->source) {
	readNext();
}

rxpm::ExpressionParser::ExpressionParser(std::wstring_view source) : lexer(source) {
	readNext();
}

void rxpm::ExpressionParser::parse() {
	result = parseExpression();
	if (next.type != Lexer::LexemeType::END) {
		error = true;
	}
}
bool rxpm::ExpressionParser::isError() const {
	return error;
}
rxpm::ExpressionTreeNode rxpm::ExpressionParser::getExpression() const {
	return result;
}
void rxpm::ExpressionParser::readNext() {
	next = lexer.next();
	if (next.type == Lexer::LexemeType::UNKNOWN) {
		error = true;
	}
}
void rxpm::ExpressionParser::toUpper(std::wstring& s) {
	for (wchar_t& c : s) {
		c = towupper(c);
	}
}

rxpm::ExpressionTreeNode rxpm::ExpressionParser::parseExpression() {
	ExpressionTreeNode result = parseTerm();
	if (error) {
		return ExpressionTreeNode();
	}
	while (next.type == Lexer::LexemeType::PLUS || next.type == Lexer::LexemeType::MINUS) {
		if (next.type == Lexer::LexemeType::PLUS) {
			ExpressionTreeNode sumResult;
			sumResult.type = ExpressionType::SUM;
			sumResult.nodes.push_back(result);
			while (next.type == Lexer::LexemeType::PLUS) {
				readNext();
				if (error) {
					return ExpressionTreeNode();
				}
				result = parseTerm();
				if (error) {
					return ExpressionTreeNode();
				}
				sumResult.nodes.push_back(result);
			}
			result = std::move(sumResult);
		}
		if (next.type == Lexer::LexemeType::MINUS) {
			ExpressionTreeNode diffResult;
			diffResult.type = ExpressionType::DIFF;
			diffResult.nodes.push_back(result);
			while (next.type == Lexer::LexemeType::MINUS) {
				readNext();
				if (error) {
					return ExpressionTreeNode();
				}
				result = parseTerm();
				if (error) {
					return ExpressionTreeNode();
				}
				diffResult.nodes.push_back(result);
			}
			result = std::move(diffResult);
		}
	}
	return result;
}

rxpm::ExpressionTreeNode rxpm::ExpressionParser::parseTerm() {
	ExpressionTreeNode result = parseFactor();
	if (error) {
		return ExpressionTreeNode();
	}
	while (next.type == Lexer::LexemeType::MULT || next.type == Lexer::LexemeType::DIV) {
		if (next.type == Lexer::LexemeType::MULT) {
			ExpressionTreeNode multResult;
			multResult.type = ExpressionType::MULT;
			multResult.nodes.push_back(result);
			while (next.type == Lexer::LexemeType::MULT) {
				readNext();
				if (error) {
					return ExpressionTreeNode();
				}
				result = parseFactor();
				if (error) {
					return ExpressionTreeNode();
				}
				multResult.nodes.push_back(result);
			}
			result = std::move(multResult);
		}
		if (next.type == Lexer::LexemeType::DIV) {
			ExpressionTreeNode divResult;
			divResult.type = ExpressionType::DIV;
			divResult.nodes.push_back(result);
			while (next.type == Lexer::LexemeType::DIV) {
				readNext();
				if (error) {
					return ExpressionTreeNode();
				}
				result = parseFactor();
				if (error) {
					return ExpressionTreeNode();
				}
				divResult.nodes.push_back(result);
			}
			result = std::move(divResult);
		}
	}
	return result;
}

rxpm::ExpressionTreeNode rxpm::ExpressionParser::parseFactor() {
	ExpressionTreeNode power = parsePower();
	if (error) {
		return ExpressionTreeNode();
	}
	if (next.type == Lexer::LexemeType::POWER) {
		ExpressionTreeNode res;
		res.type = ExpressionType::POWER;
		res.nodes.push_back(power);
		// while (next.type == POWER) {
		readNext();
		if (error) {
			return ExpressionTreeNode();
		}
		power = parseFactor();
		if (error) {
			return ExpressionTreeNode();
		}
		res.nodes.push_back(power);
		// }
		return res;
	}
	return power;
}

rxpm::ExpressionTreeNode rxpm::ExpressionParser::parsePower() {
	if (next.type == Lexer::LexemeType::MINUS) {
		ExpressionTreeNode res;
		res.type = ExpressionType::INVERSE;
		readNext();
		if (error) {
			return ExpressionTreeNode();
		}
		const ExpressionTreeNode atom = parseAtom();
		if (error) {
			return ExpressionTreeNode();
		}
		res.nodes.push_back(atom);
		return res;
	}
	return parseAtom();
}

rxpm::ExpressionTreeNode rxpm::ExpressionParser::parseAtom() {
	if (next.type == Lexer::LexemeType::PAR_OPEN) {
		readNext();
		if (error) {
			return ExpressionTreeNode();
		}
		ExpressionTreeNode res = parseExpression();
		if (next.type != Lexer::LexemeType::PAR_CLOSE) {
			error = true;
			return ExpressionTreeNode();
		}
		readNext();
		if (error) {
			return ExpressionTreeNode();
		}
		return res;
	}
	if (next.type == Lexer::LexemeType::NUMBER) {
		ExpressionTreeNode res;
		res.type = ExpressionType::NUMBER;
		const double i = parseInt(next.value);
		double m = 0;
		readNext();
		if (error) {
			return ExpressionTreeNode();
		}
		if (next.type == Lexer::LexemeType::DOT) {
			readNext();
			if (error) {
				return ExpressionTreeNode();
			}
			if (next.type != Lexer::LexemeType::NUMBER) {
				error = true;
				return ExpressionTreeNode();
			}
			m = parseFractional(next.value);
			readNext();
			if (error) {
				return ExpressionTreeNode();
			}
		}
		res.number = i + m;
		return res;
	}
	if (next.type == Lexer::LexemeType::WORD) {
		ExpressionTreeNode res;
		res.type = ExpressionType::REF;
		res.ref = parseReference();
		if (error) {
			return ExpressionTreeNode();
		}
		return res;
	}
	error = true;
	return ExpressionTreeNode();
}

rxpm::Reference rxpm::ExpressionParser::parseReference() {
	if (next.type != Lexer::LexemeType::WORD) {
		error = true;
		return Reference();
	}
	Reference ref;
	std::wstring name { next.value };
	toUpper(name);
	if (name == L"COUNTERRAW" || name == L"CR") {
		ref.type = ReferenceType::COUNTER_RAW;
	} else if (name == L"COUNTERFORMATED" || name == L"CF") {
		ref.type = ReferenceType::COUNTER_FORMATTED;
	} else if (name == L"EXPRESSION" || name == L"E") {
		ref.type = ReferenceType::EXPRESSION;
	} else if (name == L"ROLLUPEXPRESSION" || name == L"R") {
		ref.type = ReferenceType::ROLLUP_EXPRESSION;
	} else if (name == L"COUNT" || name == L"C") {
		ref.type = ReferenceType::COUNT;
	} else {
		error = true;
		return Reference();
	}
	readNext();
	if (error) {
		return Reference();
	}
	if (ref.type != ReferenceType::COUNT) {
		if (next.type != Lexer::LexemeType::NUMBER) {
			error = true;
			return Reference();
		}
		ref.counter = std::stoi(next.value.data()); // will stop at first non-digit, doesn't require null-terminated string
		readNext();
		if (error) {
			return Reference();
		}
	}
	if (next.type == Lexer::LexemeType::BR_OPEN) {
		ref.name = lexer.getUntil(L'#', L']');
		ref.named = !ref.name.empty();
		if (ref.named) {
			if (ref.name[0] == L'\\') {
				std::wstring::size_type indexOfFirstNonFlag = ref.name.find_first_of(L' ');
				if (indexOfFirstNonFlag == std::string::npos) {
					indexOfFirstNonFlag = ref.name.length();
				}
				std::wstring flags = ref.name.substr(1, indexOfFirstNonFlag - 1);
				toUpper(flags);

				if (flags.find(L'D') != std::string::npos) {
					ref.discarded = true;
				}
				if (flags.find(L'O') != std::string::npos) {
					ref.useOrigName = true;
				}
				if (flags.find(L'T') != std::string::npos) {
					ref.total = true;
				}
				ref.name = ref.name.substr(indexOfFirstNonFlag);
			}

			rxu::StringUtils::trimInplace(ref.name);

			const auto len = ref.name.size();
			if (len >= 2 && ref.name[0] == L'*' && ref.name[len - 1] == L'*') {
				ref.name = ref.name.substr(1, len - 2);
				ref.namePartialMatch = true;
			}
		}
		readNext();
		if (error) {
			return Reference();
		}
		if (next.type != Lexer::LexemeType::BR_CLOSE) {
			error = true;
			return Reference();
		}
		readNext();
		if (error) {
			return Reference();
		}
	}
	if (next.type == Lexer::LexemeType::WORD) {
		std::wstring suffix { next.value };
		toUpper(suffix);
		if (suffix == L"SUM" || suffix == L"S") {
			ref.rollupFunction = RollupFunction::SUM;
		} else if (suffix == L"AVG" || suffix == L"A") {
			ref.rollupFunction = RollupFunction::AVERAGE;
		} else if (suffix == L"MIN" || next.value == L"m") {
			ref.rollupFunction = RollupFunction::MINIMUM;
		} else if (suffix == L"MAX" || next.value == L"M") {
			ref.rollupFunction = RollupFunction::MAXIMUM;
		} else if (suffix == L"COUNT" || suffix == L"C") {
			// handling deprecated rollup function
			ref.type = ReferenceType::COUNT;
		} else if (suffix == L"FIRST" || suffix == L"F") {
			ref.rollupFunction = RollupFunction::FIRST;
		}
		readNext();
		if (error) {
			return Reference();
		}
	}
	return ref;
}

int64_t rxpm::ExpressionParser::parseInt(std::wstring_view string) {
	return std::stoi(string.data()); // will stop at first non-digit, doesn't require null-terminated string
}

double rxpm::ExpressionParser::parseFractional(std::wstring_view string) {
	std::wstring temp = L"0.";
	temp += string;
	return std::stod(temp); // TODO
}
