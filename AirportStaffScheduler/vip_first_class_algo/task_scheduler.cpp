/**
 * @file task_scheduler.cpp
 * @brief 任务调度类实现
 */

#include "task_scheduler.h"
#include "task_config.h"
#include "employee_manager.h"
#include "employee_info.h"
#include "../CommonAdapterUtils.h"
#include <algorithm>
#include <map>
#include <vector>
#include <set>
#include <climits>
#include <iostream>
#include <sstream>

namespace vip_first_class {

using namespace std;

// 静态成员变量定义
map<string, int> TaskScheduler::first_shift_counts_;

TaskScheduler::TaskScheduler()
{
}

TaskScheduler::~TaskScheduler()
{
}

// 辅助函数：检查两个时间段是否重叠
static bool isTimeOverlap(long start1, long end1, long start2, long end2, bool allow_overlap, long max_overlap_time)
{
    // 如果任务结束时间为-1（航后），默认使用22:30（81000秒）
    const long DEFAULT_AFTER_FLIGHT_TIME = 22 * 3600 + 30 * 60;  // 22:30 = 81000秒
    
    // 处理航后任务（end_time < 0）
    long actual_end1 = (end1 < 0) ? DEFAULT_AFTER_FLIGHT_TIME : end1;
    long actual_end2 = (end2 < 0) ? DEFAULT_AFTER_FLIGHT_TIME : end2;
    
    // 检查时间段重叠
    bool overlap = !(actual_end1 <= start2 || actual_end2 <= start1);
    
    if (!overlap) {
        return false;
    }
    
    // 如果允许重叠，检查重叠时间是否超过最大允许时间
    if (allow_overlap && max_overlap_time > 0) {
        long overlap_start = max(start1, start2);
        long overlap_end = min(actual_end1, actual_end2);
        long overlap_duration = overlap_end - overlap_start;
        return overlap_duration > max_overlap_time;
    }
    
    // 不允许重叠或重叠时间超过限制
    return true;
}

// 辅助函数：检查员工资质是否匹配任务要求
static bool isQualificationMatch(const string& employee_id, int required_qualification)
{
    if (required_qualification == 0) {
        return true;  // 没有资质要求，任何员工都可以
    }
    
    auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
    if (!employee) {
        return false;
    }
    
    int employee_qualification = employee->getQualificationMask();
    // 检查员工是否具有任务要求的所有资质（位掩码检查）
    return (employee_qualification & required_qualification) == required_qualification;
}

// 辅助函数：检查员工在指定时间段是否空闲
static bool isEmployeeAvailable(const string& employee_id, long task_start, long task_end, 
                                 bool task_allow_overlap, long task_max_overlap_time,
                                 const map<string, TaskDefinition*>& task_ptr_map)
{
    auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
    if (!employee) {
        return false;
    }
    
    // 遍历员工的所有任务
    const auto& assigned_task_ids = employee->getAssignedTaskIds();
    for (const string& assigned_task_id : assigned_task_ids) {
        auto task_it = task_ptr_map.find(assigned_task_id);
        if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
            continue;
        }
        
        const TaskDefinition& assigned_task = *(task_it->second);
        long assigned_start = assigned_task.getStartTime();
        long assigned_end = assigned_task.getEndTime();
        
        // 检查时间段是否重叠（考虑两个任务的允许重叠设置）
        bool allow_overlap = task_allow_overlap && assigned_task.allowOverlap();
        long max_overlap = max(task_max_overlap_time, assigned_task.getMaxOverlapTime());
        
        if (isTimeOverlap(task_start, task_end, assigned_start, assigned_end, allow_overlap, max_overlap)) {
            return false;  // 时间段重叠，员工不空闲
        }
    }
    
    return true;  // 员工空闲
}

// 辅助函数：计算员工当日已分配任务的总时长（秒）
static long calculateEmployeeDailyTaskTime(const string& employee_id, 
                                                long current_task_start_time,
                                                const map<string, TaskDefinition*>& task_ptr_map)
{
    const long SECONDS_PER_DAY = 24 * 3600;  // 一天的秒数
    const long DEFAULT_AFTER_FLIGHT_TIME = 22 * 3600 + 30 * 60;  // 22:30 = 81000秒
    
    // 计算当前任务所属的日期（从2020-01-01开始的第几天）
    long current_day = current_task_start_time / SECONDS_PER_DAY;
    
    auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
    if (!employee) {
        return 0;
    }
    
    long total_task_time = 0;
    
    // 遍历员工的所有已分配任务
    const auto& assigned_task_ids = employee->getAssignedTaskIds();
    for (const string& assigned_task_id : assigned_task_ids) {
        auto task_it = task_ptr_map.find(assigned_task_id);
        if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
            continue;
        }
        
        const TaskDefinition& assigned_task = *(task_it->second);
        long task_start = assigned_task.getStartTime();
        
        // 计算任务所属的日期
        long task_day = task_start / SECONDS_PER_DAY;
        
        // 只计算当日的任务
        if (task_day == current_day) {
            long task_end = assigned_task.getEndTime();
            
            // 处理航后任务（end_time < 0）
            long actual_end;
            if (task_end < 0) {
                // 航后任务：结束时间为当天的22:30
                actual_end = task_day * SECONDS_PER_DAY + DEFAULT_AFTER_FLIGHT_TIME;
            } else {
                actual_end = task_end;
            }
            
            // 计算任务时长（秒）
            long task_duration = actual_end - task_start;
            if (task_duration > 0) {
                total_task_time += task_duration;
            }
        }
    }
    
    return total_task_time;
}

