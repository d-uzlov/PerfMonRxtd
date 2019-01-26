/*
 * Copyright (C) 2019 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include "FileWrapper.h"
#include <Windows.h>
#include <string_view>

rxu::FileWrapper::FileWrapper(const wchar_t *path) {
	fileHandle = CreateFileW(path,
		0
		| GENERIC_WRITE
		,
		0
		| FILE_SHARE_READ
		// | FILE_SHARE_DELETE
		, nullptr, CREATE_ALWAYS,
		0
		// |FILE_ATTRIBUTE_NORMAL
		// | FILE_ATTRIBUTE_TEMPORARY
		, nullptr);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		valid = false;
		return;
	}
}

rxu::FileWrapper::~FileWrapper() {
	if (fileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(fileHandle);
		fileHandle = INVALID_HANDLE_VALUE;
	}
}

bool rxu::FileWrapper::isValid() const {
	return valid;
}

void rxu::FileWrapper::write(uint8_t* data, size_t count) {
	if (!valid) {
		return;
	}

	DWORD bytesWritten;
	const bool success = WriteFile(fileHandle, data,
		count,  // number of bytes to write
		&bytesWritten, nullptr);

	if (!success || bytesWritten != count) {
		valid = false;
		return;
	}
}

void rxu::FileWrapper::createDirectories(std::wstring_view path) {
	std::wstring buffer { path };

	size_t pos = buffer.find(L':') + 1; // either npos+1 == 0 or index of first meaningful symbol

	while (true) {
		const auto nextPos = buffer.find(L'\\', pos);

		if (nextPos == std::wstring::npos) {
			break;
		}

		buffer[nextPos] = L'\0';
		CreateDirectoryW(buffer.c_str(), nullptr);
		buffer[nextPos] = L'\\';

		pos = nextPos + 1;
	}
	CreateDirectoryW(buffer.c_str(), nullptr);
}