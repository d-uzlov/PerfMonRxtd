// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Danil Uzlov

#include "Color.h"
#include "rxtd/Logger.h"
#include "rxtd/option_parsing/Option.h"
#include "rxtd/option_parsing/OptionList.h"

using rxtd::audio_analyzer::image_utils::Color;

using rxtd::option_parsing::OptionList;
using rxtd::option_parsing::Option;
using rxtd::std_fixes::StringUtils;
using rxtd::Logger;

Color Color::rgb2hsv() const {
	const float xMax = std::max(std::max(_.rgb.red, _.rgb.green), _.rgb.blue);
	const float xMin = std::min(std::min(_.rgb.red, _.rgb.green), _.rgb.blue);

	const float val = xMax;
	const float chroma = xMax - xMin;

	float hue = 0.0f;
	if (chroma == 0.0f) {
		hue = 0.0f;
	} else if (val == _.rgb.red) {
		hue = (_.rgb.green - _.rgb.blue) / chroma;
	} else if (val == _.rgb.green) {
		hue = 2.0f + (_.rgb.blue - _.rgb.red) / chroma;
	} else if (val == _.rgb.blue) {
		hue = 4.0f + (_.rgb.red - _.rgb.green) / chroma;
	}
	hue *= 60.0f;

	const float sat = val == 0.0f ? 0.0f : chroma / val;

	return { hue, sat, val, alpha, Mode::eHSV };
}

Color Color::hsv2hsl() const {
	Color result;
	result.mode = Mode::eHSL;
	result.alpha = alpha;
	result._.hsv.hue = _.hsv.hue;

	const float l = _.hsv.val * (1.0f - _.hsv.sat * 0.5f);
	if (l <= 0.0f || l >= 1.0f) {
		result._.hsv.sat = 0.0f;
	} else {
		result._.hsv.sat = (_.hsv.val - l) / std::min(l, 1.0f - l);
	}

	result._.hsv.val = l;

	return result;
}

Color Color::hsl2hsv() const {
	Color result;
	result.mode = Mode::eHSV;
	result.alpha = alpha;
	result._.hsv.hue = _.hsv.hue;

	const float v = _.hsv.val + _.hsv.sat * std::min(_.hsv.val, 1.0f - _.hsv.val);
	if (v <= 0.0f) {
		result._.hsv.sat = 0.0f;
	} else {
		result._.hsv.sat = 2.0f * (1.0f - _.hsv.val / v);
	}

	result._.hsv.val = v;

	return result;
}

Color Color::hsv2rgb() const {
	const float chroma = _.hsv.val * _.hsv.sat;
	float fractionalPart;
	const float h = std::modff(_.hsv.hue * (1.0f / 60.0f) * (1.0f / 6.0f), &fractionalPart) * 6.0f;
	const float hFraction = std::modff(h * 0.5f, &fractionalPart) * 2.0f;
	const float x = chroma * (1.0f - std::abs(hFraction - 1.0f));

	struct {
		float r, g, b;
	} tmp{};
	if (chroma == 0.0f) {
		tmp = { 0.0f, 0.0f, 0.0f };
	} else if (h >= 0.0f && h <= 1.0f) {
		tmp = { chroma, x, 0.0f };
	} else if (h >= 1.0f && h <= 2.0f) {
		tmp = { x, chroma, 0.0f };
	} else if (h >= 2.0f && h <= 3.0f) {
		tmp = { 0.0f, chroma, x };
	} else if (h >= 3.0f && h <= 4.0f) {
		tmp = { 0.0f, x, chroma };
	} else if (h >= 4.0f && h <= 5.0f) {
		tmp = { x, 0.0f, chroma };
	} else {
		tmp = { chroma, 0.0f, x };
	}

	const float m = _.hsv.val - chroma;
	return {
		tmp.r + m,
		tmp.g + m,
		tmp.b + m,
		alpha,
		Mode::eRGB
	};
}

Color Color::rgb2ycbcr() const {
	const float kr = 0.299f;
	const float kg = 0.587f;
	const float kb = 0.114f;

	const float y = kr * _.rgb.red + kg * _.rgb.green + kb * _.rgb.blue;
	const float pb = 0.5f * (_.rgb.blue - y) / (1.0f - kb);
	const float pr = 0.5f * (_.rgb.red - y) / (1.0f - kr);

	Color result;
	result.mode = Mode::eYCBCR;
	result._.tv.y = y;
	result._.tv.cb = pb;
	result._.tv.cr = pr;
	result.alpha = alpha;

	return result;
}