// 辅助函数：检查一个时间段是否包含另一个时间段
// 如果outer时间段完全包含inner时间段，返回true
static bool isTimeRangeContains(long outer_start, long outer_end, 
                                 long inner_start, long inner_end)
{
    const long DEFAULT_AFTER_FLIGHT_TIME = 22 * 3600 + 30 * 60;  // 22:30 = 81000秒
    const long SECONDS_PER_DAY = 24 * 3600;
    
    // 处理航后任务
    long actual_outer_end = (outer_end < 0) ? (outer_start / SECONDS_PER_DAY) * SECONDS_PER_DAY + DEFAULT_AFTER_FLIGHT_TIME : outer_end;
    long actual_inner_end = (inner_end < 0) ? (inner_start / SECONDS_PER_DAY) * SECONDS_PER_DAY + DEFAULT_AFTER_FLIGHT_TIME : inner_end;
    
    // outer时间段包含inner时间段：outer_start <= inner_start 且 outer_end >= inner_end
    return outer_start <= inner_start && actual_outer_end >= actual_inner_end;
}

// 辅助函数：检查任务是否是固定任务（通过员工ID和班次信息）
static bool isTaskFixedForEmployee(const string& task_id, TaskType task_type, const string& employee_id,
                                    const vector<Shift>& shifts)
{
    // 获取任务的固定人选配置
    const auto& fixed_persons = TaskConfig::getInstance().getFixedPersonsByType(task_type);
    if (fixed_persons.empty()) {
        return false;
    }
    
    // 在班次列表中找到该员工所在的班次和位置
    for (const auto& shift : shifts) {
        // 跳过休息的班次（shift_type == 0 表示休息）
        int shift_type = shift.getShiftType();
        if (shift_type == 0) {
            continue;
        }
        
        const auto& position_map = shift.getPositionToEmployeeId();
        for (const auto& pos_pair : position_map) {
            if (pos_pair.second == employee_id) {
                int position = pos_pair.first;
                
                // 将ShiftType转换为ShiftCategory
                ShiftCategory category;
                if (shift_type == 1) {  // MAIN
                    category = ShiftCategory::MAIN;
                } else if (shift_type == 2) {  // SUB
                    category = ShiftCategory::SUB;
                } else {
                    continue;
                }
                
                // 检查该位置是否匹配固定人选配置
                for (const auto& fixed_info : fixed_persons) {
                    if (fixed_info.shift_category == category && fixed_info.position == position) {
                        return true;  // 是固定人选
                    }
                }
            }
        }
    }
    
    return false;
}

void TaskScheduler::resetFirstShiftCounts()
{
    first_shift_counts_.clear();
}

int TaskScheduler::getFirstShiftCount(const string& employee_id)
{
    auto it = first_shift_counts_.find(employee_id);
    return (it != first_shift_counts_.end()) ? it->second : 0;
}

void TaskScheduler::incrementFirstShiftCount(const string& employee_id)
{
    first_shift_counts_[employee_id]++;
}

