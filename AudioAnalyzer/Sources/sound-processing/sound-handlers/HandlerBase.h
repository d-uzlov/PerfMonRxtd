/*
 * Copyright (C) 2019-2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

/*
 Handler life cycle
 First of all, handler should parse its parameters. It happens in the method #parseParams(),
 which is called on an empty object.
 If params are invalid, then parseParams should return invalid object,
 so that handler with these params will not be created.
 Then the main life cycle happens:
 1. Function #setParams is called.
		This function should only save parameters, any external info in the moment of time when this function is called is considered invalid
 2. Function #vConfigure is called
		At this point handler should do all calculations required to work.
		Handler may call #vConfigure to get external info
		If something fails, handler should return invalid object, and the it is invalidated until next params change
 3. vProcess happens in the loop
 4. Data is accessed from other handlers and child measures.
 */

#pragma once
#include <chrono>
#include <utility>

#include "BufferPrinter.h"
#include "ExternalMethods.h"
#include "ParamsContainer.h"
#include "Vector2D.h"
#include "../../Version.h"
#include "option-parsing/OptionMap.h"
#include "rainmeter/Rainmeter.h"


namespace rxtd::audio_analyzer::handler {
	class HandlerBase;
}

namespace rxtd::audio_analyzer {
	class HandlerFinder {
	public:
		virtual ~HandlerFinder() = default;

		[[nodiscard]]
		virtual handler::HandlerBase* getHandler(isview id) const = 0;
	};
}

namespace rxtd::audio_analyzer::handler {
	class HandlerBase : VirtualDestructorBase {
	public:
		using Option = common::options::Option;
		using OptionMap = common::options::OptionMap;
		using OptionList = common::options::OptionList;
		using Rainmeter = common::rainmeter::Rainmeter;
		using Logger = common::rainmeter::Logger;
		using clock = std::chrono::high_resolution_clock;
		static_assert(clock::is_steady);
		using BufferPrinter = common::buffer_printer::BufferPrinter;

		class TooManyValuesException : public std::runtime_error {
			string sourceName;
		public:
			explicit TooManyValuesException(string sourceName) : runtime_error(""), sourceName(std::move(sourceName)) {}

			[[nodiscard]]
			sview getSourceName() const {
				return sourceName;
			}
		};

		class InvalidOptionsException : public std::runtime_error {
		public:
			explicit InvalidOptionsException() : runtime_error("") {}
		};

		struct DataSize {
			index valuesCount{};
			index layersCount{};
			std::vector<index> eqWaveSizes;

			DataSize() = default;

			DataSize(index valuesCount, std::vector<index> _eqWaveSizes) :
				valuesCount(valuesCount),
				eqWaveSizes(std::move(_eqWaveSizes)) {
				layersCount = eqWaveSizes.size();
			}

			[[nodiscard]]
			bool isEmpty() const {
				return eqWaveSizes.empty() || valuesCount == 0;
			}
		};

		struct ProcessContext {
			array_view<float> wave;
			array_view<float> originalWave;
			clock::time_point killTime;
		};

		struct HandlerMetaInfo {
			using handlerUptr = std::unique_ptr<HandlerBase>;
			using TransformFun = handlerUptr(*)(handlerUptr old);

			bool valid = false;
			ParamsContainer params;
			// std::vector<istring> sources;
			TransformFun transform = [](handlerUptr ptr) -> handlerUptr { return {}; };
			ExternalMethods externalMethods{};

			HandlerMetaInfo() : HandlerMetaInfo(false) {}

			explicit HandlerMetaInfo(bool isValid) {
				valid = isValid;
			}
		};

		struct Snapshot {
			utils::Vector2D<float> values;
			ExternalData handlerSpecificData;
		};

	protected:
		struct Configuration {
			HandlerBase* sourcePtr = nullptr;
			index sampleRate{};
			Version version{};

			// autogenerated
			friend bool operator==(const Configuration& lhs, const Configuration& rhs) {
				return lhs.sourcePtr == rhs.sourcePtr
					&& lhs.sampleRate == rhs.sampleRate
					&& lhs.version == rhs.version;
			}

			friend bool operator!=(const Configuration& lhs, const Configuration& rhs) {
				return !(lhs == rhs);
			}
		};

		struct ConfigurationResult {
			bool success = false;
			DataSize dataSize;

			ConfigurationResult() = default;

