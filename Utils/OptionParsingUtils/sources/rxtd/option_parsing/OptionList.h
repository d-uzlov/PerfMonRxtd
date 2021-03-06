﻿// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2019 Danil Uzlov

#pragma once
#include "Option.h"
#include "OptionBase.h"
#include "rxtd/std_fixes/StringUtils.h"

namespace rxtd::option_parsing {
	//
	// Parses its contents in the form:
	// <value> <delimiter> <value> <delimiter> ...
	//
	class OptionList : public OptionBase {
		using SubstringViewInfo = std_fixes::SubstringViewInfo;

		std::vector<SubstringViewInfo> list{};

	public:
		OptionList() = default;

		OptionList(sview view, std::vector<SubstringViewInfo>&& list) :
			OptionBase(view), list(std::move(list)) { }

		OptionList(SourceType&& source, std::vector<SubstringViewInfo>&& list) :
			OptionBase(std::move(source)), list(std::move(list)) { }

		// Allows you to steal inner resources.
		[[nodiscard]]
		std::pair<sview, std::vector<SubstringViewInfo>> consume() && {
			return { getView(), std::move(list) };
		}

		// Count of elements in list.
		[[nodiscard]]
		index size() const {
			return static_cast<index>(list.size());
		}

		// Alias to "size() == 0".
		[[nodiscard]]
		bool empty() const {
			return list.empty();
		}

		// Returns Nth element
		[[nodiscard]]
		GhostOption get(index ind) const & {
			if (ind >= static_cast<index>(list.size())) {
				return {};
			}
			return GhostOption{ list[static_cast<std::vector<SubstringViewInfo>::size_type>(ind)].makeView(getView()) };
		}

		// Returns Nth element
		[[nodiscard]]
		Option get(index ind) const && {
			return get(ind);
		}

		template<typename T>
		class iterator {
			const OptionList& container;
			index ind;

		public:
			iterator(const OptionList& container, index _index) :
				container(container),
				ind(_index) { }

			iterator& operator++() {
				ind++;
				return *this;
			}

			bool operator !=(const iterator& other) const {
				return &container != &other.container || ind != other.ind;
			}

			[[nodiscard]]
			T operator*() const {
				return container.get(ind);
			}
		};

		[[nodiscard]]
		iterator<GhostOption> begin() const & {
			return { *this, 0 };
		}

		[[nodiscard]]
		iterator<GhostOption> end() const & {
			return { *this, size() };
		}

		[[nodiscard]]
		iterator<Option> begin() const && {
			return { *this, 0 };
		}

		[[nodiscard]]
		iterator<Option> end() const && {
			return { *this, size() };
		}
	};
}