void TaskScheduler::scheduleTasks(vector<TaskDefinition>& tasks, 
                                   const vector<Shift>& shifts)
{
    // 0. 动态设定厅内保障任务的4个固定人选
    TaskConfig::getInstance().setHallMaintenanceFixedPersons(shifts, tasks);
    
    // 0.1 预先预留足够容量，避免在添加操作间任务时重新分配内存导致指针失效
    // 估计需要添加的操作间任务数量（最多14个，对应14个厅内保障任务时间段）
    size_t estimated_operation_tasks = 14;
    tasks.reserve(tasks.size() + estimated_operation_tasks);
    cerr << "[DEBUG] 任务向量容量已预留，当前大小=" << tasks.size() 
         << ", 容量=" << tasks.capacity() << endl;
    
    // 1. 根据任务优先级对任务进行排序（优先级高的在前）
    sort(tasks.begin(), tasks.end(), [](const TaskDefinition& a, const TaskDefinition& b) {
        int priority_a = TaskConfig::getInstance().getTaskPriority(a.getTaskType());
        int priority_b = TaskConfig::getInstance().getTaskPriority(b.getTaskType());
        
        // 优先级高的排在前面（数值越大优先级越高）
        if (priority_a != priority_b) {
            return priority_a > priority_b;
        }
        
        // 如果优先级相同，按照任务ID排序（保持稳定排序）
        return a.getTaskId() < b.getTaskId();
    });
    
    // 2. 创建任务ID到TaskDefinition指针的映射，方便查找和更新
    map<string, TaskDefinition*> task_ptr_map;
    for (auto& task : tasks) {
        task_ptr_map[task.getTaskId()] = &task;
    }
    cerr << "[DEBUG] 任务指针映射建立完成，共 " << task_ptr_map.size() << " 个任务" << endl;
    
    // 2.1 先处理厅内保障任务（4人，2人一组轮流值守）
    scheduleHallMaintenanceTasks(tasks, shifts, task_ptr_map);
    
    // 2.2 在添加操作间任务后，重新建立任务指针映射，确保所有指针都是最新的
    // 因为scheduleHallMaintenanceTasks可能会添加新的操作间任务，导致tasks向量重新分配
    task_ptr_map.clear();
    for (auto& task : tasks) {
        task_ptr_map[task.getTaskId()] = &task;
    }
    cerr << "[DEBUG] 厅内任务处理后，任务指针映射已更新，共 " << task_ptr_map.size() << " 个任务" << endl;
    
    // 3. 使用任务ID集合来跟踪已处理的任务
    set<string> processed_task_ids;
    
    // 4. 遍历任务列表，逐个分配任务
    size_t current_index = 0;
    while (current_index < tasks.size()) {
        TaskDefinition& task = tasks[current_index];
        string task_id = task.getTaskId();
        
        // 跳过已经处理过的任务
        if (processed_task_ids.find(task.getTaskId()) != processed_task_ids.end()) {
            current_index++;
            continue;
        }
        
        // 跳过已经分配的任务
        if (task.isAssigned() && task.getAssignedEmployeeCount() > 0) {
            processed_task_ids.insert(task.getTaskId());
            current_index++;
            continue;
        }
        
        // 跳过厅内保障任务（已经单独处理）
        bool is_hall_maintenance_task = false;
        static const TaskType hall_task_types[] = {
            TaskType::DOMESTIC_HALL_EARLY,      // 国内厅内早班（05:30-08:30）
            TaskType::DOMESTIC_HALL_0830_0930, TaskType::DOMESTIC_HALL_0930_1030,
            TaskType::DOMESTIC_HALL_1030_1130, TaskType::DOMESTIC_HALL_1130_1230,
            TaskType::DOMESTIC_HALL_1230_1330, TaskType::DOMESTIC_HALL_1330_1430,
            TaskType::DOMESTIC_HALL_1430_1530, TaskType::DOMESTIC_HALL_1530_1630,
            TaskType::DOMESTIC_HALL_1630_1730, TaskType::DOMESTIC_HALL_1730_1830,
            TaskType::DOMESTIC_HALL_1830_1930, TaskType::DOMESTIC_HALL_1930_2030,
            TaskType::DOMESTIC_HALL_2030_AFTER
        };
        for (const auto& hall_task_type : hall_task_types) {
            if (task.getTaskType() == hall_task_type) {
                is_hall_maintenance_task = true;
                break;
            }
        }
        if (is_hall_maintenance_task) {
            processed_task_ids.insert(task.getTaskId());
            current_index++;
            continue;
        }
        
        // 获取任务的固定人选配置
        const auto& fixed_persons = TaskConfig::getInstance().getFixedPersonsByType(task.getTaskType());
        
        // 从任务属性获取已分配人数和需要人数
        int assigned_count = static_cast<int>(task.getAssignedEmployeeCount());  // 已分配的人数（从任务->人员的映射获取）
        int required_count = task.getRequiredCount();  // 需要的人数（从任务属性获取）
        
        // 3.1 分配固定人选：收集所有固定人选对应的员工，然后统一处理
        vector<string> fixed_employee_candidates;
        set<string> fixed_employee_set;  // 用于去重
        
        for (const auto& fixed_info : fixed_persons) {
            // 在班次列表中找到固定人选
            string fixed_employee_id;
            for (const auto& shift : shifts) {
                // 跳过休息的班次（shift_type == 0 表示休息）
                int shift_type = shift.getShiftType();
                if (shift_type == 0) {
                    continue;
                }
                
                ShiftCategory category = (shift_type == 1) ? ShiftCategory::MAIN : ShiftCategory::SUB;
                
                if (fixed_info.shift_category == category) {
                    fixed_employee_id = shift.getEmployeeIdAtPosition(fixed_info.position);
                    if (!fixed_employee_id.empty()) {
                        break;
                    }
                }
            }
            
            // 去重：避免同一个员工被添加多次
            if (!fixed_employee_id.empty() && fixed_employee_set.find(fixed_employee_id) == fixed_employee_set.end()) {
                fixed_employee_candidates.push_back(fixed_employee_id);
                fixed_employee_set.insert(fixed_employee_id);
            }
        }
        
        // 对所有固定人选候选进行空闲检查和资质检查并分配
        for (const auto& fixed_employee_id : fixed_employee_candidates) {
            // 检查资质是否匹配（硬约束）
            if (!isQualificationMatch(fixed_employee_id, task.getRequiredQualification())) {
                continue;  // 资质不匹配，跳过
            }
            
            // 检查固定人选是否在时间段空闲
            if (isEmployeeAvailable(fixed_employee_id, task.getStartTime(), task.getEndTime(),
                                     task.allowOverlap(), task.getMaxOverlapTime(), task_ptr_map)) {
                // 检查是否已经分配（避免重复分配）
                if (!task.isAssignedToEmployee(fixed_employee_id)) {
                    // 分配任务给固定人选
                    // 维护双向映射：任务到人（任务->人员）
                    task.addAssignedEmployeeId(fixed_employee_id);
                    
                    // 维护双向映射：人到任务（人员->任务）
                    auto* employee = EmployeeManager::getInstance().getEmployee(fixed_employee_id);
                    if (employee) {
                        employee->addAssignedTaskId(task.getTaskId());
                    }
                }
            }
        }
        
        // 更新已分配人数（从任务对象重新获取，确保数据一致）
        assigned_count = static_cast<int>(task.getAssignedEmployeeCount());
        
        // 3.2 如果还需要其他人，继续分配
        while (assigned_count < required_count) {
            string selected_employee_id;
            
            // 3.2.1 优先选择空闲的、当日任务时间最少的人
            // 软约束：副班人员上下班弹性 - 任务繁忙时，优先安排工时少的副班人员提前上岗或延迟下岗
            long min_daily_task_time = LONG_MAX;
            int selected_shift_type = 0;  // 记录选中的班次类型，用于副班优先
            
            // 首先尝试从主班选择（如果任务优先主班）
            if (task.isPreferMainShift()) {
                for (const auto& shift : shifts) {
                    // 只处理主班（shift_type == 1）
                    if (shift.getShiftType() != 1) {
                        continue;
                    }
                    
                    const auto& position_map = shift.getPositionToEmployeeId();
                    for (const auto& pos_pair : position_map) {
                        const string& employee_id = pos_pair.second;
                        
                        // 检查是否已经分配
                        if (task.isAssignedToEmployee(employee_id)) {
                            continue;
                        }
                        
                        // 检查资质是否匹配（硬约束）
                        if (!isQualificationMatch(employee_id, task.getRequiredQualification())) {
                            continue;  // 资质不匹配，跳过
                        }
                        
                        // 检查是否空闲
                        if (isEmployeeAvailable(employee_id, task.getStartTime(), task.getEndTime(),
                                                 task.allowOverlap(), task.getMaxOverlapTime(), task_ptr_map)) {
                            // 计算该员工当日已分配任务的总时长
                            long daily_task_time = calculateEmployeeDailyTaskTime(employee_id, 
                                                                                      task.getStartTime(), 
                                                                                      task_ptr_map);
                            if (daily_task_time < min_daily_task_time) {
                                min_daily_task_time = daily_task_time;
                                selected_employee_id = employee_id;
                                selected_shift_type = 1;  // 主班
                            }
                        }
                    }
                }
            }
            
            // 如果主班没有找到合适的人，或者任务不优先主班，从所有班次中选择
            if (selected_employee_id.empty() || !task.isPreferMainShift()) {
                for (const auto& shift : shifts) {
                    // 跳过休息的班次（shift_type == 0 表示休息）
                    if (shift.getShiftType() == 0) {
                        continue;
                    }
                    
                    const auto& position_map = shift.getPositionToEmployeeId();
                    for (const auto& pos_pair : position_map) {
                        const string& employee_id = pos_pair.second;
                        
                        // 检查是否已经分配
                        if (task.isAssignedToEmployee(employee_id)) {
                            continue;
                        }
                        
                        // 检查资质是否匹配（硬约束）
                        if (!isQualificationMatch(employee_id, task.getRequiredQualification())) {
                            continue;  // 资质不匹配，跳过
                        }
                        
                        // 检查新员工限制（如果任务不允许新员工，需要检查员工是否为新员工）
                        // 注意：这里简化处理，假设canNewEmployee=false时，所有员工都可以（实际应该检查员工是否为新员工）
                        // 如果需要更严格的检查，需要在EmployeeInfo中添加isNewEmployee字段
                        
                        // 检查是否空闲
                        if (isEmployeeAvailable(employee_id, task.getStartTime(), task.getEndTime(),
                                                 task.allowOverlap(), task.getMaxOverlapTime(), task_ptr_map)) {
                            // 计算该员工当日已分配任务的总时长
                            long daily_task_time = calculateEmployeeDailyTaskTime(employee_id, 
                                                                                      task.getStartTime(), 
                                                                                      task_ptr_map);
                            
                            // 软约束：副班人员上下班弹性 - 如果任务繁忙（当前已分配人数不足），优先选择工时少的副班
                            int shift_type = shift.getShiftType();
                            bool is_sub_shift = (shift_type == 2);  // 副班
                            
                            // 如果任务繁忙且是副班，且工时更少，优先选择
                            if (is_sub_shift && assigned_count < required_count && daily_task_time < min_daily_task_time) {
                                min_daily_task_time = daily_task_time;
                                selected_employee_id = employee_id;
                                selected_shift_type = 2;  // 副班
                            } else if (daily_task_time < min_daily_task_time) {
                                min_daily_task_time = daily_task_time;
                                selected_employee_id = employee_id;
                                selected_shift_type = shift_type;
                            }
                        }
                    }
                }
            }
            
            // 3.2.2 如果找到空闲的人，分配任务
            if (!selected_employee_id.empty()) {
                // 维护双向映射：任务到人（任务->人员）
                task.addAssignedEmployeeId(selected_employee_id);
                
                // 维护双向映射：人到任务（人员->任务）
                auto* employee = EmployeeManager::getInstance().getEmployee(selected_employee_id);
                if (employee) {
                    employee->addAssignedTaskId(task.getTaskId());
                }
                
                assigned_count++;
                continue;
            }
            
            // 3.2.3 如果没有空闲的人，找有非固定任务且优先级低的人
            // 先收集所有可以撤销的任务，然后按优先级排序，选择优先级最低的
            struct ReplaceableTask {
                string employee_id;
                string task_id;
                TaskDefinition* task_ptr;
                long priority;
            };
            
            vector<ReplaceableTask> replaceable_tasks;
            long current_priority = TaskConfig::getInstance().getTaskPriority(task.getTaskType());
            
            // 收集所有可以撤销的任务
            for (const auto& shift : shifts) {
                // 跳过休息的班次（shift_type == 0 表示休息）
                if (shift.getShiftType() == 0) {
                    continue;
                }
                
                const auto& position_map = shift.getPositionToEmployeeId();
                for (const auto& pos_pair : position_map) {
                    const string& employee_id = pos_pair.second;
                    
                    // 检查是否已经分配
                    if (task.isAssignedToEmployee(employee_id)) {
                        continue;
                    }
                    
                    auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
                    if (!employee) {
                        continue;
                    }
                    
                    // 遍历该员工的所有任务
                    const auto& assigned_task_ids = employee->getAssignedTaskIds();
                    for (const string& assigned_task_id : assigned_task_ids) {
                        auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                        if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                            continue;
                        }
                        
                        TaskDefinition& assigned_task = *(assigned_task_it->second);
                        
                        // 检查是否时间重叠
                        if (isTimeOverlap(task.getStartTime(), task.getEndTime(),
                                          assigned_task.getStartTime(), assigned_task.getEndTime(),
                                          task.allowOverlap() && assigned_task.allowOverlap(),
                                          max(task.getMaxOverlapTime(), assigned_task.getMaxOverlapTime()))) {
                            // 检查是否是固定任务
                            if (!isTaskFixedForEmployee(assigned_task_id, assigned_task.getTaskType(),
                                                         employee_id, shifts)) {
                                // 检查优先级
                                long assigned_priority = TaskConfig::getInstance().getTaskPriority(assigned_task.getTaskType());
                                if (assigned_priority < current_priority) {
                                    replaceable_tasks.push_back({employee_id, assigned_task_id, &assigned_task, assigned_priority});
                                }
                            }
                        }
                    }
                }
            }
            
            // 按优先级排序，优先级最低的在前（优先考虑优先级最低的任务）
            sort(replaceable_tasks.begin(), replaceable_tasks.end(),
                      [](const ReplaceableTask& a, const ReplaceableTask& b) {
                          return a.priority < b.priority;
                      });
            
            bool found_replacement = false;
            bool use_overlap = false;  // 是否使用重叠模式
            TaskDefinition* overlapping_task = nullptr;  // 重叠模式下的原任务指针
            
            // 遍历所有可以撤销的任务，从优先级最低的开始
            for (const auto& replaceable : replaceable_tasks) {
                TaskDefinition& assigned_task = *(replaceable.task_ptr);
                
                // 检查被撤销任务的时间段是否包含高优先级任务的时间段
                if (isTimeRangeContains(assigned_task.getStartTime(), assigned_task.getEndTime(),
                                        task.getStartTime(), task.getEndTime())) {
                    // 被撤销任务包含高优先级任务，允许重叠而不是撤销
                    selected_employee_id = replaceable.employee_id;
                    overlapping_task = replaceable.task_ptr;
                    use_overlap = true;
                    found_replacement = true;
                    break;
                }
            }
            
            // 如果没有找到可以重叠的任务，则使用原来的撤销逻辑（选择优先级最低的任务）
            if (!found_replacement && !replaceable_tasks.empty()) {
                const auto& replaceable = replaceable_tasks[0];  // 优先级最低的任务
                selected_employee_id = replaceable.employee_id;
                TaskDefinition& assigned_task = *(replaceable.task_ptr);
                
                // 移除原任务分配，维护双向映射
                // 维护双向映射：任务到人（任务->人员），从任务中移除员工
                assigned_task.removeAssignedEmployeeId(replaceable.employee_id, shifts);
                // 维护双向映射：人到任务（人员->任务），从员工中移除任务
                auto* employee = EmployeeManager::getInstance().getEmployee(replaceable.employee_id);
                if (employee) {
                    employee->removeAssignedTaskId(replaceable.task_id);
                }
                
                // 更新被替换任务的状态
                size_t remaining_count = assigned_task.getAssignedEmployeeCount();
                if (remaining_count > 0) {
                    // 如果还有人负责，则标记为已分配但人手不足
                    assigned_task.setAssigned(true);
                    assigned_task.setShortStaffed(true);
                } else {
                    // 如果没人负责了，标记为未分配，以便重新处理
                    assigned_task.setAssigned(false);
                    assigned_task.setShortStaffed(false);
                    processed_task_ids.erase(replaceable.task_id);
                }
                
                found_replacement = true;
            }
            
            // 如果找不到可以撤销的任务，输出错误信息
            if (!found_replacement && replaceable_tasks.empty()) {
                cerr << "错误：无法为任务 ID " << task.getTaskId() 
                          << " (名称: " << task.getTaskName() 
                          << ", 类型: " << static_cast<int>(task.getTaskType())
                          << ", 优先级: " << current_priority
                          << ") 找到可以撤销的任务。任务需要 " << required_count 
                          << " 人，当前已分配 " << assigned_count << " 人。" << endl;
            }
            
            // 如果找到可以替换的员工，分配当前任务
            if (found_replacement && !selected_employee_id.empty()) {
                if (use_overlap) {
                    // 重叠模式：允许两个任务重叠，不撤销原任务
                    // 确保当前任务和原任务都允许重叠
                    task.setAllowOverlap(true);
                    if (overlapping_task) {
                        overlapping_task->setAllowOverlap(true);
                    }
                    
                    // 维护双向映射：任务到人（任务->人员）
                    task.addAssignedEmployeeId(selected_employee_id);
                    
                    // 维护双向映射：人到任务（人员->任务）
                    auto* employee = EmployeeManager::getInstance().getEmployee(selected_employee_id);
                    if (employee) {
                        employee->addAssignedTaskId(task.getTaskId());
                    }
                    
                    assigned_count++;
                    // 重叠模式下不需要重新排序，因为原任务没有被撤销
                } else {
                    // 撤销模式：原任务被撤销，需要重新排序
                    // 维护双向映射：任务到人（任务->人员）
                    task.addAssignedEmployeeId(selected_employee_id);
                    
                    // 维护双向映射：人到任务（人员->任务）
                    auto* employee = EmployeeManager::getInstance().getEmployee(selected_employee_id);
                    if (employee) {
                        employee->addAssignedTaskId(task.getTaskId());
                    }
                    
                    assigned_count++;
                    
                    // 重新排序tasks列表（因为任务状态可能改变）
                    sort(tasks.begin(), tasks.end(), [](const TaskDefinition& a, const TaskDefinition& b) {
                        int priority_a = TaskConfig::getInstance().getTaskPriority(a.getTaskType());
                        int priority_b = TaskConfig::getInstance().getTaskPriority(b.getTaskType());
                        
                        if (priority_a != priority_b) {
                            return priority_a > priority_b;
                        }
                        
                        return a.getTaskId() < b.getTaskId();
                    });
                    
                    // 重新建立任务指针映射
                    task_ptr_map.clear();
                    for (auto& t : tasks) {
                        task_ptr_map[t.getTaskId()] = &t;
                    }
                    cerr << "[DEBUG] 任务重新排序后，指针映射已更新，共 " << task_ptr_map.size() << " 个任务" << endl;
                    
                    // 重新开始循环（从0开始）
                    current_index = 0;
                    continue;
                }
            } else {
                // 无法分配更多人员，标记为缺少人手
                task.setShortStaffed(true);
                break;
            }
        }
        cout<<"任务 ID " << task.getTaskId() 
                  << " (名称: " << task.getTaskName() 
                  << ", 类型: " << static_cast<int>(task.getTaskType())
                  << ") 已分配 " << assigned_count 
                  << " 人，需求 " << required_count << " 人。" << endl;
        // 更新任务状态
        if (assigned_count > 0) {
            task.setAssigned(true);
        }
        
        // 标记为已处理
        processed_task_ids.insert(task.getTaskId());
        current_index++;
    }
    cout<<"任务调度完成！"<<endl;
}

