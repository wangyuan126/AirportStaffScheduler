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
     * @return 任务ID
     */
    long getTaskId() const { return task_id_; }
    
    /**
     * @brief 设置任务ID
     * @param id 任务ID
     */
    void setTaskId(long id) { task_id_ = id; }
    
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
    void setAllowOverlap(bool allow) { allow_overlap_ = allow; }
    
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
    long task_id_;                    ///< 任务ID
    std::string task_name_;              ///< 任务名称
    TaskType task_type_;                 ///< 任务类型
    bool prefer_main_shift_;             ///< 是否优先主班
    long start_time_;                 ///< 任务开始时间（从2020年1月1日0点0分0秒开始的秒数）
    long end_time_;                   ///< 任务结束时间（从2020年1月1日0点0分0秒开始的秒数，-1表示航后）
    int required_qualification_;     ///< 需要的资质类型（位掩码）
    bool can_new_employee_;              ///< 是否可以由新员工担任
    bool allow_overlap_;                 ///< 是否允许任务重叠
    long max_overlap_time_;           ///< 最大任务重叠时间（秒）
    int required_count_;             ///< 需要的人员数量
    bool is_assigned_;                   ///< 是否已经分配
    bool is_short_staffed_;              ///< 是否缺少人手
    std::vector<std::string> assigned_employee_ids_;  ///< 分配的人员ID列表
};

}  // namespace vip_first_class

#endif  // VIP_FIRST_CLASS_TASK_DEFINITION_H

