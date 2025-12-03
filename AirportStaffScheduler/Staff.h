// include/Core/Staff.h
#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace AirportStaffScheduler {

    class Staff {
    public:
        // 构造函数（仅必填字段）
        Staff(
            const std::string& staffId,
            const std::string& name,
            const std::string& gender,
            const std::vector<std::string>& qualifications,  // 接收 vector，内部转为 unordered_set
            DateTime startTime,
            DateTime endTime
        )
            : staffId_(staffId)
            , name_(name)
            , gender_(gender)
            , startTime_(startTime)
            , endTime_(endTime)
            , latestEndTime_(startTime)  // 初始为班次开始时间
            , allowOvertime_(false)
            , avoidAssignIfPossible_(false)
        {
            // 转换资质 vector → unordered_set
            qualifications_.reserve(qualifications.size());
            for (const auto& q : qualifications) {
                qualifications_.insert(q);
            }
        }


        // ===== Getters =====
        const std::string& getStaffId() const { return staffId_; }
        const std::string& getName() const { return name_; }
        const std::string& getGender() const { return gender_; }
        const std::unordered_set<std::string>& getQualifications() const { return qualifications_; }
        const std::string& getTeamName() const { return teamName_; }
        const std::vector<std::string>& getRelatedStaffIds() const { return relatedStaffIds_; }
        const std::string& getShiftName() const { return shiftName_; }
        DateTime getStartTime() const { return startTime_; }
        DateTime getEndTime() const { return endTime_; }
        const std::string& getBoundTerminal() const { return boundTerminal_; }
        const std::vector<std::string>& getTaskTypePreferences() const { return taskTypePreferences_; }
        bool getAllowOvertime() const { return allowOvertime_; }
        bool getAvoidAssignIfPossible() const { return avoidAssignIfPossible_; }
        DateTime getLatestEndTime() const { return latestEndTime_; }

        // ===== Setters =====
        void setStaffId(const std::string& id) { staffId_ = id; }
        void setName(const std::string& name) { name_ = name; }
        void setGender(const std::string& gender) { gender_ = gender; }
        void setQualifications(const std::vector<std::string>& quals) { qualifications_ = quals; }
        void setTeamName(const std::string& team) { teamName_ = team; }
        void setRelatedStaffIds(const std::vector<std::string>& ids) { relatedStaffIds_ = ids; }
        void setShiftName(const std::string& name) { shiftName_ = name; }
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
        // Member variables
        std::string staffId_;               // 必填
        std::string name_;                  // 必填
        std::string gender_;                // 必填 ("男" / "女")
        std::unordered_set<std::string> qualifications_;   // 必填语义，但可用空 vector 表示“无资质”
        std::string teamName_;              // 可空
        std::vector<std::string> relatedStaffIds_;  // 可空，存储关联的员工ID列表
        std::string shiftName_;
        DateTime startTime_;                          // yyyy/mm/dd hh:mm
        DateTime endTime_;                            // yyyy/mm/dd hh:mm
        std::string boundTerminal_;                      // 绑定的航站楼
        std::vector<std::string> taskTypePreferences_;   // 可空，任务类型偏好列表
        bool allowOvertime_;                             // 是否允许加班
        bool avoidAssignIfPossible_;                     // Y -> true（尽量不派工）
        DateTime latestEndTime_;               // 已分配任务中最晚结束时间
    };

} // namespace AirportStaffScheduler