// 使用公共类的适配器函数实现
void TaskScheduler::scheduleTasksFromCommon(
    const std::vector<AirportStaffScheduler::Task>& common_tasks,
    const std::vector<AirportStaffScheduler::Shift>& common_shifts,
    const std::vector<AirportStaffScheduler::Staff>& common_staffs)
{
    using namespace AirportStaffScheduler::Adapter;
    
    // 1. 转换Staff到EmployeeInfo并注册到EmployeeManager
    EmployeeManager& emp_manager = EmployeeManager::getInstance();
    for (const auto& staff : common_staffs) {
        EmployeeInfo emp_info = StaffToEmployeeInfo(staff);
        emp_manager.addOrUpdateEmployee(staff.getStaffId(), emp_info);
    }
    
    // 2. 转换Task到TaskDefinition
    vector<TaskDefinition> tasks;
    tasks.reserve(common_tasks.size());
    for (const auto& common_task : common_tasks) {
        // 这里需要根据task_name推断TaskType，简化处理，使用默认值
        // 实际使用时可能需要更复杂的映射逻辑
        TaskDefinition task_def = TaskToTaskDefinition(common_task, TaskType::DISPATCH);
        tasks.push_back(task_def);
    }
    
    // 3. 转换Shift：需要将公共Shift列表按班次类型分组
    // 公共Shift是单个员工的班次，需要转换成vip_first_class::Shift（包含多个位置）
    map<int, Shift> shift_map;  // shift_type -> Shift对象
    
    for (const auto& common_shift : common_shifts) {
        // 从shift_name推断shift_type（简化处理）
        const std::string& shift_name = common_shift.getShiftName();
        int shift_type = 0;  // 默认休息
        if (shift_name.find("主班") != string::npos) {
            shift_type = 1;  // 主班
        } else if (shift_name.find("副班") != string::npos) {
            shift_type = 2;  // 副班
        }
        
        // 获取或创建对应类型的Shift
        if (shift_map.find(shift_type) == shift_map.end()) {
            Shift vip_shift;
            vip_shift.setShiftType(shift_type);
            shift_map[shift_type] = vip_shift;
        }
        
        // 确定位置（这里简化处理，假设按顺序分配位置）
        // 实际使用时可能需要从shift_id或其他信息中提取位置编号
        int position = shift_map[shift_type].getPositionToEmployeeId().size() + 1;
        shift_map[shift_type].setEmployeeIdAtPosition(position, common_shift.getStaffId());
    }
    
    // 转换为vector
    vector<Shift> shifts;
    for (auto& pair : shift_map) {
        shifts.push_back(pair.second);
    }
    
    // 4. 调用原有的调度函数
    scheduleTasks(tasks, shifts);
}

