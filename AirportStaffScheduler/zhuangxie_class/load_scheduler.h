/**
 * @file load_scheduler.h
 * @brief 装卸任务调度类
 * 
 * 定义装卸排班系统中的任务调度功能
 */

#ifndef ZHUANGXIE_CLASS_LOAD_SCHEDULER_H
#define ZHUANGXIE_CLASS_LOAD_SCHEDULER_H

#include "load_employee_info.h"
#include "load_task.h"
#include "stand_distance.h"
#include "../vip_first_class_algo/shift.h"
#include <vector>
#include <string>
#include <map>
#include <set>

// 前向声明公共类
namespace AirportStaffScheduler {
    class Task;
    class Shift;
    class Staff;
}

namespace zhuangxie_class {

using namespace std;

/**
 * @brief 装卸任务调度类
 * 
 * 负责将航班任务分配给装卸员工
 */
class LoadScheduler {
public:
    /**
     * @brief 构造函数
     */
    LoadScheduler();
    
    /**
     * @brief 析构函数
     */
    ~LoadScheduler();
    
    /**
     * @brief 班次占位时间段（已废弃，不再使用）
     */
    struct ShiftBlockPeriod {
        int shift_type;    ///< 班次类型（已废弃）
        long start_time;    ///< 占位开始时间
        long end_time;      ///< 占位结束时间
    };
    
    /**
     * @brief 调度任务
     * @param employees 员工列表（从shifts中提取）
     * @param tasks 输入输出参数，任务列表（从CSV加载，分配后更新）
     * @param shifts 班次列表（已废弃，不再使用）
     * @param block_periods 班次占位时间段列表（已废弃，不再使用）
     * @param previous_tasks 上一次预排方案（用于减少调整），可以为空
     * @param group_name_to_employees 班组名到员工ID列表的映射（从shift.csv中提取）
     */
    void scheduleLoadTasks(const vector<LoadEmployeeInfo>& employees,
                          vector<LoadTask>& tasks,
                          const vector<vip_first_class::Shift>& shifts,
                          const vector<ShiftBlockPeriod>& block_periods = vector<ShiftBlockPeriod>(),
                          const vector<LoadTask>* previous_tasks = nullptr,
                          const map<string, vector<string>>* group_name_to_employees = nullptr);
    
private:
    /**
     * @brief 按任务保障优先级排序任务
     * @param tasks 任务列表（会被修改）
     */
    void sortTasksByPriority(vector<LoadTask>& tasks);
    
    /**
     * @brief 分配任务给员工
     * @param tasks 任务列表
     * @param employees 员工列表（从shifts中提取）
     * @param shifts 班次列表（已废弃，不再使用）
     * @param block_periods 班次占位时间段列表（已废弃，不再使用）
     * @param previous_tasks 上一次预排方案（用于减少调整）
     * @param group_name_to_employees 班组名到员工ID列表的映射（从shift.csv中提取）
     */
    void assignTasksToEmployees(vector<LoadTask>& tasks,
                                const vector<LoadEmployeeInfo>& employees,
                                const vector<vip_first_class::Shift>& shifts,
                                const vector<ShiftBlockPeriod>& block_periods,
                                const vector<LoadTask>* previous_tasks,
                                const map<string, vector<string>>& group_name_to_employees);
    
    
};

}  // namespace zhuangxie_class

#endif  // ZHUANGXIE_CLASS_LOAD_SCHEDULER_H

