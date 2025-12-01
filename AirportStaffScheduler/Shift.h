// include/Core/Shift.h
#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <unordered_set>

namespace AirportStaffScheduler {
    using DateTime = std::chrono::system_clock::time_point;
    class Shift {
    public:
        // 构造函数（必填字段）
        Shift(
            const std::string& shiftId,
            const std::string& shiftName,
            const std::string& staffId,  
            const DateTime& startTime,
            const DateTime& endTime,
            const std::string& boundTerminal,
            bool allowOvertime,
            bool avoidAssignIfPossible  // Y -> true, N -> false
        )
            : shiftId_(shiftId)
            , shiftName_(shiftName)
            , staffId_(staffId)
            , startTime_(startTime)
            , endTime_(endTime)
            , boundTerminal_(boundTerminal)
            , taskTypePreferences_()
            , allowOvertime_(allowOvertime)
            , avoidAssignIfPossible_(avoidAssignIfPossible)
        {}

        // ===== Getters =====
        const std::string& getShiftId() const { return shiftId_; }
        const std::string& getShiftName() const { return shiftName_; }
        const std::string& getStaffId() const { return staffId_; }
        const DateTime& getStartTime() const { return startTime_; }
        const DateTime& getEndTime() const { return endTime_; }
        const std::string& getBoundTerminal() const { return boundTerminal_; }
        const std::vector<std::string>& getTaskTypePreferences() const { return taskTypePreferences_; }
        bool isAllowOvertime() const { return allowOvertime_; }
        bool isAvoidAssignIfPossible() const { return avoidAssignIfPossible_; }
        const DateTime& getLatestEndTime() const { return latestEndTime_; }

        // ===== Setters =====
        void setShiftId(const std::string& id) { shiftId_ = id; }
        void setShiftName(const std::string& name) { shiftName_ = name; }
        void setStaffIds(const std::string& ids) { staffId_ = ids; }
        void setStartTime(const DateTime& time) { startTime_ = time; }
        void setEndTime(const DateTime& time) { endTime_ = time; }
        void setBoundTerminal(const std::string& terminal) { boundTerminal_ = terminal; }
        void setTaskTypePreferences(const std::vector<std::string>& prefs) { taskTypePreferences_ = prefs; }
        void setAllowOvertime(bool allow) { allowOvertime_ = allow; }
        void setAvoidAssignIfPossible(bool avoid) { avoidAssignIfPossible_ = avoid; }
        void setLatestEndTime(const DateTime& time) { latestEndTime_ = time; }
        void setQualifications(const std::vector<std::string>& quals) {
            qualifications_.clear();
            qualifications_.reserve(quals.size()); // 避免多次 rehash
            for (const auto& q : quals) {
                qualifications_.insert(q);
            }
            // 注意：vector 中的重复资质会被自动去重
        }

        bool hasAllQualifications(const std::vector<std::string>& required) const {
            for (const auto& qual : required) {
                if (qualifications_.count(qual) == 0) {
                    return false; // 缺少某项资质
                }
            }
            return true; // 全部满足
        }
        void assignTask(const Task& task) {
            if (task.getTaskEndTime() > latestEndTime_) {
                latestEndTime_ = task.getTaskEndTime();
            }
        }

    private:
        std::string shiftId_;
        std::string shiftName_;
        std::string staffId_;              // 必填，员工ID
        DateTime startTime_;                          // yyyy/mm/dd hh:mm
        DateTime endTime_;                            // yyyy/mm/dd hh:mm
        std::string boundTerminal_;                      // 绑定的航站楼
        std::vector<std::string> taskTypePreferences_;   // 可空，任务类型偏好列表
        bool allowOvertime_;                             // 是否允许加班
        bool avoidAssignIfPossible_;                     // Y -> true（尽量不派工）
        DateTime latestEndTime_;               // 已分配任务中最晚结束时间
        std::unordered_set<std::string> qualifications_; // 班次拥有资质
    };

} // namespace AirportStaffScheduler