void TaskScheduler::scheduleHallMaintenanceTasks(vector<TaskDefinition>& tasks,
                                                 const vector<Shift>& shifts,
                                                 map<string, TaskDefinition*>& task_ptr_map)
{
    // 获取厅内保障任务的4个固定人选
    const auto& hall_fixed_persons = TaskConfig::getInstance().getHallMaintenanceFixedPersons();
    if (hall_fixed_persons.size() < 2) {
        // 如果不足2个人，无法进行分组，直接返回
        cerr << "警告：厅内保障任务固定人选不足2人，无法进行分组。当前人数: " << hall_fixed_persons.size() << endl;
        return;
    }
    
    if (hall_fixed_persons.size() < 4) {
        cerr << "警告：厅内保障任务固定人选不足4人，当前人数: " << hall_fixed_persons.size() << "，将使用现有人员进行分配" << endl;
    }
    
    // 将人员分为两组：根据第一次值守次数决定
    vector<string> group1, group2;
    bool group1_starts_first = true;  // 记录group1是否先值守
    
    if (hall_fixed_persons.size() == 4) {
        // 4个人：分成两组，每组2人
        int sum1 = getFirstShiftCount(hall_fixed_persons[0]) + getFirstShiftCount(hall_fixed_persons[1]);
        int sum2 = getFirstShiftCount(hall_fixed_persons[2]) + getFirstShiftCount(hall_fixed_persons[3]);
        
        if (sum1 <= sum2) {
            // 第一组先值守
            group1.push_back(hall_fixed_persons[0]);
            group1.push_back(hall_fixed_persons[1]);
            group2.push_back(hall_fixed_persons[2]);
            group2.push_back(hall_fixed_persons[3]);
            group1_starts_first = true;
        } else {
            // 第二组先值守，但为了保持group1总是先值守的组，交换一下
            group1.push_back(hall_fixed_persons[2]);
            group1.push_back(hall_fixed_persons[3]);
            group2.push_back(hall_fixed_persons[0]);
            group2.push_back(hall_fixed_persons[1]);
            group1_starts_first = true;
        }
    } else if (hall_fixed_persons.size() == 3) {
        // 3个人：第一组2人，第二组1人
        int sum1 = getFirstShiftCount(hall_fixed_persons[0]) + getFirstShiftCount(hall_fixed_persons[1]);
        int sum2 = getFirstShiftCount(hall_fixed_persons[2]);
        
        if (sum1 <= sum2) {
            group1.push_back(hall_fixed_persons[0]);
            group1.push_back(hall_fixed_persons[1]);
            group2.push_back(hall_fixed_persons[2]);
            group1_starts_first = true;
        } else {
            group1.push_back(hall_fixed_persons[2]);
            group2.push_back(hall_fixed_persons[0]);
            group2.push_back(hall_fixed_persons[1]);
            group1_starts_first = true;
        }
    } else if (hall_fixed_persons.size() == 2) {
        // 2个人：每组1人
        int sum1 = getFirstShiftCount(hall_fixed_persons[0]);
        int sum2 = getFirstShiftCount(hall_fixed_persons[1]);
        
        if (sum1 <= sum2) {
            group1.push_back(hall_fixed_persons[0]);
            group2.push_back(hall_fixed_persons[1]);
            group1_starts_first = true;
        } else {
            group1.push_back(hall_fixed_persons[1]);
            group2.push_back(hall_fixed_persons[0]);
            group1_starts_first = true;
        }
    }
    
    // 收集所有厅内保障任务ID（包括国内厅内早班和1小时为粒度的任务）
    // 使用任务ID而不是指针，避免在添加操作间任务时指针失效
    vector<string> hall_task_ids;
    static const TaskType hall_task_types[] = {
        TaskType::DOMESTIC_HALL_EARLY,      // 国内厅内早班（05:30-08:30）
        TaskType::DOMESTIC_HALL_0830_0930, TaskType::DOMESTIC_HALL_0930_1030,
        TaskType::DOMESTIC_HALL_1030_1130, TaskType::DOMESTIC_HALL_1130_1230,
        TaskType::DOMESTIC_HALL_1230_1330, TaskType::DOMESTIC_HALL_1330_1430,
        TaskType::DOMESTIC_HALL_1430_1530, TaskType::DOMESTIC_HALL_1530_1630,
        TaskType::DOMESTIC_HALL_1630_1730, TaskType::DOMESTIC_HALL_1730_1830,
        TaskType::DOMESTIC_HALL_1830_1930, TaskType::DOMESTIC_HALL_1930_2030,
        TaskType::DOMESTIC_HALL_2030_AFTER
    };
    cerr << "[DEBUG] 开始收集厅内保障任务，任务总数: " << tasks.size() << endl;
    for (auto& task : tasks) {
        bool is_hall_task = false;
        for (const auto& hall_task_type : hall_task_types) {
            if (task.getTaskType() == hall_task_type) {
                is_hall_task = true;
                break;
            }
        }
        if (is_hall_task) {
            hall_task_ids.push_back(task.getTaskId());
            cerr << "[DEBUG] 找到厅内保障任务: ID=" << task.getTaskId() 
                 << ", 名称=" << task.getTaskName()
                 << ", 类型=" << static_cast<int>(task.getTaskType())
                 << ", 指针=" << static_cast<void*>(&task)
                 << ", 开始时间=" << task.getStartTime()
                 << ", 已分配人数=" << task.getAssignedEmployeeCount() << endl;
        }
    }
    
    if (hall_task_ids.empty()) {
        cerr << "警告：未找到任何厅内保障任务，任务总数: " << tasks.size() << endl;
        return;
    }
    
    cerr << "找到 " << hall_task_ids.size() << " 个厅内保障任务，固定人选 " << hall_fixed_persons.size() << " 人" << endl;
    
    // 按时间排序（通过task_ptr_map获取指针）
    sort(hall_task_ids.begin(), hall_task_ids.end(), 
              [&task_ptr_map](const string& id_a, const string& id_b) {
                  auto it_a = task_ptr_map.find(id_a);
                  auto it_b = task_ptr_map.find(id_b);
                  if (it_a == task_ptr_map.end() || it_a->second == nullptr ||
                      it_b == task_ptr_map.end() || it_b->second == nullptr) {
                      return id_a < id_b;  // 如果找不到，按ID排序
                  }
                  return it_a->second->getStartTime() < it_b->second->getStartTime();
              });
    
    // 轮流值守：根据分组时的决策，第一组先值守第一个时间段，然后每1小时轮换
    bool group1_on_duty = group1_starts_first;
    long last_task_start = -1;
    bool first_task = true;
    bool first_shift_count_incremented = false;  // 记录是否已经增加过第一次值守次数
    
    for (const string& task_id : hall_task_ids) {
        // 通过task_ptr_map获取任务指针，确保使用最新指针（避免指针失效）
        auto task_it = task_ptr_map.find(task_id);
        if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
            cerr << "[ERROR] 任务ID=" << task_id << " 不在指针映射中或指针为空！跳过" << endl;
            continue;
        }
        
        TaskDefinition* task = task_it->second;
        long task_start = task->getStartTime();
        
        cerr << "[DEBUG] 开始处理厅内任务 ID=" << task_id 
             << ", 名称=" << task->getTaskName()
             << ", 指针=" << static_cast<void*>(task)
             << ", 开始时间=" << task_start
             << ", 需要人数=" << task->getRequiredCount()
             << ", 当前已分配=" << task->getAssignedEmployeeCount() << endl;
        
        // 如果是第一个任务，根据分组决策设置group1_on_duty
        if (first_task) {
            first_task = false;
            group1_on_duty = group1_starts_first;
        } else if (last_task_start >= 0) {
            // 检查是否需要轮换（每1小时，即3600秒）
            // 如果当前任务开始时间比上一个任务开始时间晚1小时或以上，则轮换
            if (task_start >= last_task_start + 3600) {
                group1_on_duty = !group1_on_duty;
            }
        }
        
        // 分配值守任务
        const auto& on_duty_group = group1_on_duty ? group1 : group2;
        const auto& off_duty_group = group1_on_duty ? group2 : group1;
        
        cerr << "[DEBUG] 任务ID=" << task_id << " 值守组=" << (group1_on_duty ? "group1" : "group2")
             << ", 值守组人数=" << on_duty_group.size() << endl;
        
        // 为值守组分配厅内保障任务（维护双向映射：任务->员工 和 员工->任务）
        for (const auto& employee_id : on_duty_group) {
            if (!task->isAssignedToEmployee(employee_id)) {
                // 维护双向映射：任务->人员（任务->人员）
                task->addAssignedEmployeeId(employee_id);
                
                // 维护双向映射：人员->任务（人员->任务）
                auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
                if (employee) {
                    employee->addAssignedTaskId(task->getTaskId());
                }
                cerr << "[DEBUG] 任务ID=" << task_id << " 分配给员工 " << employee_id << endl;
            }
        }
        
        // 如果任务需要的人数超过值守组人数，从另一组补充（维护双向映射）
        int assigned_count = static_cast<int>(task->getAssignedEmployeeCount());
        int required_count = task->getRequiredCount();
        cerr << "[DEBUG] 任务ID=" << task_id << " 已分配=" << assigned_count 
             << ", 需要=" << required_count << endl;
        
        if (assigned_count < required_count) {
            for (const auto& employee_id : off_duty_group) {
                if (assigned_count >= required_count) break;
                if (!task->isAssignedToEmployee(employee_id)) {
                    // 维护双向映射：任务->人员（任务->人员）
                    task->addAssignedEmployeeId(employee_id);
                    
                    // 维护双向映射：人员->任务（人员->任务）
                    auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
                    if (employee) {
                        employee->addAssignedTaskId(task->getTaskId());
                    }
                    assigned_count++;
                    cerr << "[DEBUG] 任务ID=" << task_id << " 从非值守组补充分配给员工 " << employee_id << endl;
                }
            }
        }
        
        // 为不值守的组分配操作间任务（允许完全重叠）
        // 操作间任务的时间段与厅内保障任务相同（不值守的组在这个时间段做操作间任务）
        scheduleOperationRoomTasks(tasks, shifts, task_ptr_map, off_duty_group,
                                    task->getStartTime(), task->getEndTime());
        
        // 在添加操作间任务后，重新从task_ptr_map获取任务指针，确保使用最新指针
        // 虽然已经预留了容量，但为了安全起见，还是重新获取指针
        auto updated_task_it = task_ptr_map.find(task_id);
        if (updated_task_it != task_ptr_map.end() && updated_task_it->second != nullptr) {
            task = updated_task_it->second;
            cerr << "[DEBUG] 任务ID=" << task_id << " 在添加操作间任务后，重新获取指针=" 
                 << static_cast<void*>(task) << endl;
        } else {
            cerr << "[ERROR] 任务ID=" << task_id << " 在添加操作间任务后，无法从映射中获取指针！" << endl;
        }
        
        // 更新第一次值守次数（只在第一次任务且group1先值守时增加）
        if (!first_shift_count_incremented && group1_on_duty && group1_starts_first) {
            for (const auto& employee_id : group1) {
                incrementFirstShiftCount(employee_id);
            }
            first_shift_count_incremented = true;
        } else if (!first_shift_count_incremented && !group1_on_duty && !group1_starts_first) {
            // 如果group2先值守，增加group2的第一次值守次数
            for (const auto& employee_id : group2) {
                incrementFirstShiftCount(employee_id);
            }
            first_shift_count_incremented = true;
        }
        
        last_task_start = task->getStartTime();
        task->setAssigned(true);
        
        // 验证分配结果
        int final_assigned = static_cast<int>(task->getAssignedEmployeeCount());
        cerr << "[DEBUG] 任务ID=" << task_id << " 分配完成，最终已分配人数=" << final_assigned 
             << ", 任务指针=" << static_cast<void*>(task) << endl;
        
        if (final_assigned == 0) {
            cerr << "[ERROR] 警告：任务ID=" << task->getTaskId() << " (" << task->getTaskName() 
                 << ") 在scheduleHallMaintenanceTasks结束后仍未被分配，任务指针=" 
                 << static_cast<void*>(task) << endl;
        } else {
            cout << "厅内任务ID=" << task->getTaskId() << " (" << task->getTaskName() 
                 << ") 已分配 " << final_assigned << " 人" << endl;
        }
    }
    
    cerr << "厅内保障任务分配完成，共处理 " << hall_task_ids.size() << " 个任务" << endl;
    
    // 验证所有厅内保障任务的分配状态（通过task_ptr_map验证）
    cerr << "[DEBUG] 开始验证厅内保障任务的分配状态..." << endl;
    for (const string& task_id : hall_task_ids) {
        auto it = task_ptr_map.find(task_id);
        if (it != task_ptr_map.end() && it->second != nullptr) {
            TaskDefinition* mapped_task = it->second;
            int assigned_count = static_cast<int>(mapped_task->getAssignedEmployeeCount());
            cerr << "[DEBUG] 任务ID=" << task_id << " 验证通过: 已分配人数=" 
                 << assigned_count << ", 指针=" << static_cast<void*>(mapped_task) << endl;
            if (assigned_count == 0) {
                cerr << "[ERROR] 任务ID=" << task_id << " (" << mapped_task->getTaskName() 
                     << ") 验证失败：分配后仍为0人！" << endl;
            }
        } else {
            cerr << "[ERROR] 任务ID=" << task_id 
                 << " 不在任务指针映射中或指针为空！" << endl;
        }
    }
    cerr << "[DEBUG] 厅内保障任务分配状态验证完成" << endl;
}