			ConfigurationResult(DataSize dataSize): success(true), dataSize(std::move(dataSize)) { }

			ConfigurationResult(index valuesCount, std::vector<index> eqWaveSizes):
				ConfigurationResult(DataSize{ valuesCount, std::move(eqWaveSizes) }) { }
		};

	private:
		struct LayerCache {
			mutable std::vector<array_view<float>> chunksView;
			std::vector<index> offsets;
		};

		string _name;
		bool _anyChanges = false;
		DataSize _dataSize;
		mutable bool _layersAreValid = false;
		std::vector<float> _buffer;
		std::vector<LayerCache> _layers;
		utils::Vector2D<float> _lastResults;

		Configuration _configuration{};

	public:
		/// <summary>
		/// Creates all the necessary meta info to create an object of a class, patch it and use it.
		/// </summary>
		/// <typeparam name="Type">Type of the handler implementation. Must be a descendant from HandlerBase class.</typeparam>
		/// <param name="om">Map to read options from.</param>
		/// <param name="cl">Logger.</param>
		/// <param name="rain">Rainmeter object for possible communication with outside world. One such example would be to read current directory for current skin.</param>
		/// <param name="version">Current version of the plugin API.</param>
		/// <returns>Valid HandlerMetaInfo object.</returns>
		template<typename Type>
		[[nodiscard]]
		static HandlerMetaInfo createMetaForClass(const OptionMap& om, Logger& cl, const Rainmeter& rain, Version version) {
			using HandlerType = Type;
			HandlerMetaInfo meta;

			meta.transform = patchHandlerImpl<HandlerType>;

			HandlerType instance;
			HandlerBase& ref = instance;

			meta.params = ref.vParseParams(om, cl, rain, version);
			meta.externalMethods.finish = ref.vGetExt_finish();
			meta.externalMethods.getProp = ref.vGetExt_getProp();

			meta.valid = true;

			return meta;
		}

	protected:
		[[nodiscard]]
		virtual ExternalMethods::FinishMethodType vGetExt_finish() const {
			return [](const ExternalData&, const ExternalMethods::CallContext&) {};
		}

		[[nodiscard]]
		virtual ExternalMethods::GetPropMethodType vGetExt_getProp() const {
			return [](const ExternalData&, isview, BufferPrinter&, const ExternalMethods::CallContext&) -> bool {
				return {};
			};
		}

		/// <summary>
		/// Reads options from map and creates a ParamsContainer object.
		/// Implementation is allowed to throw InvalidOptionsException.
		/// </summary>
		/// <param name="om">Map to read options from.</param>
		/// <param name="cl">Logger.</param>
		/// <param name="rain">Rainmeter object for possible communication with outside world. One such example would be to read current directory for current skin.</param>
		/// <param name="version">Current version of the plugin API.</param>
		/// <returns>ParamsContainer, that is valid for vConfigure call on the same object.</returns>
		[[nodiscard]]
		virtual ParamsContainer vParseParams(const OptionMap& om, Logger& cl, const Rainmeter& rain, Version version) const noexcept(false) = 0;

		// if handler is potentially heavy,
		// handler should try to return control to caller
		// when time is more than context.killTime
		virtual void vProcess(ProcessContext context, ExternalData& handlerSpecificData) = 0;

		[[nodiscard]]
		virtual ConfigurationResult vConfigure(
			const ParamsContainer& _params, Logger& cl,
			ExternalData& externalData
		) = 0;

		// should return true when params are the same
		[[nodiscard]]
		virtual bool vCheckSameParams(const ParamsContainer& p) const = 0;

	public:
		// returns true on success, false on invalid handler
		[[nodiscard]]
		bool patch(
			sview name,
			const ParamsContainer& params,
			array_view<istring> sources,
			index sampleRate, Version version,
			HandlerFinder& hf, Logger& cl,
			Snapshot& snapshot
		);

		void finishConfiguration() {
			_anyChanges = false;
		}

		void process(ProcessContext context, Snapshot& snapshot) {
			clearChunks();

			vProcess(context, snapshot.handlerSpecificData);

			for (index layer = 0; layer < index(_dataSize.eqWaveSizes.size()); layer++) {
				auto& offsets = _layers[layer].offsets;
				if (!offsets.empty()) {
					const auto lastChunk = array_view<float>{ _buffer.data() + offsets.back(), _dataSize.valuesCount };
					snapshot.values[layer].copyFrom(lastChunk);
				} else {
					snapshot.values[layer].copyFrom(_lastResults[layer]);
				}
			}
		}

