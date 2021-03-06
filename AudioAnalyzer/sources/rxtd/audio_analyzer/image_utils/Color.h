// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2019 Danil Uzlov

#pragma once
#include "IntColor.h"
#include "rxtd/option_parsing/OptionParser.h"

namespace rxtd::audio_analyzer::image_utils {
	class Color {
	public:
		enum class Mode {
			eRGB,
			eHEX,
			eRGB255,
			eHSV,
			eHSL,
			eYCBCR,
		};

	private:
		union {
			struct {
				float red = 0.0;
				float green = 0.0;
				float blue = 0.0;
			} rgb;

			struct {
				float hue;
				float sat;
				float val;
			} hsv;

			struct {
				float y;
				float cb;
				float cr;
			} tv;
		} _{};

		float alpha = 1.0f;
		Mode mode = Mode::eRGB;

	public:
		Color() = default;

		Color(float red, float green, float blue, float alpha = 1.0f, Mode mode = Mode::eRGB) :
			alpha(alpha), mode(mode) {
			_.rgb.red = red;
			_.rgb.green = green;
			_.rgb.blue = blue;
		}

		[[nodiscard]]
		Color operator*(float value) const {
			return {
				_.rgb.red * value,
				_.rgb.green * value,
				_.rgb.blue * value,
				alpha * value,
				mode
			};
		}

		[[nodiscard]]
		Color operator+(const Color& other) const {
			return {
				_.rgb.red + other._.rgb.red,
				_.rgb.green + other._.rgb.green,
				_.rgb.blue + other._.rgb.blue,
				alpha + other.alpha,
				mode
			};
		}

		// autogenerated
		friend bool operator==(const Color& lhs, const Color& rhs) {
			return lhs._.rgb.red == rhs._.rgb.red
				&& lhs._.rgb.green == rhs._.rgb.green
				&& lhs._.rgb.blue == rhs._.rgb.blue
				&& lhs.alpha == rhs.alpha;
		}

		friend bool operator!=(const Color& lhs, const Color& rhs) {
			return !(lhs == rhs);
		}

		[[nodiscard]]
		Color rgb() const {
			switch (mode) {
			case Mode::eRGB: return *this;
			case Mode::eHEX: [[fallthrough]];
			case Mode::eRGB255: {
				auto result = (*this) * (1.0f / 255.0f);
				result.mode = Mode::eRGB;
				return result;
			}
			case Mode::eHSV: return hsv2rgb();
			case Mode::eHSL: return hsl2hsv().hsv2rgb();
			case Mode::eYCBCR: return ycbcr2rgb();
			}
			return {};
		}

		[[nodiscard]]
		Color hsv() const {
			switch (mode) {
			case Mode::eRGB: return rgb2hsv();
			case Mode::eHEX: [[fallthrough]];
			case Mode::eRGB255: {
				auto result = (*this) * (1.0f / 255.0f);
				result.mode = Mode::eRGB;
				return result.rgb2hsv();
			}
			case Mode::eHSV: return *this;
			case Mode::eHSL: return hsl2hsv();
			case Mode::eYCBCR: return ycbcr2rgb().rgb2hsv();
			}
			return {};
		}

		[[nodiscard]]
		Color hsl() const {
			switch (mode) {
			case Mode::eRGB: return rgb2hsv().hsv2hsl();
			case Mode::eHEX: [[fallthrough]];
			case Mode::eRGB255: {
				auto result = (*this) * (1.0f / 255.0f);
				result.mode = Mode::eRGB;
				return result.rgb2hsv().hsv2hsl();
			}
			case Mode::eHSV: return hsv2hsl();
			case Mode::eHSL: return *this;
			case Mode::eYCBCR: return ycbcr2rgb().rgb2hsv().hsv2hsl();
			}
			return {};
		}

		[[nodiscard]]
		Color ycbcr() const {
			switch (mode) {
			case Mode::eRGB: return rgb2ycbcr();
			case Mode::eHEX: [[fallthrough]];
			case Mode::eRGB255: {
				auto result = (*this) * (1.0f / 255.0f);
				result.mode = Mode::eRGB;
				return result.rgb2ycbcr();
			}
			case Mode::eHSV: return hsv2rgb().rgb2ycbcr();
			case Mode::eHSL: return hsl2hsv().hsv2rgb().rgb2ycbcr();
			case Mode::eYCBCR: return *this;
			}
			return {};
		}

		[[nodiscard]]
		Color convert(Mode _mode) const {
			switch (_mode) {
			case Mode::eRGB: return rgb();
			case Mode::eHEX: {
				auto result = rgb() * 255.0f;
				result.mode = Mode::eHEX;
				return result;
			}
			case Mode::eRGB255: {
				auto result = rgb() * 255.0f;
				result.mode = Mode::eRGB255;
				return result;
			}
			case Mode::eHSV: return hsv();
			case Mode::eHSL: return hsl();
			case Mode::eYCBCR: return ycbcr();
			}
			return {};
		}

		[[nodiscard]]
		IntColor toIntColor() const {
			if (mode != Mode::eRGB) {
				return rgb().toIntColor();
			}

			IntColor result{};
			result.value.rgba.r = uint8_t(std::clamp<int>(std::lround(_.rgb.red * 255), 0, 255));
			result.value.rgba.g = uint8_t(std::clamp<int>(std::lround(_.rgb.green * 255), 0, 255));
			result.value.rgba.b = uint8_t(std::clamp<int>(std::lround(_.rgb.blue * 255), 0, 255));
			result.value.rgba.a = uint8_t(std::clamp<int>(std::lround(alpha * 255), 0, 255));

			return result;
		}

	private:
		[[nodiscard]]
		Color rgb2hsv() const;

		[[nodiscard]]
		Color hsv2hsl() const;

		[[nodiscard]]
		Color hsl2hsv() const;

		[[nodiscard]]
		Color hsv2rgb() const;

		[[nodiscard]]
		Color rgb2ycbcr() const;

		[[nodiscard]]
		Color ycbcr2rgb() const;
	};
}

template<>
rxtd::audio_analyzer::image_utils::Color
rxtd::option_parsing::OptionParser::ParseContext::solveCustom
<rxtd::audio_analyzer::image_utils::Color, rxtd::audio_analyzer::image_utils::Color::Mode>
(const audio_analyzer::image_utils::Color::Mode& defaultMode);

template<>
std::optional<rxtd::audio_analyzer::image_utils::Color::Mode>
parseEnum<rxtd::audio_analyzer::image_utils::Color::Mode>(rxtd::isview text);