void TaskScheduler::scheduleOperationRoomTasks(vector<TaskDefinition>& tasks,
                                               const vector<Shift>& shifts,
                                               map<string, TaskDefinition*>& task_ptr_map,
                                               const vector<string>& off_duty_employees,
                                               long time_slot_start,
                                               long time_slot_end)
{
    // 查找或创建操作间任务
    TaskDefinition* operation_task = nullptr;
    for (auto& task : tasks) {
        if (task.getTaskType() == TaskType::OPERATION_ROOM &&
            task.getStartTime() == time_slot_start &&
            task.getEndTime() == time_slot_end) {
            operation_task = &task;
            break;
        }
    }
    
    // 如果不存在，创建一个新的操作间任务
    if (!operation_task) {
        tasks.emplace_back();
        operation_task = &tasks.back();
        operation_task->setTaskType(TaskType::OPERATION_ROOM);
        operation_task->setTaskName("操作间任务");
        operation_task->setStartTime(time_slot_start);
        operation_task->setEndTime(time_slot_end);
        operation_task->setRequiredCount(2);
        operation_task->setAllowOverlap(true);  // 允许重叠
        operation_task->setMaxOverlapTime(60);  // 最大重叠时间60秒
        operation_task->setRequiredQualification(static_cast<int>(QualificationMask::HALL_INTERNAL));
        operation_task->setCanNewEmployee(true);
        operation_task->setPreferMainShift(true);
        
        // 生成任务ID（使用时间戳）
        string task_id = "operation_" + to_string(time_slot_start) + "_" + to_string(static_cast<long>(TaskType::OPERATION_ROOM));
        operation_task->setTaskId(task_id);
        task_ptr_map[task_id] = operation_task;
    }
    
    // 为不值守的员工分配操作间任务（允许完全重叠，维护双向映射）
    for (const auto& employee_id : off_duty_employees) {
        if (!operation_task->isAssignedToEmployee(employee_id)) {
            // 检查员工是否有厅内资质
            auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
            if (employee && employee->hasQualification(QualificationMask::HALL_INTERNAL)) {
                // 维护双向映射：任务->人员（任务->人员）
                operation_task->addAssignedEmployeeId(employee_id);
                
                // 维护双向映射：人员->任务（人员->任务）
                employee->addAssignedTaskId(operation_task->getTaskId());
            }
        }
    }
    
    operation_task->setAssigned(true);
}

}  // namespace vip_first_class

