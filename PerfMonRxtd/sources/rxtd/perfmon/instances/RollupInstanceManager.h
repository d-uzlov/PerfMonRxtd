﻿/*
 * Copyright (C) 2019-2021 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "SimpleInstanceManager.h"
#include "rxtd/perfmon/BlacklistManager.h"
#include "rxtd/perfmon/Reference.h"
#include "rxtd/perfmon/pdh/PdhWrapper.h"

namespace rxtd::perfmon {
	namespace expressions {
		class RollupExpressionResolver;
	}

	class RollupInstanceManager {
	public:
		using Indices = SimpleInstanceManager::Indices;

		struct RollupInstanceInfo {
			std::vector<Indices> indices;
			sview sortName;
			double sortValue = 0.0;

			[[nodiscard]]
			Indices getFirst() const {
				return indices.front();
			}
		};

		struct Options {
			SortInfo sortInfo;
			bool limitIndexOffset = false;
		};

	private:
		Logger log;

		Options options;
		index indexOffset = 0;

		std::vector<RollupInstanceInfo> instancesRolledUp;

		const SimpleInstanceManager& simpleInstanceManager;

		struct CacheKey {
			MatchPattern pattern;
			bool useOriginalName;

			friend bool operator<(const CacheKey& lhs, const CacheKey& rhs) {
				return lhs.pattern < rhs.pattern
					&& lhs.useOriginalName < rhs.useOriginalName;
			}
		};

		struct Caches {
			std::map<CacheKey, std::optional<const RollupInstanceInfo*>> rollup;

			void reset() {
				rollup.clear();
			}
		};

		mutable Caches nameCaches;

	public:
		RollupInstanceManager(Logger log, const SimpleInstanceManager& simpleInstanceManager) :
			log(std::move(log)), simpleInstanceManager(simpleInstanceManager) {}

		void setOptions(Options value) {
			options = value;

			if (options.limitIndexOffset && indexOffset < 0) {
				indexOffset = 0;
			}
		}

		void setIndexOffset(index value, bool relative) {
			if (relative) {
				indexOffset += value;
			} else {
				indexOffset = value;
			}
			if (options.limitIndexOffset && indexOffset < 0) {
				indexOffset = 0;
			}
		}

		void update();

		void sort(const expressions::RollupExpressionResolver& simpleExpressionSolver);

		array_view<RollupInstanceInfo> getRollupInstances() const {
			return instancesRolledUp;
		}

		const RollupInstanceInfo* findRollupInstance(const Reference& ref, index sortedIndex) const;

		const RollupInstanceInfo* findRollupInstanceByName(const Reference& ref) const;

	private:
		void buildRollupKeys();
	};
}
