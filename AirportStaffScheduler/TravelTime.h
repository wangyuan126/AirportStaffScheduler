#pragma once

#include <string>

namespace AirportStaffScheduler {
    class TravelTime {
    public:
        // 构造函数：所有字段均为必填
        TravelTime(
            const std::string& taskOneId,
            const std::string& taskTwoId,
            int travelMinutes
        )
            : taskOneId_(taskOneId)
            , taskTwoId_(taskTwoId)
            , travelMinutes_(travelMinutes)
        {}

        // Getters
        const std::string& getTaskOneId() const { return taskOneId_; }
        const std::string& getTaskTwoId() const { return taskTwoId_; }
        int getTravelMinutes() const { return travelMinutes_; }

        // Setters
        void setTaskOneId(const std::string& value) { taskOneId_ = value; }
        void setTaskTwoId(const std::string& value) { taskTwoId_ = value; }
        void setTravelMinutes(int value) { travelMinutes_ = value; }

    private:
        std::string taskOneId_;   // 任务一ID，必填
        std::string taskTwoId_;   // 任务二ID，必填
        int travelMinutes_;       // 路程时间（分钟），必填
    };
} // namespace AirportStaffScheduler