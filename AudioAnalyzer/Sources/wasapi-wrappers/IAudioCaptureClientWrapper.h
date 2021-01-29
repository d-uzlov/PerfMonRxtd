/*
 * Copyright (C) 2020-2021 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once

#include <Audioclient.h>

#include "Vector2D.h"
#include "winapi-wrappers/GenericComWrapper.h"

namespace rxtd::audio_analyzer::wasapi_wrappers {
	class IAudioCaptureClientWrapper : public common::winapi_wrappers::GenericComWrapper<IAudioCaptureClient> {
	public:
		enum class Type {
			eInt16,
			eFloat,
		};

	private:
		Type type{};
		index channelsCount{};

		utils::Vector2D<float> buffer;

	public:
		IAudioCaptureClientWrapper() = default;

		template<typename InitFunction>
		IAudioCaptureClientWrapper(InitFunction initFunction) : GenericComWrapper(std::move(initFunction)) { }

		void setParams(Type _type, index _channelsCount) {
			type = _type;
			channelsCount = _channelsCount;
		}

		HRESULT readBuffer();

		[[nodiscard]]
		utils::array2d_view<float> getBuffer() const {
			return buffer;
		}

	private:
		static void copyFloat(void* source, array_span<float> dest, index offset, index stride);

		static void copyInt(void* source, array_span<float> dest, index offset, index stride);
	};
}
