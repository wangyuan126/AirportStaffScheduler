#pragma once
#include <string>
#include <chrono>

namespace AirportStaffScheduler {
    using DateTime = std::chrono::system_clock::time_point;

    class FlightSchedule {
    public:
        // 构造函数：仅包含“是否可空 = 否”的字段（只有航班ID）
        FlightSchedule(
            const std::string& flightId,
            DateTime boardingStartTime,
            DateTime cabinOpenTime,
            DateTime cabinCloseTime,
            DateTime pushbackTime,
            DateTime bridgeRetractTime,
            DateTime uctot
        )
            : flightId_(flightId)
            , boardingStartTime_(boardingStartTime)
            , cabinOpenTime_(cabinOpenTime)
            , cabinCloseTime_(cabinCloseTime)
            , pushbackTime_(pushbackTime)
            , bridgeRetractTime_(bridgeRetractTime)
            , uctot_(uctot)
        {}

        // Getters
        const std::string& getFlightId() const { return flightId_; }
        const DateTime& getBoardingStartTime() const { return boardingStartTime_; }
        const DateTime& getCabinOpenTime() const { return cabinOpenTime_; }
        const DateTime& getCabinCloseTime() const { return cabinCloseTime_; }
        const DateTime& getPushbackTime() const { return pushbackTime_; }
        const DateTime& getBridgeRetractTime() const { return bridgeRetractTime_; }
        const DateTime& getUctot() const { return uctot_; }

        // Setters
        void setFlightId(const std::string& value) { flightId_ = value; }
        void setBoardingStartTime(const DateTime& value) { boardingStartTime_ = value; }
        void setCabinOpenTime(const DateTime& value) { cabinOpenTime_ = value; }
        void setCabinCloseTime(const DateTime& value) { cabinCloseTime_ = value; }
        void setPushbackTime(const DateTime& value) { pushbackTime_ = value; }
        void setBridgeRetractTime(const DateTime& value) { bridgeRetractTime_ = value; }
        void setUctot(const DateTime& value) { uctot_ = value; }

    private:
        std::string flightId_;             // 航班ID，必填
        DateTime boardingStartTime_;    // 开始登机，可空
        DateTime cabinOpenTime_;        // 客舱开启，可空
        DateTime cabinCloseTime_;       // 客舱关闭，可空
        DateTime pushbackTime_;         // 开车时间，可空
        DateTime bridgeRetractTime_;    // 撤桥时间，可空
        DateTime uctot_;                // UCTOT，可空
    };
}