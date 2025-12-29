/**
 * @file task_definition.h
 * @brief 任务定义类
 * 
 * 定义排班系统中的任务基本信息
 */

#ifndef VIP_FIRST_CLASS_TASK_DEFINITION_H
#define VIP_FIRST_CLASS_TASK_DEFINITION_H

#include <string>
#include <vector>
#include "task_type.h"
#include "task_config.h"
#include "shift.h"

namespace vip_first_class {

/**
 * @brief 任务定义类
 * 
 * 描述一个任务的基本信息和要求
 */
class TaskDefinition {
public:
    /**
     * @brief 构造函数
     */
    TaskDefinition();
    
    /**
     * @brief 析构函数
     */
    ~TaskDefinition();
    
    /**
     * @brief 获取任务ID
     * @return 任务ID（字符串类型）
     */
    const std::string& getTaskId() const { return task_id_; }
    
    /**
     * @brief 设置任务ID
     * @param id 任务ID（字符串类型）
     */
    void setTaskId(const std::string& id) { task_id_ = id; }
    
    /**
     * @brief 获取任务名称
     * @return 任务名称
     */
    const std::string& getTaskName() const { return task_name_; }
    
    /**
     * @brief 设置任务名称
     * @param name 任务名称
     */
    void setTaskName(const std::string& name) { task_name_ = name; }
    
    /**
     * @brief 是否优先主班
     * @return true表示优先主班，false表示不优先主班
     */
    bool isPreferMainShift() const { return prefer_main_shift_; }
    
    /**
     * @brief 设置是否优先主班
     * @param prefer true表示优先主班，false表示不优先主班
     */
    void setPreferMainShift(bool prefer) { prefer_main_shift_ = prefer; }
    
    /**
     * @brief 获取任务开始时间
     * @return 任务开始时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    long getStartTime() const { return start_time_; }
    
    /**
     * @brief 设置任务开始时间
     * @param time 任务开始时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    void setStartTime(long time) { start_time_ = time; }
    
    /**
     * @brief 获取任务结束时间
     * @return 任务结束时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    long getEndTime() const { return end_time_; }
    
    /**
     * @brief 设置任务结束时间
     * @param time 任务结束时间（从2020年1月1日0点0分0秒开始的秒数，如果为航后则设为特殊值）
     */
    void setEndTime(long time) { end_time_ = time; }
    
    /**
     * @brief 检查结束时间是否为航后
     * @return true表示结束时间为航后（需要在运行时确定），false表示固定时间
     */
    bool isAfterFlight() const { return end_time_ < 0; }
    
    /**
     * @brief 设置结束时间为航后
     */
    void setAfterFlight() { end_time_ = -1; }
    
    /**
     * @brief 获取任务实际开始时间（分配后设置）
     * @return 任务实际开始时间（从2020年1月1日0点0分0秒开始的秒数），0表示未分配
     */
    long getActualStartTime() const { return actual_start_time_; }
    
    /**
     * @brief 设置任务实际开始时间
     * @param time 任务实际开始时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    void setActualStartTime(long time) { actual_start_time_ = time; }
    
    /**
     * @brief 获取任务时长
     * @return 任务时长（秒）
     */
    long getDuration() const { return duration_; }
    
    /**
     * @brief 设置任务时长
     * @param duration 任务时长（秒）
     */
    void setDuration(long duration) { 
        duration_ = duration; 
        // 如果允许重叠，最大重叠时间等于任务时长
        if (allow_overlap_) {
            max_overlap_time_ = duration_;
        }
    }
    
    /**
     * @brief 获取任务实际结束时间（实际开始时间 + 时长）
     * @return 任务实际结束时间（从2020年1月1日0点0分0秒开始的秒数），0表示未分配
     */
    long getActualEndTime() const { 
        return actual_start_time_ > 0 ? actual_start_time_ + duration_ : 0; 
    }
    
    /**
     * @brief 获取任务日期
     * @return 任务日期（字符串，格式：YYYY-MM-DD）
     */
    const std::string& getTaskDate() const { return task_date_; }
    
    /**
     * @brief 设置任务日期
     * @param date 任务日期（字符串，格式：YYYY-MM-DD）
     */
    void setTaskDate(const std::string& date) { task_date_ = date; }
    
    /**
     * @brief 获取到达航班ID
     * @return 到达航班ID
     */
    const std::string& getArrivalFlightId() const { return arrival_flight_id_; }
    
