/*  
 * Copyright (C) 2018-2019 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */


#include "RainmeterAPI.h"
#include "PerfMonRXTD.h"
#include "PerfmonParent.h"
#include "PerfmonChild.h"
#include "TypeHolder.h"

PLUGIN_EXPORT void Initialize(void** data, void* rm) {
	rxu::Rainmeter rain(rm);

	const wchar_t *str = rain.readString(L"Type");
	if (_wcsicmp(str, L"Parent") == 0) {
		*data = new rxpm::PerfmonParent(std::move(rain));
	} else {
		*data = new rxpm::PerfmonChild(std::move(rain));
	}
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue) {
	if (data == nullptr) {
		return;
	}

	static_cast<rxu::TypeHolder*>(data)->reload();
}

PLUGIN_EXPORT double Update(void* data) {
	if (data == nullptr) {
		return 0.0;
	}

	return static_cast<rxu::TypeHolder*>(data)->update();
}

PLUGIN_EXPORT LPCWSTR GetString(void* data) {
	if (data == nullptr) {
		return nullptr;
	}

	return static_cast<rxu::TypeHolder*>(data)->getString();
}

PLUGIN_EXPORT void Finalize(void* data) {
	if (data == nullptr) {
		return;
	}

	delete static_cast<rxu::TypeHolder*>(data);
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args) {
	if (data == nullptr) {
		return;
	}

	static_cast<rxu::TypeHolder*>(data)->command(args);
}
