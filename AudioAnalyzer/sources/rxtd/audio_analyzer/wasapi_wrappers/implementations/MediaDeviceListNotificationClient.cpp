// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Danil Uzlov

#include "MediaDeviceListNotificationClient.h"

using rxtd::audio_analyzer::wasapi_wrappers::implementations::MediaDeviceListNotificationClient;

void MediaDeviceListNotificationClient::init(MediaDeviceEnumerator& enumerator) {
	auto lock = getLock();
	enumerator.ref().RegisterEndpointNotificationCallback(this);

	for (auto& dev : enumerator.getActiveDevices(MediaDeviceType::eINPUT)) {
		activeDevices.insert(dev.getId() % own());
	}

	for (auto& dev : enumerator.getActiveDevices(MediaDeviceType::eOUTPUT)) {
		activeDevices.insert(dev.getId() % own());
	}

	try {
		defaultInputId = enumerator.getDefaultDevice(MediaDeviceType::eINPUT).getId();
	} catch(winapi_wrappers::ComException&) {
		defaultInputId.clear();
	}
	try {
		defaultOutputId = enumerator.getDefaultDevice(MediaDeviceType::eOUTPUT).getId();
	} catch(winapi_wrappers::ComException&) {
		defaultOutputId.clear();
	}
}

void MediaDeviceListNotificationClient::deinit(MediaDeviceEnumerator& enumerator) {
	enumerator.ref().UnregisterEndpointNotificationCallback(this);
}

HRESULT MediaDeviceListNotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, const wchar_t* deviceId) {
	if (role != eConsole) {
		return S_OK;
	}

	auto lock = getLock();

	// if (deviceId == nullptr) then no default device is available
	// this can only happen in #OnDefaultDeviceChanged
	const auto change = deviceId == nullptr ? DefaultDeviceChange::eNO_DEVICE : DefaultDeviceChange::eCHANGED;

	if (deviceId == nullptr) {
		deviceId = L"";
	}

	if (flow == eCapture) {
		if (defaultInputId != deviceId) {
			defaultInputId = deviceId;
			changes.defaultInputChange = change;
		}
	} else {
		if (defaultOutputId != deviceId) {
			defaultOutputId = deviceId;
			changes.defaultOutputChange = change;
		}
	}

	if (changesCallback != nullptr) {
		changesCallback();
	}

	return S_OK;
}

HRESULT MediaDeviceListNotificationClient::OnDeviceAdded(const wchar_t* deviceId) {
	auto lock = getLock();
	if (activeDevices.find(deviceId) != activeDevices.end()) {
		return S_OK;
	}

	activeDevices.insert(deviceId);
	changes.stateChanged.insert(deviceId);
	changes.removed.erase(deviceId);

	if (changesCallback != nullptr) {
		changesCallback();
	}

	return S_OK;
}

HRESULT MediaDeviceListNotificationClient::OnDeviceRemoved(const wchar_t* deviceId) {
	auto lock = getLock();
	if (activeDevices.find(deviceId) == activeDevices.end()) {
		return S_OK;
	}

	activeDevices.erase(deviceId);
	changes.removed.insert(deviceId);
	changes.stateChanged.erase(deviceId);

	if (changesCallback != nullptr) {
		changesCallback();
	}

	return S_OK;
}

HRESULT MediaDeviceListNotificationClient::OnDeviceStateChanged(const wchar_t* deviceId, DWORD newState) {
	auto lock = getLock();

	if (newState == DEVICE_STATE_ACTIVE) {
		if (activeDevices.find(deviceId) != activeDevices.end()) {
			return S_OK;
		}

		activeDevices.insert(deviceId);
		changes.stateChanged.insert(deviceId);
		changes.removed.erase(deviceId);
	} else {
		if (activeDevices.find(deviceId) == activeDevices.end()) {
			return S_OK;
		}

		activeDevices.insert(deviceId);
		changes.removed.insert(deviceId);
		changes.stateChanged.erase(deviceId);
	}

	if (changesCallback != nullptr) {
		changesCallback();
	}

	return S_OK;
}

HRESULT MediaDeviceListNotificationClient::OnPropertyValueChanged(const wchar_t* deviceId, const PROPERTYKEY key) {
	auto lock = getLock();

	if (activeDevices.find(deviceId) == activeDevices.end()) {
		return S_OK;
	}

	changes.stateChanged.insert(deviceId);

	if (changesCallback != nullptr) {
		changesCallback();
	}

	return S_OK;
}