Color Color::ycbcr2rgb() const {
	const float kr = 0.299f;
	const float kg = 0.587f;
	const float kb = 0.114f;

	const float r = _.tv.y + (2.0f - 2.0f * kr) * _.tv.cr;
	const float g = _.tv.y - kb / kg * (2.0f - 2.0f * kb) * _.tv.cb - kr / kg * (2.0f - 2.0f * kr) * _.tv.cr;
	const float b = _.tv.y + (2.0f - 2.0f * kb) * _.tv.cb;

	Color result;
	result.mode = Mode::eRGB;
	result._.rgb.red = r;
	result._.rgb.green = g;
	result._.rgb.blue = b;
	result.alpha = alpha;

	return result;
}

template<>
Color rxtd::option_parsing::OptionParser::ParseContext::solveCustom
<Color, Color::Mode>
(const Color::Mode& defaultMode) {
	OptionList components{};

	using Mode = Color::Mode;
	Mode mode = defaultMode;

	if (source.startsWith(L"@")) {
		source.remove_prefix(1);
		auto [attr, colorDesc] = Option{ source }.breakFirst(L' ');
		components = colorDesc.asList(L',');

		if (colorDesc.empty()) {
			parent.logger.error(L"{}: annotated color without color components: '{}'", loggerPrefix, source);
			throw Exception{};
		}

		auto modeOpt = parseEnum<Color::Mode>(attr.asIString(L"sRGB"));
		if (!modeOpt.has_value()) {
			parent.logger.error(L"{}: unknown color mode: {}", loggerPrefix, attr);
			throw Exception{};
		}

		mode = modeOpt.value();
		source = colorDesc.asString();
	} else {
		components = Option{ source }.asList(L',');
	}

	if (mode == Mode::eHEX) {
		if (source.size() != 6 && source.size() != 8) {
			parent.logger.error(L"{}: can't parse as HEX color: need 6 or 8 hex digits, value: {}", loggerPrefix, source);
			throw Exception{};
		}

		for (auto c : source) {
			if (!std::iswxdigit(c)) {
				parent.logger.error(L"{}: can't parse as HEX color: only hex digits are allowed, value: {}", loggerPrefix, source);
				throw Exception{};
			}
		}

		using namespace std::string_literals;

		float floatComponents[3]{
			parent.parse(source.substr(0, 2), L"components 1").asCustom<float>(hex_tag{}) / 255.0f,
			parent.parse(source.substr(2, 2), L"components 2").asCustom<float>(hex_tag{}) / 255.0f,
			parent.parse(source.substr(4, 2), L"components 3").asCustom<float>(hex_tag{}) / 255.0f,
		};

		const float alpha =
			source.size() == 8
			? parent.parse(source.substr(6, 2), L"components 4").asCustom<float>(hex_tag{}) / 255.0f
			: 1.0f;

		return Color{ floatComponents[0], floatComponents[1], floatComponents[2], alpha, Mode::eRGB };
	}

	const auto count = components.size();
	if (count < 3 || count > 4) {
		parent.logger.error(L"{}: can't parse color: need 3 or 4 color components: {}", loggerPrefix, source);
		throw Exception{};
	}

	float floatComponents[3]{
		parent.parse(components.get(0), L"colors").as<float>(),
		parent.parse(components.get(1), L"colors").as<float>(),
		parent.parse(components.get(2), L"colors").as<float>(),
	};

	const float alpha =
		components.size() == 4
		? parent.parse(components.get(3), L"colors").as<float>()
		: (mode == Mode::eRGB255 ? 255.0f : 1.0f);

	return Color{ floatComponents[0], floatComponents[1], floatComponents[2], alpha, mode };
}

template<>
std::optional<Color::Mode> parseEnum<Color::Mode>(rxtd::isview text) {
	using Mode = Color::Mode;
	if (text == L"sRGB") {
		return Mode::eRGB;
	} else if (text == L"hex") {
		return Mode::eHEX;
	} else if (text == L"sRGB255") {
		return Mode::eRGB255;
	} else if (text == L"hsv") {
		return Mode::eHSV;
	} else if (text == L"hsl") {
		return Mode::eHSL;
	} else if (text == L"yCbCr") {
		return Mode::eYCBCR;
	} else {
		return {};
	}
}
