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

#include "MathExpressionParser.h"

#include <string>
#include <string_view>
#include "StringUtils.h"

#pragma warning(disable : 4458)
#pragma warning(disable : 4244)

void rxu::ExpressionTreeNode::solve() {
	if (type == ExpressionType::NUMBER) {
		return;
	}

	bool isConst = true;
	for (ExpressionTreeNode& node : nodes) {
		node.solve();
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
	default: ;
	}
}
rxu::MathExpressionParser::Lexer::Lexer(std::wstring_view source) : source(source) {

}

rxu::MathExpressionParser::Lexer::Lexeme rxu::MathExpressionParser::Lexer::next() {
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

	Lexeme result(LexemeType::UNKNOWN, std::to_wstring(source[position]));
	position++;
	return result;
}

std::wstring_view rxu::MathExpressionParser::Lexer::getUntil(const wchar_t stop1, const wchar_t stop2) {
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

void rxu::MathExpressionParser::Lexer::skipSpaces() {
	while (position < source.length() && std::iswspace(source[position])) {
		position++;
	}
}

bool rxu::MathExpressionParser::Lexer::isSymbol(const wchar_t c) {
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

std::wstring_view rxu::MathExpressionParser::Lexer::readNumber() {
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

rxu::MathExpressionParser::MathExpressionParser(std::wstring source) : source(std::move(source)), lexer(this->source) {
	readNext();
}

rxu::MathExpressionParser::MathExpressionParser(std::wstring_view source) : lexer(source) {
	readNext();
}

void rxu::MathExpressionParser::parse() {
	result = parseExpression();
	if (next.type != Lexer::LexemeType::END) {
		error = true;
	}
}
bool rxu::MathExpressionParser::isError() const {
	return error;
}
rxu::ExpressionTreeNode rxu::MathExpressionParser::getExpression() const {
	return result;
}
void rxu::MathExpressionParser::readNext() {
	next = lexer.next();
	if (next.type == Lexer::LexemeType::UNKNOWN) {
		error = true;
	}
}
void rxu::MathExpressionParser::toUpper(std::wstring& s) {
	for (wchar_t& c : s) {
		c = towupper(c);
	}
}

rxu::ExpressionTreeNode rxu::MathExpressionParser::parseExpression() {
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

rxu::ExpressionTreeNode rxu::MathExpressionParser::parseTerm() {
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

rxu::ExpressionTreeNode rxu::MathExpressionParser::parseFactor() {
	ExpressionTreeNode power = parsePower();
	if (error) {
		return ExpressionTreeNode();
	}
	if (next.type == Lexer::LexemeType::POWER) {
		ExpressionTreeNode res;
		res.type = ExpressionType::POWER;
		res.nodes.push_back(power);
		readNext();
		if (error) {
			return ExpressionTreeNode();
		}
		power = parseFactor();
		if (error) {
			return ExpressionTreeNode();
		}
		res.nodes.push_back(power);
		return res;
	}
	return power;
}

rxu::ExpressionTreeNode rxu::MathExpressionParser::parsePower() {
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

rxu::ExpressionTreeNode rxu::MathExpressionParser::parseAtom() {
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
	error = true;
	return ExpressionTreeNode();
}

int64_t rxu::MathExpressionParser::parseInt(std::wstring_view string) {
	return std::stoi(string.data()); // will stop at first non-digit, doesn't require null-terminated string
}

double rxu::MathExpressionParser::parseFractional(std::wstring_view string) {
	std::wstring temp = L"0.";
	temp += string;
	return std::stod(temp); // TODO
}

