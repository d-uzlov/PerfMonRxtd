﻿/* 
 * Copyright (C) 2019 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include "NamesManager.h"
#include <string_view>
#include <unordered_map>

namespace rxpm::pdh {
	using namespace std::string_view_literals;

	const ModifiedNameItem& NamesManager::get(size_t index) const {
		return names[index];
	}

	void NamesManager::setModificationType(ModificationType value) {
		modificationType = value;
	}

	void NamesManager::createModifiedNames(const Snapshot& snapshot, const Snapshot& idSnapshot) {
		resetBuffers();

		originalNamesSize = snapshot.getNamesSize();
		names.resize(snapshot.getItemsCount());

		copyOriginalNames(snapshot);

		switch (modificationType) {
		case ModificationType::NONE:
			break;
		case ModificationType::PROCESS:
			modifyNameProcess(idSnapshot);
			break;
		case ModificationType::THREAD:
			modifyNameThread(idSnapshot);
			break;
		case ModificationType::LOGICAL_DISK_DRIVE_LETTER:
			modifyNameLogicalDiskDriveLetter();
			break;
		case ModificationType::LOGICAL_DISK_MOUNT_PATH:
			modifyNameLogicalDiskMountPath();
			break;
		case ModificationType::GPU_PROCESS:
			modifyNameGPUProcessName(idSnapshot);
			break;
		case ModificationType::GPU_ENGTYPE:
			modifyNameGPUEngtype();
			break;
		default:
			break;
		}

		generateSearchNames();
	}

	void NamesManager::copyOriginalNames(const Snapshot& snapshot) {
		wchar_t* namesBuffer = getBuffer(originalNamesSize);

		for (unsigned long instanceInx = 0; instanceInx < snapshot.getItemsCount(); ++instanceInx) {
			const wchar_t* namePtr = snapshot.getName(instanceInx);
			ModifiedNameItem& item = names[instanceInx];

			std::wstring_view name = copyString(namePtr, namesBuffer);
			namesBuffer += name.length();

			item.originalName = name;
			item.uniqueName = name;
			item.displayName = name;
		}
	}

	void NamesManager::generateSearchNames() {
		wchar_t* namesBuffer = getBuffer(originalNamesSize);

		for (auto& item : names) {
			std::wstring_view name = copyString(item.displayName, namesBuffer);
			namesBuffer[name.length()] = L'\0';
			CharUpperW(namesBuffer);

			// don't need +1 here because string_view doesn't need null-termination
			namesBuffer += name.length();

			item.searchName = name;
		}
	}

	void NamesManager::resetBuffers() {
		buffersCount = 0;
	}

	wchar_t* NamesManager::getBuffer(size_t value) {
		const auto nextBufferIndex = buffersCount;
		buffersCount++;

		if (nextBufferIndex >= buffers.size()) {
			buffers.emplace_back();
		}

		auto& buffer = buffers[nextBufferIndex];
		buffer.reserve(value);
		return buffer.data();
	}

	std::wstring_view NamesManager::copyString(std::wstring_view source, wchar_t* dest) {
		const auto len = source.length();
		std::copy_n(source.data(), len, dest);
		return { dest, len };
	}

	void NamesManager::modifyNameProcess(const Snapshot& idSnapshot) {
		// process name is name of the process file, which is not unique
		// set unique name to <name>#<pid>
		// assume that string representation of pid is no more than 20 symbols

		wchar_t* namesBuffer = getBuffer(originalNamesSize + names.size() * 20);

		rxu::BufferPrinter printer { };

		for (unsigned long instanceInx = 0; instanceInx < names.size(); ++instanceInx) {
			ModifiedNameItem& item = names[instanceInx];

			const long long pid = idSnapshot.getItem(0, instanceInx).FirstValue;

			auto nameCopy = copyString(item.originalName, namesBuffer);
			namesBuffer += nameCopy.length();
			namesBuffer[0] = L'#';
			namesBuffer++;

			const auto length = swprintf(namesBuffer, 20, L"%llu", pid);
			if (length < 0) {
				namesBuffer += 20;
			} else {
				namesBuffer += length;
			}

			item.uniqueName = { nameCopy.data(), nameCopy.length() + 1 + length };
		}
	}

	void NamesManager::modifyNameThread(const Snapshot& idSnapshot) {
		// instance names are "<processName>/<threadIndex>"
		// process names are not unique
		// thread indices enumerate threads inside one process, starting from 0

		// unique name: <processName>#<tid>
		// exception is Idle/n: all threads have tid 0, so keep /n for this tid
		// _Total/_Total also has tid 0, no problems with this

		// display name: <processName>
		// _Total/_Total -> _Total
		// Idle/n -> Idle

		// assume that string representation of tid is no more than 20 symbols

		wchar_t* namesBuffer = getBuffer(originalNamesSize + names.size() * 20);

		for (unsigned long instanceInx = 0; instanceInx < names.size(); ++instanceInx) {
			ModifiedNameItem& item = names[instanceInx];

			const long long tid = idSnapshot.getItem(0, instanceInx).FirstValue;

			std::wstring_view nameCopy = copyString(item.originalName, namesBuffer);
			if (tid != 0) {
				nameCopy = nameCopy.substr(0, nameCopy.find_last_of(L'/'));
			}
			namesBuffer += nameCopy.length();

			namesBuffer[0] = L'#';
			namesBuffer++;

			const auto length = swprintf(namesBuffer, 20, L"%llu", tid);
			if (length < 0) {
				namesBuffer += 20;
			} else {
				namesBuffer += length;
			}

			item.uniqueName = { nameCopy.data(), nameCopy.length() + 1 + length };

			item.displayName = item.originalName.substr(0, item.originalName.find_last_of(L'/'));
		}
	}

	void NamesManager::modifyNameLogicalDiskDriveLetter() {
		// keep folder in mount path: "C:\path\mount" -> "C:"
		// volumes that are not mounted: "HardDiskVolume#123" -> "HardDiskVolume"

		for (auto& item : names) {
			if (item.originalName.length() < 2) {
				continue;
			}

			if (item.originalName[1] == L':') {
				item.displayName = item.originalName.substr(0, 2);
			} else if (rxu::StringUtils::startsWith(item.originalName, L"HarddiskVolume"sv)) {
				item.displayName = L"HarddiskVolume"sv;
			}
		}
	}

	void NamesManager::modifyNameLogicalDiskMountPath() {
		// keep folder in mount path: "C:\path\mount" -> "C:\path\"
		// volumes that are not mounted: "HardDiskVolume#123" -> "HardDiskVolume"

		for (auto& item : names) {
			if (item.originalName.length() < 2) {
				continue;
			}

			if (item.originalName[1] == L':') {
				const auto slashPosition = item.originalName.find_last_of(L'\\');
				if (slashPosition != std::wstring_view::npos) {
					item.displayName = item.originalName.substr(0, slashPosition + 1);
				}
			} else if (rxu::StringUtils::startsWith(item.originalName, L"HarddiskVolume"sv)) {
				item.displayName = L"HarddiskVolume"sv;
			}
		}
	}

	void NamesManager::modifyNameGPUProcessName(const Snapshot& idSnapshot) {
		// display name is process name (found by PID)

		wchar_t* processNamesBuffer = getBuffer(idSnapshot.getNamesSize());

		std::unordered_map<long long, std::wstring_view> pidToName(idSnapshot.getItemsCount());
		for (size_t instanceInx = 0; instanceInx < idSnapshot.getItemsCount(); ++instanceInx) {
			std::wstring_view name = copyString(idSnapshot.getName(instanceInx), processNamesBuffer);
			processNamesBuffer += name.length();

			const auto pid = idSnapshot.getItem(0, instanceInx).FirstValue;
			pidToName[pid] = name;
		}

		for (auto& item : names) {
			const auto pidPosition = item.originalName.find(L"pid_");
			if (pidPosition == std::wstring_view::npos) {
				continue;
			}

			const auto pid = wcstoul(item.originalName.data() + pidPosition + 4, nullptr, 10);
			const auto iter = pidToName.find(pid);
			if (iter == pidToName.end()) {
				continue;
			}

			item.displayName = iter->second;
		}
	}

	void NamesManager::modifyNameGPUEngtype() {
		// keeping engtype_x only

		for (auto& item : names) {
			const auto suffixStartPlace = item.originalName.find(L"engtype_");
			if (suffixStartPlace != std::wstring_view::npos) {
				item.displayName = item.originalName.substr(suffixStartPlace);
			}
		}
	}

}