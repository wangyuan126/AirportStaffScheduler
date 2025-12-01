#pragma once
#include <string>
#include <chrono>

namespace AirportStaffScheduler {
    using DateTime = std::chrono::system_clock::time_point;
    class TemporaryTask {
    public:
        // 构造函数：所有字段均为必填
        TemporaryTask(
            const std::string& shiftId,
            const std::string& taskName,
            const DateTime& startTime,
            const DateTime& endTime,
            bool isLocked  // Y -> true, N -> false
        )
            : shiftId_(shiftId)
            , taskName_(taskName)
            , startTime_(startTime)
            , endTime_(endTime)
            , isLocked_(isLocked)
        {}

        // Getters
        const std::string& getShiftId() const { return shiftId_; }
        const std::string& getTaskName() const { return taskName_; }
        const DateTime& getStartTime() const { return startTime_; }
        const DateTime& getEndTime() const { return endTime_; }
        bool isLocked() const { return isLocked_; }

        // Setters
        void setShiftId(const std::string& value) { shiftId_ = value; }
        void setTaskName(const std::string& value) { taskName_ = value; }
        void setStartTime(const DateTime& value) { startTime_ = value; }
        void setEndTime(const DateTime& value) { endTime_ = value; }
        void setLocked(bool value) { isLocked_ = value; }

    private:
        std::string shiftId_;     // 班次编号，必填
        std::string taskName_;    // 任务名，必填
        DateTime startTime_;   // 任务开始时间，格式 yyyy/mm/dd hh:mm，必填
        DateTime endTime_;     // 任务结束时间，格式 yyyy/mm/dd hh:mm，必填
        bool isLocked_;           // 是否锁定，Y=true, N=false，必填
    };
}