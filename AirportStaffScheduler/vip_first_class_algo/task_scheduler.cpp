/**
 * @file task_scheduler.cpp
 * @brief 任务调度类实现
 */

#include "task_scheduler.h"
#include "task_config.h"
#include "employee_manager.h"
#include "employee_info.h"
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
map<string, int32_t> TaskScheduler::first_shift_counts_;

TaskScheduler::TaskScheduler()
{
}

TaskScheduler::~TaskScheduler()
{
}

// 辅助函数：检查两个时间段是否重叠
static bool isTimeOverlap(int64_t start1, int64_t end1, int64_t start2, int64_t end2, bool allow_overlap, int64_t max_overlap_time)
{
    // 如果任务结束时间为-1（航后），默认使用22:30（81000秒）
    const int64_t DEFAULT_AFTER_FLIGHT_TIME = 22 * 3600 + 30 * 60;  // 22:30 = 81000秒
    
    // 处理航后任务（end_time < 0）
    int64_t actual_end1 = (end1 < 0) ? DEFAULT_AFTER_FLIGHT_TIME : end1;
    int64_t actual_end2 = (end2 < 0) ? DEFAULT_AFTER_FLIGHT_TIME : end2;
    
    // 检查时间段重叠
    bool overlap = !(actual_end1 <= start2 || actual_end2 <= start1);
    
    if (!overlap) {
        return false;
    }
    
    // 如果允许重叠，检查重叠时间是否超过最大允许时间
    if (allow_overlap && max_overlap_time > 0) {
        int64_t overlap_start = max(start1, start2);
        int64_t overlap_end = min(actual_end1, actual_end2);
        int64_t overlap_duration = overlap_end - overlap_start;
        return overlap_duration > max_overlap_time;
    }
    
    // 不允许重叠或重叠时间超过限制
    return true;
}

// 辅助函数：检查员工在指定时间段是否空闲
static bool isEmployeeAvailable(const string& employee_id, int64_t task_start, int64_t task_end, 
                                 bool task_allow_overlap, int64_t task_max_overlap_time,
                                 const map<int64_t, TaskDefinition*>& task_ptr_map)
{
    auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
    if (!employee) {
        return false;
    }
    
    // 遍历员工的所有任务
    const auto& assigned_task_ids = employee->getAssignedTaskIds();
    for (int64_t assigned_task_id : assigned_task_ids) {
        auto task_it = task_ptr_map.find(assigned_task_id);
        if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
            continue;
        }
        
        const TaskDefinition& assigned_task = *(task_it->second);
        int64_t assigned_start = assigned_task.getStartTime();
        int64_t assigned_end = assigned_task.getEndTime();
        
        // 检查时间段是否重叠（考虑两个任务的允许重叠设置）
        bool allow_overlap = task_allow_overlap && assigned_task.allowOverlap();
        int64_t max_overlap = max(task_max_overlap_time, assigned_task.getMaxOverlapTime());
        
        if (isTimeOverlap(task_start, task_end, assigned_start, assigned_end, allow_overlap, max_overlap)) {
            return false;  // 时间段重叠，员工不空闲
        }
    }
    
    return true;  // 员工空闲
}

// 辅助函数：计算员工当日已分配任务的总时长（秒）
static int64_t calculateEmployeeDailyTaskTime(const string& employee_id, 
                                                int64_t current_task_start_time,
                                                const map<int64_t, TaskDefinition*>& task_ptr_map)
{
    const int64_t SECONDS_PER_DAY = 24 * 3600;  // 一天的秒数
    const int64_t DEFAULT_AFTER_FLIGHT_TIME = 22 * 3600 + 30 * 60;  // 22:30 = 81000秒
    
    // 计算当前任务所属的日期（从2020-01-01开始的第几天）
    int64_t current_day = current_task_start_time / SECONDS_PER_DAY;
    
    auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
    if (!employee) {
        return 0;
    }
    
    int64_t total_task_time = 0;
    
    // 遍历员工的所有已分配任务
    const auto& assigned_task_ids = employee->getAssignedTaskIds();
    for (int64_t assigned_task_id : assigned_task_ids) {
        auto task_it = task_ptr_map.find(assigned_task_id);
        if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
            continue;
        }
        
        const TaskDefinition& assigned_task = *(task_it->second);
        int64_t task_start = assigned_task.getStartTime();
        
        // 计算任务所属的日期
        int64_t task_day = task_start / SECONDS_PER_DAY;
        
        // 只计算当日的任务
        if (task_day == current_day) {
            int64_t task_end = assigned_task.getEndTime();
            
            // 处理航后任务（end_time < 0）
            int64_t actual_end;
            if (task_end < 0) {
                // 航后任务：结束时间为当天的22:30
                actual_end = task_day * SECONDS_PER_DAY + DEFAULT_AFTER_FLIGHT_TIME;
            } else {
                actual_end = task_end;
            }
            
            // 计算任务时长（秒）
            int64_t task_duration = actual_end - task_start;
            if (task_duration > 0) {
                total_task_time += task_duration;
            }
        }
    }
    
    return total_task_time;
}

