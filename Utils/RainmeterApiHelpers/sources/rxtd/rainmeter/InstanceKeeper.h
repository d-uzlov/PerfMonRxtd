// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Danil Uzlov

#pragma once

#include "DataHandle.h"
#include "SkinHandle.h"

namespace rxtd::rainmeter {
	//
	// This class acts as a proxy for Rainmeter API functions RmLog and RmExecute
	//
	// Since Rainmeter plugins are dynamically loaded and unloaded, after a Rainmeter Skin is unloaded all of the plugins it used are unloaded,
	// and if it was the last skin using this plugin then memory of the plugin will be freed.
	// If there were background threads running then these threads will be running in freed memory, so the program will crash.
	// Therefore, the plugin must wait for its background threads to finish before the plugin is unloaded.
	// Plugins are notified before unloading by function Finalize() (see dllmain.cpp), which is called by the main thread.
	//
	// Functions RmLog and RmExecute are allowed to be called concurrently, however they are blocking.
	// Moreover, they block on the same mutex as the main thread. This unfortunate side effect can cause a deadlock.
	//
	// If background thread was blocked on RmLog or RmExecute call, and main thread is in the Finalize() waiting for background thread, then deadlock happens.
	// #sendLog and #sendCommand are not blocking, so they can't cause deadlock.
	//
	// This class uses WinAPI functions to delay dll unloading.
	// As far as I'm aware, this is only possible by using Windows threads API,
	// which is inconvenient and non-portable, so I encapsulated its usage in this class, so I never ever have to look at it again.
	//
	// See generic issue explanation:
	//	https://devblogs.microsoft.com/oldnewthing/20131105-00/?p=2733
	// See my issue "ticket" that doesn't have any answer from Rainmeter devs at the moment of writing this comment:
	//	https://forum.rainmeter.net/viewtopic.php?f=14&t=35948&p=182576
	//
	// Usage:
	//
	// Keep at least one instance of this class for as long as you need to use RmLog or RmExecute through it.
	// Classes rainmeter::Rainmeter and rainmeter::Logger already hold an instance each.
	//
	class InstanceKeeper final {
	public:
		struct Message {
			// must return true when need to stop the thread
			using Action = bool(*)(const Message& message);
			Action action = nullptr;
			string messageText;
			int logLevel = 0;
			DataHandle dataHandle;
			SkinHandle skinHandle;
		};

		InstanceKeeper() {
			incrementCounter(nullptr);
		}

		explicit InstanceKeeper(DataHandle handle) {
			incrementCounter(handle.getRawHandle());

		}

		~InstanceKeeper() {
			decrementCounter();
		}

		InstanceKeeper(const InstanceKeeper& other) {
			incrementCounter(nullptr);
		}

		InstanceKeeper(InstanceKeeper&& other) noexcept {
			incrementCounter(nullptr);
		}

		InstanceKeeper& operator=(const InstanceKeeper& other) = default;

		InstanceKeeper& operator=(InstanceKeeper&& other) noexcept = default;

		void sendLog(DataHandle handle, string message, int level) const;
		void sendCommand(DataHandle handle, SkinHandle skin, string command);

	private:
		void incrementCounter(void* data);
		void decrementCounter();

		static void initThread(void* rm);
		void sendMessage(Message message) const;
	};
}