		// following public members are public for access between handlers
		[[nodiscard]]
		const DataSize& getDataSize() const {
			return _dataSize;
		}

		[[nodiscard]]
		virtual index getStartingLayer() const {
			return _configuration.sourcePtr == nullptr ? 0 : _configuration.sourcePtr->getStartingLayer();
		}

		[[nodiscard]]
		array_view<array_view<float>> getChunks(index layer) const {
			if (layer >= index(_dataSize.eqWaveSizes.size())) {
				return {};
			}

			inflateLayers();
			return _layers[layer].chunksView;
		}

		// returns saved data from previous iteration
		[[nodiscard]]
		array_view<float> getSavedData(index layer) const {
			if (layer >= index(_dataSize.eqWaveSizes.size())) {
				return {};
			}

			return _lastResults[layer];
		}


	protected:
		template<typename Params>
		static bool compareParamsEquals(const Params& p1, const ParamsContainer& p2) {
			return p1 == p2.cast<Params>();
		}

		template<typename DataStructType, auto methodPtr>
		static ExternalMethods::FinishMethodType wrapExternalFinish() {
			static_assert(
				std::is_invocable<decltype(methodPtr), const DataStructType&, const ExternalMethods::CallContext&>::value,
				"Method doesn't match the required signature."
			);
			return [](const ExternalData& dataWrapper, const ExternalMethods::CallContext& context) {
				return methodPtr(dataWrapper.cast<DataStructType>(), context);
			};
		}

		template<typename DataStructType, auto methodPtr>
		static ExternalMethods::GetPropMethodType wrapExternalGetProp() {
			static_assert(
				std::is_invocable_r<bool, decltype(methodPtr), const DataStructType&, isview, BufferPrinter&, const ExternalMethods::CallContext&>::value,
				"Method doesn't match the required signature."
			);
			return [](
				const ExternalData& dataWrapper,
				isview prop,
				BufferPrinter& bp,
				const ExternalMethods::CallContext& context
			) {
				return methodPtr(dataWrapper.cast<DataStructType>(), prop, bp, context);
			};
		}

		[[nodiscard]]
		const Configuration& getConfiguration() const {
			return _configuration;
		}

		[[nodiscard]]
		array_span<float> pushLayer(index layer) {
			const index offset = index(_buffer.size());

			// Prevent handlers from producing too much data
			// see: https://github.com/d-uzlov/Rainmeter-Plugins-by-rxtd/issues/4
			if (offset + _dataSize.valuesCount > 1'000'000) {
				throw TooManyValuesException{ _name };
			}

			_buffer.resize(offset + _dataSize.valuesCount);
			_layersAreValid = false;

			_layers[layer].offsets.push_back(offset);

			return { _buffer.data() + offset, _dataSize.valuesCount };
		}

		static index legacy_parseIndexProp(const isview& request, const isview& propName, index endBound) {
			return legacy_parseIndexProp(request, propName, 0, endBound);
		}

		static index legacy_parseIndexProp(
			const isview& request,
			const isview& propName,
			index minBound, index endBound
		);

	private:
		template<typename Type>
		[[nodiscard]]
		static std::unique_ptr<HandlerBase> patchHandlerImpl(std::unique_ptr<HandlerBase> handlerPtr) {
			using HandlerType = Type;

			if (dynamic_cast<HandlerType*>(handlerPtr.get()) == nullptr) {
				handlerPtr = std::make_unique<HandlerType>();
			}

			return handlerPtr;
		}

		void inflateLayers() const {
			if (_layersAreValid) {
				return;
			}

			for (auto& data : _layers) {
				data.chunksView.resize(data.offsets.size());
				for (index i = 0; i < index(data.offsets.size()); i++) {
					data.chunksView[i] = { _buffer.data() + data.offsets[i], _dataSize.valuesCount };
				}
			}

			_layersAreValid = true;
		}

		void clearChunks() {
			for (index layer = 0; layer < _dataSize.layersCount; layer++) {
				auto& offsets = _layers[layer].offsets;
				if (!offsets.empty()) {
					_lastResults[layer].copyFrom({ _buffer.data() + offsets.back(), _dataSize.valuesCount });
				}
				offsets.clear();
			}

			_layersAreValid = false;

			_buffer.clear();
		}
	};
}
