// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2019 Danil Uzlov

#pragma once

#include <charconv>

namespace rxtd::std_fixes {
	//
	// Class that can be used instead of string_view when the string is likely to be moved,
	// so all the views would point into incorrect chunk of memory.
	// SubstringViewInfo allows to create string view from given source, in the time when this exact copy of the source is valid.
	//
	class SubstringViewInfo {
		index offset{};
		index length{};

	public:
		SubstringViewInfo() = default;

		SubstringViewInfo(index offset, index length) : offset(offset), length(length) { }

		[[nodiscard]]
		bool empty() const {
			return length == 0;
		}

		sview makeView(const wchar_t* base) const {
			return sview(base + offset, static_cast<size_t>(length));
		}

		template<typename CharTraits>
		[[nodiscard]]
		StringViewBase<wchar_t, CharTraits> makeView(StringViewBase<wchar_t, CharTraits> base) const {
			return StringViewBase<wchar_t, CharTraits>(base.data() + offset, static_cast<size_t>(length));
		}

		template<typename CharTraits, typename Allocator>
		[[nodiscard]]
		StringViewBase<wchar_t, CharTraits> makeView(const StringBase<wchar_t, CharTraits, Allocator>& base) const {
			return StringViewBase<wchar_t, CharTraits>(base.data() + offset, static_cast<size_t>(length));
		}

		[[nodiscard]]
		sview makeView(const std::vector<wchar_t>& base) const {
			return makeView(sview{ base.data(), base.size() });
		}

		[[nodiscard]]
		SubstringViewInfo substr(index subOffset, index subLength = std::numeric_limits<index>::max()) const;

		[[nodiscard]]
		bool operator <(const SubstringViewInfo& other) const;
	};

	class StringUtils {
	public:
		/// <summary>
		/// Translated all symbols in the string to uppercase
		/// Be careful when using: c++17 doesn't have anything like string_span built-in
		/// and string_view can be constructed from read-only memory.
		/// However, since string_view is a lot easier to pass around,
		/// this function forcefully change string_view content.
		/// This may result in page fault!
		/// The caller is responsible to only call this on read-write views.
		/// </summary>
		static void makeUppercaseInPlace(sview str);

		static void makeUppercaseInPlace(string& str) {
			makeUppercaseInPlace(str.view());
		}

		template<typename CharTraits>
		static StringViewBase<wchar_t, CharTraits> trim(StringViewBase<wchar_t, CharTraits> view) {
			const auto begin = view.find_first_not_of(L" \t");
			if (begin == sview::npos) {
				return {};
			}

			view.remove_prefix(begin);

			const auto end = view.find_last_not_of(L" \t"); // always valid if find_first_not_of succeeded

			return view.substr(0, end + 1);
		}

		[[nodiscard]]
		static SubstringViewInfo trimInfo(sview source, SubstringViewInfo viewInfo);

		template<typename CharTraits, typename Allocator>
		static void lowerInplace(std::basic_string<wchar_t, CharTraits, Allocator>& str) {
			for (auto& c : str) {
				c = std::towlower(c);
			}
		}

		template<typename CharTraits, typename Allocator>
		static void upperInplace(std::basic_string<wchar_t, CharTraits, Allocator>& str) {
			for (auto& c : str) {
				c = std::towupper(c);
			}
		}

		template<typename CharTraits>
		[[nodiscard]]
		static string copyLower(std::basic_string_view<wchar_t, CharTraits> str) {
			std::basic_string<wchar_t, CharTraits> buffer{ str };
			lowerInplace(buffer);

			return buffer;
		}

		template<typename CharTraits>
		[[nodiscard]]
		static string copyUpper(std::basic_string_view<wchar_t, CharTraits> str) {
			std::basic_string<wchar_t, CharTraits> buffer{ str };
			upperInplace(buffer);

			return buffer;
		}

		template<typename CharTraits, typename Allocator>
		static void trimInplace(std::basic_string<wchar_t, CharTraits, Allocator>& str) {
			const auto firstNotSpace = std::find_if_not(
				str.begin(), str.end(), [](wint_t c) {
					return std::iswspace(c);
				}
			);
			str.erase(str.begin(), firstNotSpace);
			const auto lastNotSpace = std::find_if_not(
				str.rbegin(), str.rend(), [](wint_t c) {
					return std::iswspace(c);
				}
			).base();
			str.erase(lastNotSpace, str.end());
		}

		template<typename CharTraits>
		[[nodiscard]]
		static StringBase<wchar_t, CharTraits> trimCopy(StringViewBase<wchar_t, CharTraits> str) {
			// from stackoverflow
			const auto firstNotSpace = std::find_if_not(
				str.begin(),
				str.end(),
				[](wint_t c) {
					return std::iswspace(c);
				}
			);
			const auto lastNotSpace = std::find_if_not(
				str.rbegin(),
				sview::const_reverse_iterator(firstNotSpace),
				[](wint_t c) {
					return std::iswspace(c);
				}
			).base();

			return { firstNotSpace, lastNotSpace };
		}

		[[nodiscard]]
		static string trimCopy(const wchar_t* str) {
			return trimCopy(sview{ str });
		}

		template<typename CharTraits, typename Allocator>
		static void substringInplace(
			std::basic_string<wchar_t, CharTraits, Allocator>& str,
			index begin,
			index count = string::npos
		) {
			if (count != string::npos) {
				const index endRemoveStart = begin + count;
				if (endRemoveStart <= static_cast<index>(str.length())) {
					str.erase(endRemoveStart);
				}
			}
			if (begin > 0) {
				str.erase(0, begin);
			}
		}

		[[nodiscard]]
		static index parseInt(sview view, bool forceHex = false);

		[[nodiscard]]
		static double parseFloat(sview view);
	};
}