    /**
     * @brief 设置到达航班ID
     * @param id 到达航班ID
     */
    void setArrivalFlightId(const std::string& id) { arrival_flight_id_ = id; }
    
    /**
     * @brief 获取出发航班ID
     * @return 出发航班ID
     */
    const std::string& getDepartureFlightId() const { return departure_flight_id_; }
    
    /**
     * @brief 设置出发航班ID
     * @param id 出发航班ID
     */
    void setDepartureFlightId(const std::string& id) { departure_flight_id_ = id; }
    
    /**
     * @brief 获取到达航班号
     * @return 到达航班号
     */
    const std::string& getArrivalFlightNumber() const { return arrival_flight_number_; }
    
    /**
     * @brief 设置到达航班号
     * @param number 到达航班号
     */
    void setArrivalFlightNumber(const std::string& number) { arrival_flight_number_ = number; }
    
    /**
     * @brief 获取出发航班号
     * @return 出发航班号
     */
    const std::string& getDepartureFlightNumber() const { return departure_flight_number_; }
    
    /**
     * @brief 设置出发航班号
     * @param number 出发航班号
     */
    void setDepartureFlightNumber(const std::string& number) { departure_flight_number_ = number; }
    
    /**
     * @brief 获取航站楼
     * @return 航站楼
     */
    const std::string& getTerminal() const { return terminal_; }
    
    /**
     * @brief 设置航站楼
     * @param terminal 航站楼
     */
    void setTerminal(const std::string& terminal) { terminal_ = terminal; }
    
    /**
     * @brief 获取机位
     * @return 机位编号，0表示未分配机位
     */
    int getStand() const { return stand_; }
    
    /**
     * @brief 设置机位
     * @param stand 机位编号，0表示未分配机位
     */
    void setStand(int stand) { stand_ = stand; }
    
    /**
     * @brief 获取需要的资质类型
     * @return 资质类型（位掩码，参见QualificationType）
     */
    int getRequiredQualification() const { return required_qualification_; }
    
    /**
     * @brief 设置需要的资质类型
     * @param qualification 资质类型（位掩码）
     */
    void setRequiredQualification(int qualification) { required_qualification_ = qualification; }
    
    /**
     * @brief 获取任务类型
     * @return 任务类型
     */
    TaskType getTaskType() const { return task_type_; }
    
    /**
     * @brief 设置任务类型
     * @param type 任务类型
     */
    void setTaskType(TaskType type) { task_type_ = type; }
    
    /**
     * @brief 是否可以由新员工担任
     * @return true表示可以由新员工担任，false表示不可以
     */
    bool canNewEmployee() const { return can_new_employee_; }
    
    /**
     * @brief 设置是否可以由新员工担任
     * @param can true表示可以由新员工担任，false表示不可以
     */
    void setCanNewEmployee(bool can) { can_new_employee_ = can; }
    
    /**
     * @brief 是否允许任务重叠
     * @return true表示允许重叠，false表示不允许重叠
     */
    bool allowOverlap() const { return allow_overlap_; }
    
    /**
     * @brief 设置是否允许任务重叠
     * @param allow true表示允许重叠，false表示不允许重叠
     */
    void setAllowOverlap(bool allow) { 
        allow_overlap_ = allow; 
        // 如果允许重叠，最大重叠时间等于任务时长
        if (allow_overlap_ && duration_ > 0) {
            max_overlap_time_ = duration_;
        }
    }
    
    /**
     * @brief 获取最大任务重叠时间
     * @return 最大重叠时间（秒）
     */
    long getMaxOverlapTime() const { return max_overlap_time_; }
    
    /**
     * @brief 设置最大任务重叠时间
     * @param time 最大重叠时间（秒）
     */
    void setMaxOverlapTime(long time) { max_overlap_time_ = time; }
    
    /**
     * @brief 是否已经分配
     * @return true表示已经分配，false表示未分配
     */
    bool isAssigned() const { return is_assigned_; }
    
    /**
     * @brief 设置是否已经分配
     * @param assigned true表示已经分配，false表示未分配
     */
    void setAssigned(bool assigned) { is_assigned_ = assigned; }
    
    /**
     * @brief 是否缺少人手
     * @return true表示缺少人手，false表示不缺少
     */
    bool isShortStaffed() const { return is_short_staffed_; }
    
