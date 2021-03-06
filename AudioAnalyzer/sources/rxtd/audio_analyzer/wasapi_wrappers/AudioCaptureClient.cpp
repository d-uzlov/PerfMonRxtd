// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Danil Uzlov

#include "AudioCaptureClient.h"

#include <type_traits>

using rxtd::audio_analyzer::wasapi_wrappers::AudioCaptureClient;

static_assert(std::is_same<BYTE, uint8_t>::value);
static_assert(std::is_same<UINT32, uint32_t>::value);

// static_assert(std::is_same<DWORD, uint32_t>::value); // ...

HRESULT AudioCaptureClient::readBuffer() {
	uint8_t* data = nullptr;
	DWORD flags{};
	uint32_t dataSize;
	HRESULT lastResult = ref().GetBuffer(&data, &dataSize, &flags, nullptr, nullptr);

	if (lastResult != S_OK) {
		return lastResult;
	}

	const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;

	buffer.setBuffersCount(channelsCount);
	buffer.setBufferSize(dataSize);

	if (silent) {
		buffer.fill(0.0);
	} else {
		switch (type) {
		case Type::eInt16:
			for (index i = 0; i < channelsCount; i++) {
				copyInt(data, buffer[i], i, channelsCount);
			}
			break;
		case Type::eFloat:
			for (index i = 0; i < channelsCount; i++) {
				copyFloat(data, buffer[i], i, channelsCount);
			}
			break;
		}
	}

	ref().ReleaseBuffer(dataSize);
	return S_OK;
}

void AudioCaptureClient::copyFloat(void* source, array_span<float> dest, index offset, index stride) {
	const auto bufferInt = static_cast<const float*>(source);
	const auto framesCount = dest.size();

	auto channelSourceBuffer = bufferInt + offset;

	for (index frame = 0; frame < framesCount; ++frame) {
		dest[frame] = *channelSourceBuffer;
		channelSourceBuffer += stride;
	}
}

void AudioCaptureClient::copyInt(void* source, array_span<float> dest, index offset, index stride) {
	const auto bufferInt = static_cast<const int16_t*>(source);
	const auto framesCount = dest.size();

	auto channelSourceBuffer = bufferInt + offset;

	for (index frame = 0; frame < framesCount; ++frame) {
		const float value = static_cast<float>(*channelSourceBuffer) * (1.0f / static_cast<float>(std::numeric_limits<int16_t>::max()));
		dest[frame] = value;
		channelSourceBuffer += stride;
	}
}
