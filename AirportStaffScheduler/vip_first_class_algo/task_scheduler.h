/**
 * @file task_scheduler.h
 * @brief 任务调度类
 * 
 * 定义排班系统中的任务调度功能
 */

#ifndef VIP_FIRST_CLASS_TASK_SCHEDULER_H
#define VIP_FIRST_CLASS_TASK_SCHEDULER_H

#include "task_definition.h"
#include "shift.h"
#include <vector>
#include <map>
#include <string>

namespace vip_first_class {

using namespace std;

/**
 * @brief 任务调度类
 * 
 * 负责将任务列表分配给班次中的人员
 */
class TaskScheduler {
public:
    /**
     * @brief 构造函数
     */
    TaskScheduler();
    
    /**
     * @brief 析构函数
     */
    ~TaskScheduler();
    
    /**
     * @brief 调度任务
     * @param tasks 任务列表
     * @param shifts 班次列表，表示今日上班的情况
     */
    void scheduleTasks(vector<TaskDefinition>& tasks, 
                       const vector<Shift>& shifts);
    
    /**
     * @brief 重置第一次值守次数统计（用于新的一天）
     */
    static void resetFirstShiftCounts();
    
    /**
     * @brief 获取员工作为第一次值守的次数
     * @param employee_id 员工ID
     * @return 作为第一次值守的次数
     */
    static int32_t getFirstShiftCount(const string& employee_id);
    
    /**
     * @brief 增加员工作为第一次值守的次数
     * @param employee_id 员工ID
     */
    static void incrementFirstShiftCount(const string& employee_id);

private:
    /**
     * @brief 分配厅内保障任务（4人，2人一组轮流值守）
     * @param tasks 任务列表
     * @param shifts 班次列表
     * @param task_ptr_map 任务ID到TaskDefinition指针的映射
     */
    void scheduleHallMaintenanceTasks(vector<TaskDefinition>& tasks,
                                      const vector<Shift>& shifts,
                                      map<int64_t, TaskDefinition*>& task_ptr_map);
    
    /**
     * @brief 为不值守的员工分配操作间任务
     * @param tasks 任务列表
     * @param shifts 班次列表
     * @param task_ptr_map 任务ID到TaskDefinition指针的映射
     * @param on_duty_employees 正在值守的员工ID列表
     * @param time_slot_start 时间段开始时间
     * @param time_slot_end 时间段结束时间
     */
    void scheduleOperationRoomTasks(vector<TaskDefinition>& tasks,
                                    const vector<Shift>& shifts,
                                    map<int64_t, TaskDefinition*>& task_ptr_map,
                                    const vector<string>& off_duty_employees,
                                    int64_t time_slot_start,
                                    int64_t time_slot_end);
    
    // 静态成员：跟踪每个员工作为第一次值守的次数（全局字段）
    static map<string, int32_t> first_shift_counts_;
};

}  // namespace vip_first_class

#endif  // VIP_FIRST_CLASS_TASK_SCHEDULER_H

