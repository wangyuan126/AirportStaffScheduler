#pragma once

#include <string>
#include <chrono>

namespace AirportStaffScheduler {
	namespace Utils {
		std::string FormatDateTime(std::chrono::system_clock::time_point tp);
		std::chrono::system_clock::time_point ParseDateTime(const std::string& dtStr);

	} // namespace Utils
} // namespace AirportStaffScheduler