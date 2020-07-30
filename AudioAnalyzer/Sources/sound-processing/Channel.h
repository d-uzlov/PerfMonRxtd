/*
 * Copyright (C) 2019-2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace rxtd::audio_analyzer {
	class Channel {
	public:
		class ChannelParser {
			std::map<istring, Channel, std::less<>> map{ };

		public:
			ChannelParser();

			std::optional<Channel> find(isview string);

		private:
			void addElement(isview name, Channel value) {
				map[name % own()] = value;
			}
		};

		static ChannelParser channelParser;

		enum Value {
			eFRONT_LEFT,
			eFRONT_RIGHT,
			eCENTER,
			eCENTER_BACK,
			eLOW_FREQUENCY,
			eBACK_LEFT,
			eBACK_RIGHT,
			eSIDE_LEFT,
			eSIDE_RIGHT,
			eAUTO,
		};

		using underlying_type = std::underlying_type<Value>::type;

	private:
		Value value = eFRONT_LEFT;

	public:
		Channel() = default;

		Channel(Value value) : value(value) {
		}

		[[nodiscard]]
		underlying_type toUnderlyingType() const {
			return static_cast<underlying_type>(value);
		}

		bool operator==(Channel a) const {
			return value == a.value;
		}

		bool operator!=(Channel a) const {
			return !(*this == a);
		}

		[[nodiscard]]
		sview technicalName() const;
	};

	/**
	 * For use in std::map
	 */
	inline bool operator <(Channel left, Channel right) {
		return left.toUnderlyingType() < right.toUnderlyingType();
	}
}

namespace std {
	template <>
	struct hash<audio_analyzer::Channel> {
		using Channel = audio_analyzer::Channel;
		using hash_type = audio_analyzer::Channel::underlying_type;

		size_t operator()(const Channel& c) const noexcept {
			return std::hash<hash_type>()(c.toUnderlyingType());
		}
	};
}

namespace rxtd::audio_analyzer {
	class LayoutBuilder;

	class ChannelLayout {
		friend LayoutBuilder;

		sview name = { };
		std::unordered_map<Channel, index> channelMap;
		std::vector<Channel> channelOrder;

	public:
		[[nodiscard]]
		sview getName() const {
			return name;
		}

		[[nodiscard]]
		std::optional<index> indexOf(Channel channel) const;

		[[nodiscard]]
		bool contains(Channel channel) const {
			return channelMap.find(channel) != channelMap.end();
		}

		static ChannelLayout create(sview name, const std::vector<Channel::Value>& channels);

		class const_iterator {
			decltype(channelMap)::const_iterator iter;

		public:
			explicit const_iterator(const decltype(channelMap)::const_iterator& iter) :
				iter(iter) {
			}

			const_iterator& operator++() {
				++iter;
				return *this;
			}

			[[nodiscard]]
			Channel operator*() const {
				return (*iter).first;
			}

			bool operator!=(const const_iterator& other) const {
				return iter != other.iter;
			}
		};

		[[nodiscard]]
		const_iterator begin() const {
			return const_iterator{ channelMap.cbegin() };
		}

		[[nodiscard]]
		const_iterator end() const {
			return const_iterator{ channelMap.cend() };
		}

		[[nodiscard]]
		const std::vector<Channel>& getChannelsOrderView() const {
			return channelOrder;
		}

		[[nodiscard]]
		const std::unordered_map<Channel, index>& getChannelsMapView() const {
			return channelMap;
		}

		// autogenerated
		friend bool operator==(const ChannelLayout& lhs, const ChannelLayout& rhs) {
			return lhs.name == rhs.name
				&& lhs.channelMap == rhs.channelMap
				&& lhs.channelOrder == rhs.channelOrder;
		}

		friend bool operator!=(const ChannelLayout& lhs, const ChannelLayout& rhs) {
			return !(lhs == rhs);
		}
	};

	class LayoutBuilder {
		index nextIndex = 0;
		ChannelLayout layout{ };

	public:
		LayoutBuilder& add(Channel channel);

		LayoutBuilder& skip() {
			nextIndex++;

			return *this;
		}

		[[nodiscard]]
		ChannelLayout finish() const {
			return layout;
		}
	};

	class ChannelLayouts {
	public:
		[[nodiscard]]
		static ChannelLayout getMono();
		[[nodiscard]]
		static ChannelLayout getStereo();
		[[nodiscard]]
		static ChannelLayout layoutFromChannelMask(uint32_t mask, bool forceBackSpeakers);
	};
}
