#include "DateTimeUtils.h"
#include <sstream>
#include <iomanip>

#ifdef _MSC_VER
#include <io.h>
#else
#include <ctime>
#endif

namespace AirportStaffScheduler {
    namespace Utils {

        std::string FormatDateTime(std::chrono::system_clock::time_point tp) {
            auto tt = std::chrono::system_clock::to_time_t(tp);
            std::tm tm{};
#ifdef _MSC_VER
            localtime_s(&tm, &tt);
#else
            localtime_r(&tt, &tm);
#endif
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        std::chrono::system_clock::time_point ParseDateTime(const std::string& dtStr) {
            if (dtStr.empty()) {
                throw std::invalid_argument("DateTime string is empty");
            }
            std::tm tm = {};
            std::istringstream ss(dtStr);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) {
                throw std::runtime_error("Failed to parse datetime: " + dtStr);
            }
            return std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }

    } // namespace Utils
} // namespace AirportStaffScheduler