// 辅助函数：检查一个时间段是否包含另一个时间段
// 如果outer时间段完全包含inner时间段，返回true
static bool isTimeRangeContains(int64_t outer_start, int64_t outer_end, 
                                 int64_t inner_start, int64_t inner_end)
{
    const int64_t DEFAULT_AFTER_FLIGHT_TIME = 22 * 3600 + 30 * 60;  // 22:30 = 81000秒
    const int64_t SECONDS_PER_DAY = 24 * 3600;
    
    // 处理航后任务
    int64_t actual_outer_end = (outer_end < 0) ? (outer_start / SECONDS_PER_DAY) * SECONDS_PER_DAY + DEFAULT_AFTER_FLIGHT_TIME : outer_end;
    int64_t actual_inner_end = (inner_end < 0) ? (inner_start / SECONDS_PER_DAY) * SECONDS_PER_DAY + DEFAULT_AFTER_FLIGHT_TIME : inner_end;
    
    // outer时间段包含inner时间段：outer_start <= inner_start 且 outer_end >= inner_end
    return outer_start <= inner_start && actual_outer_end >= actual_inner_end;
}

// 辅助函数：检查任务是否是固定任务（通过员工ID和班次信息）
static bool isTaskFixedForEmployee(int64_t task_id, TaskType task_type, const string& employee_id,
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
        int32_t shift_type = shift.getShiftType();
        if (shift_type == 0) {
            continue;
        }
        
        const auto& position_map = shift.getPositionToEmployeeId();
        for (const auto& pos_pair : position_map) {
            if (pos_pair.second == employee_id) {
                int32_t position = pos_pair.first;
                
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

int32_t TaskScheduler::getFirstShiftCount(const string& employee_id)
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
        int32_t priority_a = TaskConfig::getInstance().getTaskPriority(a.getTaskType());
        int32_t priority_b = TaskConfig::getInstance().getTaskPriority(b.getTaskType());
        
        // 优先级高的排在前面（数值越大优先级越高）
        if (priority_a != priority_b) {
            return priority_a > priority_b;
        }
        
        // 如果优先级相同，按照任务ID排序（保持稳定排序）
        return a.getTaskId() < b.getTaskId();
    });
    
    // 2. 创建任务ID到TaskDefinition指针的映射，方便查找和更新
    map<int64_t, TaskDefinition*> task_ptr_map;
    for (auto& task : tasks) {
        task_ptr_map[task.getTaskId()] = &task;
        // 调试：检查任务12-23是否在映射中
        if (task.getTaskId() >= 12 && task.getTaskId() <= 23) {
            cerr << "[DEBUG] 初始映射: 任务ID=" << task.getTaskId() 
                 << ", 指针=" << static_cast<void*>(&task)
                 << ", 名称=" << task.getTaskName() << endl;
        }
    }
    cerr << "[DEBUG] 任务指针映射建立完成，共 " << task_ptr_map.size() << " 个任务" << endl;
    
    // 2.1 先处理厅内保障任务（4人，2人一组轮流值守）
    scheduleHallMaintenanceTasks(tasks, shifts, task_ptr_map);
    
    // 2.2 在添加操作间任务后，重新建立任务指针映射，确保所有指针都是最新的
    // 因为scheduleHallMaintenanceTasks可能会添加新的操作间任务，导致tasks向量重新分配
    task_ptr_map.clear();
    for (auto& task : tasks) {
        task_ptr_map[task.getTaskId()] = &task;
        // 调试：检查任务12-23在重新映射后的状态
        if (task.getTaskId() >= 12 && task.getTaskId() <= 23) {
            cerr << "[DEBUG] 厅内任务处理后重新映射: 任务ID=" << task.getTaskId() 
                 << ", 指针=" << static_cast<void*>(&task)
                 << ", 名称=" << task.getTaskName()
                 << ", 已分配=" << task.isAssigned()
                 << ", 已分配人数=" << task.getAssignedEmployeeCount() << endl;
        }
    }
    cerr << "[DEBUG] 厅内任务处理后，任务指针映射已更新，共 " << task_ptr_map.size() << " 个任务" << endl;
    
    // 3. 使用任务ID集合来跟踪已处理的任务
    set<int64_t> processed_task_ids;
    
    // 4. 遍历任务列表，逐个分配任务
    size_t current_index = 0;
    while (current_index < tasks.size()) {
        TaskDefinition& task = tasks[current_index];
        int64_t task_id = task.getTaskId();
        
        // 调试：检查任务12-23在主循环中的状态
        if (task_id >= 12 && task_id <= 23) {
            cerr << "[DEBUG] 主循环处理任务ID=" << task_id 
                 << ", 名称=" << task.getTaskName()
                 << ", 指针=" << static_cast<void*>(&task)
                 << ", 已分配=" << task.isAssigned()
                 << ", 已分配人数=" << task.getAssignedEmployeeCount()
                 << ", 是否在处理列表中=" << (processed_task_ids.find(task_id) != processed_task_ids.end()) << endl;
        }
        
        // 跳过已经处理过的任务
        if (processed_task_ids.find(task.getTaskId()) != processed_task_ids.end()) {
            if (task_id >= 12 && task_id <= 23) {
                cerr << "[DEBUG] 任务ID=" << task_id << " 已在处理列表中，跳过" << endl;
            }
            current_index++;
            continue;
        }
        
        // 跳过已经分配的任务
        if (task.isAssigned() && task.getAssignedEmployeeCount() > 0) {
            if (task_id >= 12 && task_id <= 23) {
                cerr << "[DEBUG] 任务ID=" << task_id << " 已分配，标记为已处理" << endl;
            }
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
            // 调试：检查厅内保障任务的状态
            if (task_id >= 12 && task_id <= 23) {
                cerr << "[DEBUG] 任务ID=" << task_id << " 是厅内保障任务，已分配=" 
                     << task.isAssigned() << ", 已分配人数=" << task.getAssignedEmployeeCount()
                     << ", 任务指针=" << static_cast<void*>(&task) << endl;
                // 验证指针映射中的指针是否一致
                auto it = task_ptr_map.find(task_id);
                if (it != task_ptr_map.end()) {
                    cerr << "[DEBUG] 任务ID=" << task_id << " 在指针映射中，映射指针=" 
                         << static_cast<void*>(it->second) << endl;
                    if (it->second != &task) {
                        cerr << "[ERROR] 任务ID=" << task_id 
                             << " 指针不一致！映射指针=" << static_cast<void*>(it->second)
                             << " != 当前任务指针=" << static_cast<void*>(&task) << endl;
                    }
                    // 验证映射中的任务状态
                    if (it->second) {
                        cerr << "[DEBUG] 映射中的任务状态: 已分配=" << it->second->isAssigned()
                             << ", 已分配人数=" << it->second->getAssignedEmployeeCount() << endl;
                    }
                } else {
                    cerr << "[ERROR] 任务ID=" << task_id << " 不在指针映射中！" << endl;
                }
                
                // 如果任务未分配，输出错误信息
                if (!task.isAssigned() || task.getAssignedEmployeeCount() == 0) {
                    cerr << "[ERROR] 任务ID=" << task_id << " (" << task.getTaskName() 
                         << ") 是厅内保障任务但未被分配！将跳过处理" << endl;
                }
            }
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
                int32_t shift_type = shift.getShiftType();
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
        
        // 对所有固定人选候选进行空闲检查并分配
        for (const auto& fixed_employee_id : fixed_employee_candidates) {
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
            int64_t min_daily_task_time = INT64_MAX;
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
                    
                    // 检查是否空闲
                    if (isEmployeeAvailable(employee_id, task.getStartTime(), task.getEndTime(),
                                             task.allowOverlap(), task.getMaxOverlapTime(), task_ptr_map)) {
                        // 计算该员工当日已分配任务的总时长
                        int64_t daily_task_time = calculateEmployeeDailyTaskTime(employee_id, 
                                                                                  task.getStartTime(), 
                                                                                  task_ptr_map);
                        if (daily_task_time < min_daily_task_time) {
                            min_daily_task_time = daily_task_time;
                            selected_employee_id = employee_id;
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
                int64_t task_id;
                TaskDefinition* task_ptr;
                int64_t priority;
            };
            
            vector<ReplaceableTask> replaceable_tasks;
            int64_t current_priority = TaskConfig::getInstance().getTaskPriority(task.getTaskType());
            
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
                    for (int64_t assigned_task_id : assigned_task_ids) {
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
                                int64_t assigned_priority = TaskConfig::getInstance().getTaskPriority(assigned_task.getTaskType());
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
                        int32_t priority_a = TaskConfig::getInstance().getTaskPriority(a.getTaskType());
                        int32_t priority_b = TaskConfig::getInstance().getTaskPriority(b.getTaskType());
                        
                        if (priority_a != priority_b) {
                            return priority_a > priority_b;
                        }
                        
                        return a.getTaskId() < b.getTaskId();
                    });
                    
                    // 重新建立任务指针映射
                    task_ptr_map.clear();
                    for (auto& t : tasks) {
                        task_ptr_map[t.getTaskId()] = &t;
                        // 调试：检查任务12-23在重新映射后的状态
                        if (t.getTaskId() >= 12 && t.getTaskId() <= 23) {
                            cerr << "[DEBUG] 重新映射后: 任务ID=" << t.getTaskId() 
                                 << ", 指针=" << static_cast<void*>(&t)
                                 << ", 已分配=" << t.isAssigned()
                                 << ", 已分配人数=" << t.getAssignedEmployeeCount() << endl;
                        }
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

void TaskScheduler::scheduleHallMaintenanceTasks(vector<TaskDefinition>& tasks,
                                                 const vector<Shift>& shifts,
                                                 map<int64_t, TaskDefinition*>& task_ptr_map)
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
        int32_t sum1 = getFirstShiftCount(hall_fixed_persons[0]) + getFirstShiftCount(hall_fixed_persons[1]);
        int32_t sum2 = getFirstShiftCount(hall_fixed_persons[2]) + getFirstShiftCount(hall_fixed_persons[3]);
        
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
        int32_t sum1 = getFirstShiftCount(hall_fixed_persons[0]) + getFirstShiftCount(hall_fixed_persons[1]);
        int32_t sum2 = getFirstShiftCount(hall_fixed_persons[2]);
        
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
        int32_t sum1 = getFirstShiftCount(hall_fixed_persons[0]);
        int32_t sum2 = getFirstShiftCount(hall_fixed_persons[1]);
        
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
    vector<int64_t> hall_task_ids;
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
              [&task_ptr_map](int64_t id_a, int64_t id_b) {
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
    int64_t last_task_start = -1;
    bool first_task = true;
    bool first_shift_count_incremented = false;  // 记录是否已经增加过第一次值守次数
    
    for (int64_t task_id : hall_task_ids) {
        // 通过task_ptr_map获取任务指针，确保使用最新指针（避免指针失效）
        auto task_it = task_ptr_map.find(task_id);
        if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
            cerr << "[ERROR] 任务ID=" << task_id << " 不在指针映射中或指针为空！跳过" << endl;
            continue;
        }
        
        TaskDefinition* task = task_it->second;
        int64_t task_start = task->getStartTime();
        
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
    for (int64_t task_id : hall_task_ids) {
        auto it = task_ptr_map.find(task_id);
        if (it != task_ptr_map.end() && it->second != nullptr) {
            TaskDefinition* mapped_task = it->second;
            int assigned_count = static_cast<int>(mapped_task->getAssignedEmployeeCount());
            cerr << "[DEBUG] 任务ID=" << task_id << " 验证通过: 已分配人数=" 
                 << assigned_count << ", 指针=" << static_cast<void*>(mapped_task) << endl;
            if (assigned_count == 0 && task_id >= 12 && task_id <= 23) {
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
                                               map<int64_t, TaskDefinition*>& task_ptr_map,
                                               const vector<string>& off_duty_employees,
                                               int64_t time_slot_start,
                                               int64_t time_slot_end)
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
        operation_task->setRequiredQualification(static_cast<int32_t>(QualificationMask::HALL_INTERNAL));
        operation_task->setCanNewEmployee(true);
        operation_task->setPreferMainShift(true);
        
        // 生成任务ID（使用时间戳）
        int64_t task_id = time_slot_start * 1000 + static_cast<int64_t>(TaskType::OPERATION_ROOM);
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