    /**
     * @brief 设置是否缺少人手
     * @param short_staffed true表示缺少人手，false表示不缺少
     */
    void setShortStaffed(bool short_staffed) { is_short_staffed_ = short_staffed; }
    
    /**
     * @brief 获取分配的人员ID列表
     * @return 人员ID列表的常量引用
     */
    const std::vector<std::string>& getAssignedEmployeeIds() const { return assigned_employee_ids_; }
    
    /**
     * @brief 获取分配的人员ID列表（非常量版本）
     * @return 人员ID列表的引用
     */
    std::vector<std::string>& getAssignedEmployeeIds() { return assigned_employee_ids_; }
    
    /**
     * @brief 添加分配的人员ID
     * @param employee_id 人员ID
     */
    void addAssignedEmployeeId(const std::string& employee_id);
    
    /**
     * @brief 移除分配的人员ID
     * @param employee_id 人员ID
     * @param shifts 班次列表，用于检查是否是固定人选
     * @return 如果该人员是固定人选则返回false，否则执行移除操作，移除成功返回true，未分配返回false
     */
    bool removeAssignedEmployeeId(const std::string& employee_id, 
                                   const std::vector<Shift>& shifts);
    
    /**
     * @brief 检查员工是否是任务的固定人选
     * @param employee_id 人员ID
     * @param shifts 班次列表
     * @return true表示是固定人选，false表示不是
     */
    bool isFixedPerson(const std::string& employee_id, 
                       const std::vector<Shift>& shifts) const;
    
    /**
     * @brief 检查是否已分配给指定人员
     * @param employee_id 人员ID
     * @return true表示已分配，false表示未分配
     */
    bool isAssignedToEmployee(const std::string& employee_id) const;
    
    /**
     * @brief 获取分配的人员数量
     * @return 分配的人员数量
     */
    size_t getAssignedEmployeeCount() const { return assigned_employee_ids_.size(); }
    
    /**
     * @brief 获取需要的人员数量
     * @return 需要的人员数量
     */
    int getRequiredCount() const { return required_count_; }
    
    /**
     * @brief 设置需要的人员数量
     * @param count 需要的人员数量
     */
    void setRequiredCount(int count) { required_count_ = count; }
    
    /**
     * @brief 清除所有人员分配
     */
    void clearAssignedEmployees() {
        assigned_employee_ids_.clear();
        is_assigned_ = false;
    }

private:
    std::string task_id_;                    ///< 任务ID（字符串类型）
    std::string task_name_;              ///< 任务名称
    TaskType task_type_;                 ///< 任务类型
    bool prefer_main_shift_;             ///< 是否优先主班
    long start_time_;                 ///< 任务开始时间（从2020年1月1日0点0分0秒开始的秒数，保留用于兼容）
    long end_time_;                   ///< 任务结束时间（从2020年1月1日0点0分0秒开始的秒数，-1表示航后，保留用于兼容）
    long actual_start_time_;            ///< 任务实际开始时间（分配后设置，从2020年1月1日0点0分0秒开始的秒数），0表示未分配
    long duration_;                     ///< 任务时长（秒）
    int required_qualification_;     ///< 需要的资质类型（位掩码）
    bool can_new_employee_;              ///< 是否可以由新员工担任
    bool allow_overlap_;                 ///< 是否允许任务重叠（默认Y，即true）
    long max_overlap_time_;           ///< 最大任务重叠时间（秒），如果允许重叠则等于任务时长
    int required_count_;             ///< 需要的人员数量
    bool is_assigned_;                   ///< 是否已经分配
    bool is_short_staffed_;              ///< 是否缺少人手
    std::vector<std::string> assigned_employee_ids_;  ///< 分配的人员ID列表
    
    // 航班信息
    std::string task_date_;                  ///< 任务日期（格式：YYYY-MM-DD）
    std::string arrival_flight_id_;         ///< 到达航班ID
    std::string departure_flight_id_;       ///< 出发航班ID
    std::string arrival_flight_number_;     ///< 到达航班号
    std::string departure_flight_number_;   ///< 出发航班号
    std::string terminal_;                   ///< 航站楼
    int stand_;                              ///< 机位编号，0表示未分配机位
};

}  // namespace vip_first_class

#endif  // VIP_FIRST_CLASS_TASK_DEFINITION_H

