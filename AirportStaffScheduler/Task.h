// include/Core/Task.h
#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace AirportStaffScheduler {
    using DateTime = std::chrono::system_clock::time_point;
    class Task {
    public:
        // 构造函数（仅包含必填字段）
        Task(
            const std::string& taskId,
            const std::string& taskDefId,
            const std::string& taskName,
            const DateTime& taskStartTime,
            const DateTime& taskEndTime,
            int durationMinutes,
            const std::string& inOrOutPort,
            const std::string& flightAttribute,
            bool mustAssign,
            bool isLocked,
            const DateTime& arrivalEstimatedLandingTime,
            const DateTime& arrivalScheduledLandingTime,
            const DateTime& departureScheduledTakeoffTime,
            const DateTime& departureEstimatedTakeoffTime
        )
            : taskId_(taskId)
            , taskDefId_(taskDefId)
            , taskName_(taskName)
            , taskDate_("")
            , taskStartTime_(taskStartTime)
            , taskEndTime_(taskEndTime)
            , durationMinutes_(durationMinutes)
            , inOrOutPort_(inOrOutPort)
            , arrivalFlightId_("")
            , arrivalEstimatedLandingTime_(arrivalEstimatedLandingTime)
            , arrivalScheduledLandingTime_(arrivalScheduledLandingTime)
            , departureFlightId_("")
            , departureScheduledTakeoffTime_(departureScheduledTakeoffTime)
            , departureEstimatedTakeoffTime_(departureEstimatedTakeoffTime)
            , arrivalFlightNumber_("")
            , departureFlightNumber_("")
            , flightType_("")
            , flightAttribute_(flightAttribute)
            , aircraftType_("")
            , requiredQualifications_()  // 初始化为空 vector
            , terminal_("")
            , area_("")
            , gatePosition_("")
            , boardingGate_("")
            , mustAssign_(mustAssign)
            , linkedTaskId_("")
            , genderRequirement_("")
            , isLocked_(isLocked)
            , assignedStaffId_("")
            , counterNumber_("")
            , relatedAirline_("")
            , requiredVehicleType_("")
        {}

        // ===== Getters =====
        const std::string& getTaskId() const { return taskId_; }
        const std::string& getTaskDefId() const { return taskDefId_; }
        const std::string& getTaskName() const { return taskName_; }
        const std::string& getTaskDate() const { return taskDate_; }
        const DateTime& getTaskStartTime() const { return taskStartTime_; }
        const DateTime& getTaskEndTime() const { return taskEndTime_; }
        int getDurationMinutes() const { return durationMinutes_; }
        const std::string& getInOrOutPort() const { return inOrOutPort_; }

        const std::string& getArrivalFlightId() const { return arrivalFlightId_; }
        const DateTime& getArrivalEstimatedLandingTime() const { return arrivalEstimatedLandingTime_; }
        const DateTime& getArrivalScheduledLandingTime() const { return arrivalScheduledLandingTime_; }

        const std::string& getDepartureFlightId() const { return departureFlightId_; }
        const DateTime& getDepartureScheduledTakeoffTime() const { return departureScheduledTakeoffTime_; }
        const DateTime& getDepartureEstimatedTakeoffTime() const { return departureEstimatedTakeoffTime_; }

        const std::string& getArrivalFlightNumber() const { return arrivalFlightNumber_; }
        const std::string& getDepartureFlightNumber() const { return departureFlightNumber_; }

        const std::string& getFlightType() const { return flightType_; }
        const std::string& getFlightAttribute() const { return flightAttribute_; }
        const std::string& getAircraftType() const { return aircraftType_; }

        const std::vector<std::string>& getRequiredQualifications() const {return requiredQualifications_;}
        const std::string& getTerminal() const { return terminal_; }
        const std::string& getArea() const { return area_; }
        const std::string& getGatePosition() const { return gatePosition_; }
        const std::string& getBoardingGate() const { return boardingGate_; }

        bool isMustAssign() const { return mustAssign_; }
        const std::string& getLinkedTaskId() const { return linkedTaskId_; }
        const std::string& getGenderRequirement() const { return genderRequirement_; }
        bool isLocked() const { return isLocked_; }

        const std::string& getAssignedStaffId() const { return assignedStaffId_; }
        const std::string& getCounterNumber() const { return counterNumber_; }
        const std::string& getRelatedAirline() const { return relatedAirline_; }
        const std::string& getRequiredVehicleType() const { return requiredVehicleType_; }

        // ===== Setters =====
        void setTaskId(const std::string& id) { taskId_ = id; }
        void setTaskDefId(const std::string& id) { taskDefId_ = id; }
        void setTaskName(const std::string& name) { taskName_ = name; }
        void setTaskDate(const std::string& date) { taskDate_ = date; }
        void setTaskStartTime(const DateTime& time) { taskStartTime_ = time; }
        void setTaskEndTime(const DateTime& time) { taskEndTime_ = time; }
        void setDurationMinutes(int minutes) { durationMinutes_ = minutes; }
        void setInOrOutPort(const std::string& port) { inOrOutPort_ = port; }
        void setArrivalFlightId(const std::string& id) { arrivalFlightId_ = id; }
        void setArrivalEstimatedLandingTime(const DateTime& time) { arrivalEstimatedLandingTime_ = time; }
        void setArrivalScheduledLandingTime(const DateTime& time) { arrivalScheduledLandingTime_ = time; }
        void setDepartureFlightId(const std::string& id) { departureFlightId_ = id; }
        void setDepartureScheduledTakeoffTime(const DateTime& time) { departureScheduledTakeoffTime_ = time; }
        void setDepartureEstimatedTakeoffTime(const DateTime& time) { departureEstimatedTakeoffTime_ = time; }
        void setArrivalFlightNumber(const std::string& number) { arrivalFlightNumber_ = number; }
        void setDepartureFlightNumber(const std::string& number) { departureFlightNumber_ = number; }
        void setFlightType(const std::string& type) { flightType_ = type; }
        void setFlightAttribute(const std::string& attr) { flightAttribute_ = attr; }
        void setAircraftType(const std::string& type) { aircraftType_ = type; }
        void setRequiredQualifications(const std::vector<std::string>& quals) { requiredQualifications_ = quals; }
        void setTerminal(const std::string& terminal) { terminal_ = terminal; }
        void setArea(const std::string& area) { area_ = area; }
        void setGatePosition(const std::string& pos) { gatePosition_ = pos; }
        void setBoardingGate(const std::string& gate) { boardingGate_ = gate; }
        void setMustAssign(bool must) { mustAssign_ = must; }
        void setLinkedTaskId(const std::string& id) { linkedTaskId_ = id; }
        void setGenderRequirement(const std::string& gender) { genderRequirement_ = gender; }
        void setLocked(bool locked) { isLocked_ = locked; }
        void setAssignedStaffId(const std::string& shiftId) { assignedStaffId_ = shiftId; }
        void setCounterNumber(const std::string& counter) { counterNumber_ = counter; }
        void setRelatedAirline(const std::string& airline) { relatedAirline_ = airline; }
        void setRequiredVehicleType(const std::string& vehicleType) { requiredVehicleType_ = vehicleType; }

    private:
        // 必填字段（不可为空）
        std::string taskId_;
        std::string taskDefId_;
        std::string taskName_;
        DateTime taskStartTime_;      // 格式：yyyy/mm/dd hh:mm
        DateTime taskEndTime_;      // 格式：yyyy/mm/dd hh:mm
        int durationMinutes_;
        std::string inOrOutPort_;        // "进港" 或 "出港"
        std::string flightAttribute_;    // "国际" / "国内" / "地区"
        bool mustAssign_;                // Y -> true, N -> false
        bool isLocked_;                  // 锁定状态

        DateTime arrivalEstimatedLandingTime_;
        DateTime arrivalScheduledLandingTime_;
        DateTime departureScheduledTakeoffTime_;
        DateTime departureEstimatedTakeoffTime_;

        // 可空字段（用空字符串表示空值）
        std::string taskDate_;                           // yyyy/mm/dd
        std::string arrivalFlightId_;
        std::string departureFlightId_;
        std::string arrivalFlightNumber_;
        std::string departureFlightNumber_;
        std::string flightType_;          // "短过站" / "长过站"
        std::string aircraftType_;        // "宽体机" / "窄体机"
        std::vector<std::string> requiredQualifications_; // 所需资质列表
        std::string terminal_;
        std::string area_;
        std::string gatePosition_;
        std::string boardingGate_;
        std::string linkedTaskId_;
        std::string genderRequirement_;   // "男" / "女" / ""（无要求）
        std::string assignedStaffId_;
        std::string counterNumber_;       // 值机柜台编号
        std::string relatedAirline_;      // 航司代码或名称
        std::string requiredVehicleType_; 
    };

} // namespace AirportStaffScheduler