/*
 * Copyright (C) 2020 rxtd
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
 */

#pragma once
#include "../SoundHandler.h"
#include "../../../audio-utils/CustomizableValueTransformer.h"
#include "ResamplerProvider.h"

namespace rxtd::audio_analyzer {
	class SingleValueTransformer : public ResamplerProvider {
		using CVT = audio_utils::CustomizableValueTransformer;

		struct Params {
			CVT transformer;

			// autogenerated
			friend bool operator==(const Params& lhs, const Params& rhs) {
				return lhs.transformer == rhs.transformer;
			}

			friend bool operator!=(const Params& lhs, const Params& rhs) {
				return !(lhs == rhs);
			}
		};

		Params params{};
		std::vector<CVT> transformersPerLayer;
		std::vector<index> countersPerLayer;

	public:
		[[nodiscard]]
		bool checkSameParams(const ParamsContainer& p) const override {
			return compareParamsEquals(params, p);
		}

		[[nodiscard]]
		ParseResult parseParams(
			const OptionMap& om, Logger& cl, const Rainmeter& rain,
			index legacyNumber
		) const override;

	protected:
		[[nodiscard]]
		ConfigurationResult vConfigure(const ParamsContainer& _params, Logger& cl, ExternalData& externalData) override;

	public:
		void vProcess(ProcessContext context, ExternalData& externalData) override;
	};
}
