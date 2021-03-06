// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Danil Uzlov

#pragma once

// my-windows must be before any WINAPI include
#include "rxtd/my-windows.h"
// ReSharper disable once CppWrongIncludesOrder
#include <endpointvolume.h>
#include <mmdeviceapi.h>

#include "AudioClientHandle.h"
#include "rxtd/winapi_wrappers/GenericComWrapper.h"

namespace rxtd::audio_analyzer::wasapi_wrappers {
	class MediaDeviceHandle : public winapi_wrappers::GenericComWrapper<IMMDevice> {
	public:
		struct DeviceInfo {
			string name;
			string desc;
			string formFactor;
		};

	private:
		string id;
		bool infoIsFilled = false;
		MediaDeviceType type{};

	public:
		MediaDeviceHandle() = default;

		/// <summary>
		/// Can throw ComException.
		/// </summary>
		template<typename InitFunction>
		MediaDeviceHandle(InitFunction initFunction, sview id) noexcept(false) : GenericComWrapper(std::move(initFunction)), id(id) {
			readType();
		}

		/// <summary>
		/// Can throw ComException.
		/// </summary>
		template<typename InitFunction>
		MediaDeviceHandle(InitFunction initFunction, MediaDeviceType type) noexcept(false) : GenericComWrapper(std::move(initFunction)), type(type) {
			readId();
		}

		[[nodiscard]]
		sview getId() const {
			return id;
		}

		[[nodiscard]]
		auto getType() const {
			return type;
		}

		/// <summary>
		/// Can throw ComException.
		/// </summary>
		[[nodiscard]]
		DeviceInfo readDeviceInfo() noexcept(false);

		/// <summary>
		/// Can throw ComException.
		/// </summary>
		[[nodiscard]]
		AudioClientHandle openAudioClient() noexcept(false);

		/// <summary>
		/// Type safe call to IMMDevice::Activate.
		/// Can throw ComException.
		/// </summary>
		/// <typeparam name="Interface">Type of the class to be retrieved by IMMDevice::Activate.</typeparam>
		/// <returns>Valid GenericComWrapper object.</returns>
		template<typename Interface>
		GenericComWrapper<Interface> activateFor() noexcept(false) {
			sview source;
			if constexpr (std::is_same_v<IAudioClient, Interface>) {
				source = L"IAudioClient.Activate(IAudioClient)";
			} else if constexpr (std::is_same_v<IAudioEndpointVolume, Interface>) {
				source = L"IAudioClient.Activate(IAudioEndpointVolume)";
			} else if constexpr (std::is_same_v<IAudioMeterInformation, Interface>) {
				source = L"IAudioClient.Activate(IAudioMeterInformation)";
			} else if constexpr (std::is_same_v<IAudioSessionManager, Interface>) {
				source = L"IAudioClient.Activate(IAudioSessionManager)";
			} else if constexpr (std::is_same_v<IAudioSessionManager2, Interface>) {
				source = L"IAudioClient.Activate(IAudioSessionManager2)";
			} else if constexpr (std::is_same_v<IDeviceTopology, Interface>) {
				source = L"IAudioClient.Activate(IDeviceTopology)";
			} else {
				// ReSharper disable once CppStaticAssertFailure
				static_assert(false, "Interface is not supported by IMMDevice");
			}

			return {
				[&](auto ptr) {
					typedQuery(&IMMDevice::Activate, ptr, source, CLSCTX_INPROC_SERVER, nullptr);
				}
			};
		}

	private:
		[[nodiscard]]
		static sview convertFormFactor(std::optional<EndpointFormFactor> value) noexcept;

		/// <summary>
		/// Can throw ComException.
		/// </summary>
		void readType() noexcept(false);

		/// <summary>
		/// Can throw ComException.
		/// </summary>
		void readId() noexcept(false);
	};
}
