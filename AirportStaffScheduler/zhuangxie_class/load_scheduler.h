/**
 * @file load_scheduler.h
 * @brief 装卸任务调度类
 * 
 * 定义装卸排班系统中的任务调度功能
 */

#ifndef ZHUANGXIE_CLASS_LOAD_SCHEDULER_H
#define ZHUANGXIE_CLASS_LOAD_SCHEDULER_H

#include "load_employee_info.h"
#include "flight.h"
#include "stand_distance.h"
#include "../vip_first_class_algo/shift.h"
#include "../vip_first_class_algo/task_definition.h"
#include "../CommonAdapterUtils.h"
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
     * @brief 班次占位时间段
     */
    struct ShiftBlockPeriod {
        int shift_type;    ///< 班次类型（1=主班，2=副班）
        long start_time;    ///< 占位开始时间
        long end_time;      ///< 占位结束时间
    };
    
    /**
     * @brief 调度任务
     * @param employees 员工列表
     * @param flights 航班信息列表
     * @param shifts 班次列表，表示今日上班的情况（主班、副班、休息）
     * @param tasks 输出参数，分配后的任务列表（形式和scheduleTasks一致）
     * @param default_travel_time 默认通勤时间（秒），如果任务未指定通勤时间则使用此值
     * @param block_periods 班次占位时间段列表（疲劳度控制），该时间段内不向该班次派工
     * @param previous_tasks 上一次预排方案（用于减少调整），可以为空
     */
    void scheduleLoadTasks(const vector<LoadEmployeeInfo>& employees,
                          const vector<Flight>& flights,
                          const vector<vip_first_class::Shift>& shifts,
                          vector<vip_first_class::TaskDefinition>& tasks,
                          long default_travel_time = 480,  // 默认8分钟=480秒
                          const vector<ShiftBlockPeriod>& block_periods = vector<ShiftBlockPeriod>(),
                          const vector<vip_first_class::TaskDefinition>* previous_tasks = nullptr);
    
    /**
     * @brief 调度装卸任务（使用公共类接口）
     * @param staffs 公共Staff列表（用于员工信息）
     * @param tasks 公共Task列表（用于生成装卸任务）
     * @param common_shifts 公共Shift列表
     * @param default_travel_time 默认通勤时间（秒）
     * @param block_periods 班次占位时间段列表（疲劳度控制）
     */
    void scheduleLoadTasksFromCommon(const std::vector<AirportStaffScheduler::Staff>& staffs,
                                     const std::vector<AirportStaffScheduler::Task>& tasks,
                                     const std::vector<AirportStaffScheduler::Shift>& common_shifts,
                                     long default_travel_time = 480,
                                     const vector<ShiftBlockPeriod>& block_periods = vector<ShiftBlockPeriod>());

private:
    /**
     * @brief 从航班信息生成任务
     * @param flights 航班信息列表
     * @param tasks 输出参数，生成的任务列表
     * @param default_travel_time 默认通勤时间（秒）
     * @param flight_task_map 输出参数，任务ID到航班索引的映射
     */
    void generateTasksFromFlights(const vector<Flight>& flights,
                                  vector<vip_first_class::TaskDefinition>& tasks,
                                  long default_travel_time,
                                  map<long, size_t>& flight_task_map);
    
    /**
     * @brief 按任务保障优先级排序任务
     * @param tasks 任务列表（会被修改）
     * @param flights 航班信息列表（用于获取报时、进港时间等信息）
     * @param flight_task_map 任务ID到航班索引的映射
     */
    void sortTasksByPriority(vector<vip_first_class::TaskDefinition>& tasks,
                             const vector<Flight>& flights,
                             const map<long, size_t>& flight_task_map);
    
    /**
     * @brief 分配任务给员工
     * @param tasks 任务列表
     * @param employees 员工列表
     * @param shifts 班次列表
     * @param flights 航班列表（用于获取机位信息）
     * @param block_periods 班次占位时间段列表
     * @param previous_tasks 上一次预排方案（用于减少调整）
     * @param flight_task_map 任务ID到航班索引的映射（用于获取机位信息）
     */
    void assignTasksToEmployees(vector<vip_first_class::TaskDefinition>& tasks,
                                const vector<LoadEmployeeInfo>& employees,
                                const vector<vip_first_class::Shift>& shifts,
                                const vector<Flight>& flights,
                                const vector<ShiftBlockPeriod>& block_periods,
                                const vector<vip_first_class::TaskDefinition>* previous_tasks,
                                const map<long, size_t>& flight_task_map);
    
    /**
     * @brief 检查班次在指定时间是否被占位
     * @param shift_type 班次类型（1=主班，2=副班）
     * @param time 时间点
     * @param block_periods 班次占位时间段列表
     * @return true表示被占位，false表示未被占位
     */
    bool isShiftBlocked(int shift_type, long time,
                       const vector<ShiftBlockPeriod>& block_periods) const;
    
};

}  // namespace zhuangxie_class

#endif  // ZHUANGXIE_CLASS_LOAD_SCHEDULER_H

