﻿/* 
 * Copyright (C) 2019 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "PdhSnapshot.h"

namespace rxtd::perfmon::pdh {
	struct UniqueInstanceId {
		int32_t id1;
		int32_t id2;

		// autogenerated
		friend bool operator==(const UniqueInstanceId& lhs, const UniqueInstanceId& rhs) {
			return lhs.id1 == rhs.id1
				&& lhs.id2 == rhs.id2;
		}

		friend bool operator!=(const UniqueInstanceId& lhs, const UniqueInstanceId& rhs) {
			return !(lhs == rhs);
		}
	};

	struct ModifiedNameItem {
		sview originalName;
		sview displayName;
		sview searchName;
	};

	class NamesManager {
	public:
		enum class ModificationType {
			NONE,
			PROCESS,
			THREAD,
			LOGICAL_DISK_DRIVE_LETTER,
			LOGICAL_DISK_MOUNT_PATH,
			GPU_PROCESS,
			GPU_ENGTYPE,
		};

	private:
		std::map<string, index, std::less<>> name2id;
		index lastId = 0;

		std::vector<ModifiedNameItem> names;
		std::vector<wchar_t> buffer;

		ModificationType modificationType{};

		index namesSize = 0;

	public:
		const ModifiedNameItem& get(index ind) const {
			return names[ind];
		}

		void setModificationType(ModificationType value) {
			modificationType = value;
		}

		void createModifiedNames(const PdhSnapshot& snapshot, const PdhSnapshot& processIdSnapshot, array_span<UniqueInstanceId> ids);

	private:
		void fillOriginalNames(const PdhSnapshot& snapshot);

		void generateSearchNames();

		static sview copyString(sview source, wchar_t* dest);

		void modifyNameProcess(const PdhSnapshot& snapshot, array_span<UniqueInstanceId> ids);

		void modifyNameThread(const PdhSnapshot& snapshot, array_span<UniqueInstanceId> ids);

		void modifyNameLogicalDiskDriveLetter();

		void modifyNameLogicalDiskMountPath();

		void modifyNameGPUProcessName(const PdhSnapshot& idSnapshot);

		void modifyNameGPUEngtype();

		void createIdsBasedOnName(array_span<UniqueInstanceId> ids);

		int32_t getIdFromName(sview name);
	};
}
