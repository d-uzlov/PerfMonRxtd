// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2019 Danil Uzlov

#pragma once

#include "rxtd/buffer_printer/BufferPrinter.h"
#include "rxtd/std_fixes/AnyContainer.h"

namespace rxtd {
	//
	// Basic logger.
	// Supports formatting using BufferPrinter.
	// Can hold a default prefix for messages.
	// If logging is not needed, logger can be forcefully silenced.
	//
	class Logger {
	public:
		enum class LogLevel {
			eERROR,
			eWARNING,
			eNOTICE,
			eDEBUG,
		};

		using WriteFunction = void(*)(std_fixes::AnyContainer& dataContainer, LogLevel, sview message);

	private:
		using BufferPrinter = buffer_printer::BufferPrinter;

		mutable std_fixes::AnyContainer dataContainer;
		mutable BufferPrinter printer;
		WriteFunction write = nullptr;
		string prefix{};

	public:
		Logger() = default;

		explicit Logger(std_fixes::AnyContainer dataContainer, WriteFunction writeFunction, string prefix = {}) :
			dataContainer(std::move(dataContainer)), write(writeFunction), prefix(std::move(prefix)) { }

		template<typename... Args>
		void error(sview formatString, const Args&... args) const {
			log(LogLevel::eERROR, formatString, args...);
		}

		template<typename... Args>
		void warning(sview formatString, const Args&... args) const {
			log(LogLevel::eWARNING, formatString, args...);
		}

		template<typename... Args>
		void notice(sview formatString, const Args&... args) const {
			log(LogLevel::eNOTICE, formatString, args...);
		}

		template<typename... Args>
		void debug(sview formatString, const Args&... args) const {
			log(LogLevel::eDEBUG, formatString, args...);
		}

		template<typename... Args>
		[[nodiscard]]
		Logger context(const wchar_t* formatString, const Args&... args) const {
			printer.print(formatString, args...);
			string newPrefix = prefix;
			newPrefix += printer.getBufferView();
			printer.reset();

			return Logger{ dataContainer, write, std::move(newPrefix) };
		}

		[[nodiscard]]
		static Logger getSilent() {
			Logger result{};
			return result;
		}

	private:
		template<typename... Args>
		void log(LogLevel logLevel, sview formatString, const Args&... args) const {
			if (write == nullptr) {
				return;
			}

			printer.print(prefix.c_str());

			printer.append(formatString, args...);

			write(dataContainer, logLevel, printer.getBufferView());

			printer.reset();
		}
	};
}
