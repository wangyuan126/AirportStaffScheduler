/**
 * @file load_scheduler.cpp
 * @brief 装卸任务调度类实现
 */

#include "load_scheduler.h"
#include "stand_distance.h"
#include <algorithm>
#include <map>
#include <set>
#include <tuple>
#include <climits>
#include <iostream>
#include <sstream>

namespace zhuangxie_class {

using namespace std;
using namespace vip_first_class;

LoadScheduler::LoadScheduler()
{
}

LoadScheduler::~LoadScheduler()
{
}

void LoadScheduler::scheduleLoadTasks(const vector<LoadEmployeeInfo>& employees,
                                     vector<LoadTask>& tasks,
                                     const vector<Shift>& shifts,
                                     const vector<ShiftBlockPeriod>& block_periods,
                                     const vector<LoadTask>* previous_tasks,
                                     const map<string, vector<string>>* group_name_to_employees)
{
    // 1. 验证任务时间约束：任务必须在起飞和落地之间完成
    // 注意：现在使用最早开始时间和最晚结束时间，不再需要调整
    // 约束检查在分配时进行：actual_start_time >= earliest_start_time && actual_start_time + duration <= latest_end_time
    
    // 2. 按任务保障优先级排序任务
    sortTasksByPriority(tasks);
    
    // 3. 分配任务给员工
    assignTasksToEmployees(tasks, employees, shifts, block_periods, previous_tasks, 
                          group_name_to_employees ? *group_name_to_employees : map<string, vector<string>>());
}


void LoadScheduler::sortTasksByPriority(vector<LoadTask>& tasks)
{
    // 任务保障优先级排序规则：
    // 1. 进港 > 出港
    // 2. 落地时间早的 > 落地时间晚的
    
    sort(tasks.begin(), tasks.end(), [](const LoadTask& a, const LoadTask& b) {
        // 1. 进港 > 出港
        int type_a = a.getFlightType();
        int type_b = b.getFlightType();
        bool a_is_arrival = (type_a == static_cast<int>(FlightType::DOMESTIC_ARRIVAL) ||
                            type_a == static_cast<int>(FlightType::INTERNATIONAL_ARRIVAL));
        bool b_is_arrival = (type_b == static_cast<int>(FlightType::DOMESTIC_ARRIVAL) ||
                            type_b == static_cast<int>(FlightType::INTERNATIONAL_ARRIVAL));
        bool a_is_transit_arrival = (type_a == static_cast<int>(FlightType::DOMESTIC_TRANSIT) ||
                                    type_a == static_cast<int>(FlightType::INTERNATIONAL_TRANSIT));
        bool b_is_transit_arrival = (type_b == static_cast<int>(FlightType::DOMESTIC_TRANSIT) ||
                                    type_b == static_cast<int>(FlightType::INTERNATIONAL_TRANSIT));
        
        // 进港任务（包括过站的进港部分）优先
        if (a_is_arrival || (a_is_transit_arrival && a.getTaskName().find("进港") != string::npos)) {
            if (!(b_is_arrival || (b_is_transit_arrival && b.getTaskName().find("进港") != string::npos))) {
                return true;  // a是进港，b不是，a优先
            }
        } else if (b_is_arrival || (b_is_transit_arrival && b.getTaskName().find("进港") != string::npos)) {
            return false;  // b是进港，a不是，b优先
        }
        
        // 3. 落地时间早的 > 落地时间晚的
        long arrival_a = a.getArrivalTime();
        long arrival_b = b.getArrivalTime();
        if (arrival_a != arrival_b) {
            return arrival_a < arrival_b;
        }
        
        // 如果都相同，按任务ID排序
        return a.getTaskId() < b.getTaskId();
    });
}


// 辅助函数：检查员工在指定时间段是否空闲
static bool isEmployeeAvailable(const string& employee_id, long task_actual_start, long task_duration,
                                const map<string, LoadTask*>& task_ptr_map,
                                const map<string, const LoadEmployeeInfo*>& employee_map)
{
    auto emp_it = employee_map.find(employee_id);
    if (emp_it == employee_map.end()) {
        return false;
    }
    
    const LoadEmployeeInfo* emp = emp_it->second;
    const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
    
    long task_actual_end = task_actual_start + task_duration;
    
    for (const string& assigned_task_id : assigned_task_ids) {
        auto task_it = task_ptr_map.find(assigned_task_id);
        if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
            continue;
        }
        
        const LoadTask& assigned_task = *(task_it->second);
        long assigned_start = assigned_task.getActualStartTime();
        long assigned_end = assigned_task.getActualEndTime();
        
        // 如果已分配任务的实际开始时间为0，说明未分配，跳过
        if (assigned_start <= 0) {
            continue;
        }
        
        // 检查时间段是否重叠
        if (!(assigned_end <= task_actual_start || task_actual_end <= assigned_start)) {
            return false;  // 时间段重叠，员工不空闲
        }
    }
    
    return true;  // 员工空闲
}

// 辅助函数：计算组的当日任务总时长
static long calculateGroupDailyTaskTime(const vector<string>& group_members, long current_task_start,
                                           const map<string, LoadTask*>& task_ptr_map,
                                           const map<string, const LoadEmployeeInfo*>& employee_map)
{
    const long SECONDS_PER_DAY = 24 * 3600;
    long current_day = current_task_start / SECONDS_PER_DAY;
    long total_time = 0;
    
    for (const string& employee_id : group_members) {
        auto emp_it = employee_map.find(employee_id);
        if (emp_it == employee_map.end()) {
            continue;
        }
        
        const LoadEmployeeInfo* emp = emp_it->second;
        const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
        
        for (const string& assigned_task_id : assigned_task_ids) {
            auto task_it = task_ptr_map.find(assigned_task_id);
            if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
                continue;
            }
            
            const LoadTask& assigned_task = *(task_it->second);
            long assigned_start = assigned_task.getActualStartTime();
            
            // 如果实际开始时间为0，说明未分配，跳过
            if (assigned_start <= 0) {
                continue;
            }
            
            long task_day = assigned_start / SECONDS_PER_DAY;
            
            if (task_day == current_day) {
                long task_duration = assigned_task.getDuration();
                if (task_duration > 0) {
                    total_time += task_duration;
                }
            }
        }
    }
    
    return total_time;
}

void LoadScheduler::assignTasksToEmployees(vector<LoadTask>& tasks,
                                          const vector<LoadEmployeeInfo>& employees,
                                          const vector<Shift>& shifts,
                                          const vector<ShiftBlockPeriod>& block_periods,
                                          const vector<LoadTask>* previous_tasks,
                                          const map<string, vector<string>>& group_name_to_employees)
{
    const int GROUP_SIZE = 3;  // 每个组3个人
    
    // 创建任务ID到LoadTask指针的映射
    map<string, LoadTask*> task_ptr_map;
    for (auto& task : tasks) {
        task_ptr_map[task.getTaskId()] = &task;
    }
    
    // 创建员工ID到LoadEmployeeInfo的映射
    map<string, const LoadEmployeeInfo*> employee_map;
    for (const auto& emp : employees) {
        employee_map[emp.getEmployeeId()] = &emp;
    }
    
    // 直接从group_name_to_employees构建组到员工的映射
    // 班组名一致的就是一个小组
    map<int, vector<string>> groups;  // 内部组ID -> 员工ID列表
    map<int, string> group_id_to_name;  // 内部组ID -> 班组名
    int internal_group_id = 1;
    
    // 从group_name_to_employees中提取员工，按班组名分组
    for (const auto& g_pair : group_name_to_employees) {
        const string& group_name = g_pair.first;
        const vector<string>& emp_list = g_pair.second;
        
        // 按3人一组分割
            for (size_t i = 0; i < emp_list.size(); i += GROUP_SIZE) {
                vector<string> group_members;
                for (size_t j = i; j < emp_list.size() && j < i + GROUP_SIZE; ++j) {
                    group_members.push_back(emp_list[j]);
                }
                
                if (group_members.size() == GROUP_SIZE) {
                    // 完整的3人组，分配一个新的内部组ID
                    groups[internal_group_id] = group_members;
                group_id_to_name[internal_group_id] = group_name;
                    internal_group_id++;
                }
            }
        }
    
    // 调试输出：检查组构建情况
    cerr << "DEBUG: Built " << groups.size() << " groups from " << group_name_to_employees.size() << " group names" << endl;
    for (const auto& g_pair : groups) {
        cerr << "DEBUG: Group " << g_pair.first << " (" << group_id_to_name[g_pair.first] << ") has " << g_pair.second.size() << " members" << endl;
    }
    
    // 注意：任务已经按优先级排序，这里不再重新排序，保持优先级顺序
    // 使用任务ID集合来跟踪已处理的任务
    set<string> processed_task_ids;
    
    // 轮转机制：记录当前轮到哪个组
    // 根据班组名出现的顺序，k个小组轮流派工（不固定为8个）
    vector<int> rotation_order;  // 按轮转顺序存储组ID
    
    // 构建轮转顺序：按班组名在group_name_to_employees中出现的顺序
    vector<string> group_name_order;
    for (const auto& g_pair : group_name_to_employees) {
        group_name_order.push_back(g_pair.first);
    }
    
    // 按班组名出现的顺序构建轮转顺序
    vector<pair<int, string>> temp_groups;  // (group_id, group_name)
    for (const auto& group_pair : groups) {
        int group_id = group_pair.first;
        if (group_pair.second.empty()) {
            continue;
        }
        
        // 从group_id_to_name中获取班组名
        string group_name;
        if (group_id_to_name.find(group_id) != group_id_to_name.end()) {
            group_name = group_id_to_name[group_id];
            temp_groups.push_back({group_id, group_name});
        }
    }
    
    // 按班组名出现的顺序排序
    sort(temp_groups.begin(), temp_groups.end(), 
         [&group_name_order](const pair<int, string>& a, const pair<int, string>& b) {
             auto it_a = find(group_name_order.begin(), group_name_order.end(), a.second);
             auto it_b = find(group_name_order.begin(), group_name_order.end(), b.second);
             if (it_a != group_name_order.end() && it_b != group_name_order.end()) {
                 return it_a < it_b;
             }
             return a.second < b.second;
         });
    
    // 添加到轮转顺序
    for (const auto& tg : temp_groups) {
        rotation_order.push_back(tg.first);
    }
    
    // 如果没有找到轮转顺序，按组ID排序
    if (rotation_order.empty()) {
        for (const auto& group_pair : groups) {
            rotation_order.push_back(group_pair.first);
        }
        sort(rotation_order.begin(), rotation_order.end());
    }
    
    // 创建1000个元素的轮换数组，内容是循环的：2,3,4,5,6,7,8,1,2,3,4,5,6,7,8,1,2,3...
    vector<int> rotation_array(1000);
    int pattern[] = {2, 3, 4, 5, 6, 7, 8, 1};  // 循环模式
    int pattern_size = 8;
    for (int i = 0; i < 1000; ++i) {
        rotation_array[i] = pattern[i % pattern_size];
    }
    
    int current_rotation_index = 0;  // 当前轮转索引
    
    // 小组位置交换逻辑：当某个小组繁忙时，尝试交换轮换数组中两个数字的位置
    auto trySwapInRotationArray = [&](int busy_group_value, long task_actual_start, long task_duration,
                                     const map<string, LoadTask*>& task_ptr_map,
                                     const map<string, const LoadEmployeeInfo*>& employee_map) -> bool {
        // 在轮换数组中查找一个可用的组值来交换位置
        for (int i = 0; i < 1000; ++i) {
            int candidate_value = rotation_array[i];
            if (candidate_value == busy_group_value) {
                continue;
            }
            
            // 检查是否有对应这个值的组，且该组在任务时间段可用
            bool found_available_group = false;
            for (const auto& group_pair : groups) {
                int group_id = group_pair.first;
                // 检查组ID是否匹配候选值（这里假设组ID就是值，或者需要建立映射）
                // 如果组ID范围是1-8，直接比较
                if (group_id == candidate_value) {
                    const vector<string>& candidate_members = group_pair.second;
                    bool all_available = true;
                    
                    for (const string& emp_id : candidate_members) {
                        auto emp_it = employee_map.find(emp_id);
                        if (emp_it == employee_map.end()) {
                            all_available = false;
                            break;
                        }
                        
                        if (!isEmployeeAvailable(emp_id, task_actual_start, task_duration, task_ptr_map, employee_map)) {
                            all_available = false;
                            break;
                        }
                    }
                    
                    if (all_available) {
                        found_available_group = true;
                        break;
                    }
                }
            }
            
            if (found_available_group) {
                // 交换两个值的位置
                for (int j = 0; j < 1000; ++j) {
                    if (rotation_array[j] == busy_group_value) {
                        rotation_array[j] = candidate_value;
                        rotation_array[i] = busy_group_value;
                        return true;  // 交换成功
                    }
                }
            }
        }
        
        return false;  // 没有找到可交换的组
    };
    
    // 遍历任务列表，逐个分配任务
    int task_index = 0;
    cerr << "DEBUG: Total tasks to process: " << tasks.size() << endl;
    
    for (auto& task : tasks) {
        task_index++;
        string task_id = task.getTaskId();
        
        // 如果任务ID为空，使用索引作为唯一标识符
        if (task_id.empty()) {
            task_id = "task_" + to_string(task_index);
            task.setTaskId(task_id);
        }
        
        // 输出前20个任务的ID，检查是否有重复
        if (task_index <= 20) {
            cerr << "DEBUG: Processing task index " << task_index << ", task_id=" << task_id 
                 << ", name=" << task.getTaskName() << endl;
        }
        
        // 跳过已经处理过的任务
        if (processed_task_ids.find(task_id) != processed_task_ids.end()) {
            if (task_index <= 20) {
                cerr << "DEBUG: Task " << task_id << " (index " << task_index << ") already processed, skipping" << endl;
            }
            continue;
        }
        
        // 跳过已经分配的任务
        if (task.isAssigned() && task.getAssignedEmployeeCount() > 0) {
            processed_task_ids.insert(task_id);
            if (task_index <= 10) {
                cerr << "DEBUG: Task " << task_id << " already assigned, skipping" << endl;
            }
            continue;
        }
        
        int assigned_count = static_cast<int>(task.getAssignedEmployeeCount());
        int required_count = task.getRequiredCount();
        
        long earliest_start = task.getEarliestStartTime();
        long latest_end = task.getLatestEndTime();
        long duration = task.getDuration();
        
        // 调试输出：检查任务时间
        if (task_index <= 10) {  // 输出前10个任务的调试信息
            cerr << "DEBUG: Task " << task_id << " (" << task.getTaskName() << ") earliest_start: " << earliest_start 
                 << ", latest_end: " << latest_end << ", duration: " << duration
                 << ", required: " << required_count << ", assigned: " << assigned_count << endl;
        }
        
        // 如果任务时间无效，跳过（但不标记为已处理，因为可能后续可以修复）
        if (earliest_start <= 0 || latest_end <= 0 || duration <= 0 || earliest_start + duration > latest_end) {
            if (task_index <= 10) {
                cerr << "DEBUG: Task " << task_id << " has invalid time (earliest_start: " << earliest_start 
                     << ", latest_end: " << latest_end << ", duration: " << duration << "), skipping" << endl;
            }
            // 不标记为已处理，因为时间可能后续可以修复
            continue;
        }
        
        // 获取当前任务的机位信息（直接从task中获取）
        int task_stand = task.getStand();
        
        // 判断是否是早出港任务（08:00前）
        const long EIGHT_AM_SECONDS = 8 * 3600;  // 08:00 = 28800秒（从当天0点开始）
        long task_day = earliest_start / (24 * 3600);
        long task_time_in_day = earliest_start % (24 * 3600);
        bool is_early_departure = (task_time_in_day < EIGHT_AM_SECONDS) && 
                                   (task.getTaskName().find("出港") != string::npos);
        
        // 计算需要的组数（3人一组）
        int required_groups = (required_count + GROUP_SIZE - 1) / GROUP_SIZE;  // 向上取整
        
        // 检查是否可以在上一次预排方案中保留分配（减少调整）
        if (previous_tasks != nullptr) {
            // 查找上一次预排方案中相同任务ID的分配
            for (const auto& prev_task : *previous_tasks) {
                if (prev_task.getTaskId() == task_id && 
                    prev_task.isAssigned() && 
                    prev_task.getAssignedEmployeeCount() > 0) {
                    // 检查上一次分配的小组是否仍然可用
                    const auto& prev_assigned = prev_task.getAssignedEmployeeIds();
                    bool can_reuse = true;
                    
                    // 检查组是否仍然完整且在任务时间段空闲
                    // 注意：需要确保上一次分配的人数是3的倍数（整组）
                    if (prev_assigned.size() % GROUP_SIZE != 0) {
                        can_reuse = false;  // 不是整组，不能重用
                    }
                    
                    if (can_reuse) {
                        // 计算实际开始时间（使用最早开始时间）
                        long actual_start = earliest_start;
                        long actual_end = actual_start + duration;
                        
                        // 检查约束：实际开始时间 + 时长 <= 最晚结束时间
                        if (actual_end > latest_end) {
                            can_reuse = false;  // 不满足约束
                        } else {
                            for (const string& emp_id : prev_assigned) {
                                if (employee_map.find(emp_id) == employee_map.end() ||
                                    task.isAssignedToEmployee(emp_id) ||
                                    !isEmployeeAvailable(emp_id, actual_start, duration, task_ptr_map, employee_map)) {
                                    can_reuse = false;
                                    break;
                                }
                            }
                        }
                    }
                    
                    if (can_reuse) {
                        // 重用上一次的分配（整组重用）
                        // 设置实际开始时间（使用最早开始时间）
                        long actual_start = earliest_start;
                        task.setActualStartTime(actual_start);
                        
                        for (const string& emp_id : prev_assigned) {
                            task.addAssignedEmployeeId(emp_id);
                            auto emp_it = employee_map.find(emp_id);
                            if (emp_it != employee_map.end()) {
                                const_cast<LoadEmployeeInfo*>(emp_it->second)->getEmployeeInfo()
                                    .addAssignedTaskId(task_id);
                            }
                            assigned_count++;
                        }
                        if (assigned_count >= required_count) {
                            // 已完全分配，跳过后续分配逻辑
                            task.setAssigned(true);
                            processed_task_ids.insert(task_id);
                            continue;  // 继续下一个任务
                        }
                    }
                    break;
                }
            }
        }
        
        // 分配任务给组（不拆组）
        while (assigned_count < required_count) {
            // 找到所有可用的组（组内所有成员都空闲）
            vector<pair<int, vector<string>>> available_groups;
            
            if (task_index <= 10) {
                cerr << "DEBUG: Task " << task_id << " (" << task.getTaskName() << ") requires " << required_count << " people, currently assigned " << assigned_count << endl;
                cerr << "DEBUG: Checking " << groups.size() << " groups for availability" << endl;
            }
            
            for (const auto& group_pair : groups) {
                int group_id = group_pair.first;
                const vector<string>& group_members = group_pair.second;
                
                // 检查组是否完整（必须有3个人）
                if (group_members.size() < GROUP_SIZE) {
                    continue;
                }
                
                // 检查组内所有成员是否都已分配给当前任务
                bool all_assigned = true;
                for (const string& emp_id : group_members) {
                    if (!task.isAssignedToEmployee(emp_id)) {
                        all_assigned = false;
                        break;
                    }
                }
                if (all_assigned) {
                    continue;  // 该组已经完全分配给当前任务
                }
            
                
                // 检查组内所有成员在任务时间段是否都空闲
                bool all_available = true;
                string unavailable_reason = "";
                for (const string& emp_id : group_members) {
                    // 如果已经分配给当前任务，跳过
                    if (task.isAssignedToEmployee(emp_id)) {
                        continue;
                    }
                    
                    // 计算实际开始时间（使用最早开始时间）
                    long actual_start = earliest_start;
                    if (!isEmployeeAvailable(emp_id, actual_start, duration, task_ptr_map, employee_map)) {
                        all_available = false;
                        unavailable_reason = "employee " + emp_id + " not available";
                        break;
                    }
                }
                
                if (!all_available) {
                    // 移除调试输出，减少日志噪音
                    continue;
                }
                
                // 如果任务有机位信息，检查小组是否能按时到达（路程时间衔接）
                // 注意：这里只做基本验证，如果时间非常紧张（比如只差几秒），仍然允许分配
                if (task_stand > 0) {
                    // 获取该组最近结束的任务的机位和时间
                    int last_stand = 0;
                    long last_end_time = -1;
                    
                    for (const string& emp_id : group_members) {
                        auto emp_it = employee_map.find(emp_id);
                        if (emp_it == employee_map.end()) {
                            continue;
                        }
                        
                        const LoadEmployeeInfo* emp = emp_it->second;
                        const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
                        
                        // 找到该成员最近结束的任务（在当前任务开始之前）
                        for (const string& assigned_task_id : assigned_task_ids) {
                            auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                            if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                continue;
                            }
                            
                            const LoadTask& assigned_task = *(assigned_task_it->second);
                            long assigned_end = assigned_task.getActualEndTime();
                            if (assigned_end > 0 && assigned_end < earliest_start && 
                                assigned_end > last_end_time) {
                                last_end_time = assigned_end;
                                // 获取该任务的机位（直接从task中获取）
                                last_stand = assigned_task.getStand();
                            }
                        }
                    }
                    
                    // 如果找到上次任务，验证是否有足够时间到达当前任务
                    // 放宽条件：允许有5分钟的缓冲时间（300秒）
                    if (last_stand > 0 && last_end_time > 0) {
                        long travel_time = StandDistance::getInstance().getTravelTime(last_stand, task_stand);
                        const long BUFFER_TIME = 5 * 60;  // 5分钟缓冲
                        long actual_start = earliest_start;  // 使用最早开始时间
                        if ((last_end_time + travel_time + BUFFER_TIME) > actual_start) {
                            // 无法按时到达，跳过该组
                            continue;
                        }
                    }
                }
                
                // 组可用且能按时到达
                available_groups.push_back({group_id, group_members});
            }
            
            if (task_index <= 10) {
                cerr << "DEBUG: Found " << available_groups.size() << " available groups for task " << task_id << endl;
            }
            
            // 选择最优的组：优先级 1.轮转顺序 2.连续工作时长 3.机位远近
            long best_score = LONG_MAX;
            int selected_group_id = -1;
            vector<string> selected_group_members;
            bool forced_assignment = false;  // 标记是否是强制分配（时间段被占满）
            
            if (available_groups.empty()) {
                // 没有可用的组，找到最先结束任务的组进行强制分配
                long earliest_end_time = LONG_MAX;
                
                for (const auto& group_pair : groups) {
                    int group_id = group_pair.first;
                    const vector<string>& group_members = group_pair.second;
                    
                    // 检查组是否完整（必须有3个人）
                    if (group_members.size() < GROUP_SIZE) {
                        continue;
                    }
                    
                    // 检查组内所有成员是否都已分配给当前任务（如果全部已分配，强制分配也没用）
                    bool all_assigned_to_current_task = true;
                    for (const string& emp_id : group_members) {
                        if (!task.isAssignedToEmployee(emp_id)) {
                            all_assigned_to_current_task = false;
                            break;
                        }
                    }
                    if (all_assigned_to_current_task) {
                        continue;  // 该组已经完全分配给当前任务，跳过
                    }
                    
                    // 找到该组所有成员中最近结束的任务
                    long group_last_end_time = -1;
                    for (const string& emp_id : group_members) {
                        auto emp_it = employee_map.find(emp_id);
                        if (emp_it == employee_map.end()) {
                            continue;
                        }
                        
                        const LoadEmployeeInfo* emp = emp_it->second;
                        const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
                        
                        for (const string& assigned_task_id : assigned_task_ids) {
                            auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                            if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                continue;
                            }
                            
                            const LoadTask& assigned_task = *(assigned_task_it->second);
                            long assigned_end = assigned_task.getActualEndTime();
                            if (assigned_end > 0 && assigned_end > group_last_end_time) {
                                group_last_end_time = assigned_end;
                            }
                        }
                    }
                    
                    // 如果没有已分配任务，设为0（最早）
                    if (group_last_end_time < 0) {
                        group_last_end_time = 0;
                    }
                    
                    // 选择最先结束任务的组（如果没有任务，则选择最早）
                    if (group_last_end_time < earliest_end_time) {
                        earliest_end_time = group_last_end_time;
                        selected_group_id = group_id;
                        selected_group_members = group_members;
                        forced_assignment = true;
                    }
                }
                
                if (selected_group_id < 0) {
                    // 实在找不到组，标记为缺少人手
                    task.setShortStaffed(true);
                    break;
                }
            } else {
                // 有可用组，按正常轮转逻辑选择
                // 创建可用组的映射，便于查找
                map<int, vector<string>> available_groups_map;
                for (const auto& group_pair : available_groups) {
                    available_groups_map[group_pair.first] = group_pair.second;
                }
                
                // 早出港派工（08:00前）：临近机位尽量同组保障
                if (is_early_departure && task_stand > 0) {
                    // 查找是否有组在临近机位（相邻或相近机位）
                    int best_group_id = -1;
                    int min_stand_distance = INT_MAX;
                    long min_group_task_time = LONG_MAX;
                    
                    for (const auto& group_pair : available_groups) {
                        int group_id = group_pair.first;
                        const vector<string>& group_members = group_pair.second;
                        
                        // 获取该组最近任务的机位
                        int last_stand = 0;
                        long last_end_time = -1;
                        for (const string& emp_id : group_members) {
                            auto emp_it = employee_map.find(emp_id);
                            if (emp_it == employee_map.end()) {
                                continue;
                            }
                            const LoadEmployeeInfo* emp = emp_it->second;
                            const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
                            
                            for (const string& assigned_task_id : assigned_task_ids) {
                                auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                                if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                    continue;
                                }
                                const LoadTask& assigned_task = *(assigned_task_it->second);
                                long assigned_end = assigned_task.getActualEndTime();
                                if (assigned_end > 0 && assigned_end < earliest_start && 
                                    assigned_end > last_end_time) {
                                    last_end_time = assigned_end;
                                    // 获取该任务的机位（直接从task中获取）
                                    last_stand = assigned_task.getStand();
                                }
                            }
                        }
                        
                        if (last_stand > 0) {
                            // 计算机位距离（绝对值）
                            int stand_distance = abs(last_stand - task_stand);
                            if (stand_distance < min_stand_distance) {
                                min_stand_distance = stand_distance;
                                best_group_id = group_id;
                                min_group_task_time = calculateGroupDailyTaskTime(group_members, earliest_start, task_ptr_map, employee_map);
                            } else if (stand_distance == min_stand_distance) {
                                // 如果距离相同，选择当日工时较少的组（均衡疲劳度）
                                long group_task_time = calculateGroupDailyTaskTime(group_members, earliest_start, task_ptr_map, employee_map);
                                if (group_task_time < min_group_task_time) {
                                    min_group_task_time = group_task_time;
                                    best_group_id = group_id;
                                }
                            }
                        } else {
                            // 如果该组没有上一个任务，也考虑（选择当日工时较少的组）
                            long group_task_time = calculateGroupDailyTaskTime(group_members, earliest_start, task_ptr_map, employee_map);
                            if (min_stand_distance == INT_MAX && group_task_time < min_group_task_time) {
                                min_group_task_time = group_task_time;
                                best_group_id = group_id;
                            }
                        }
                    }
                    
                    if (best_group_id >= 0) {
                        selected_group_id = best_group_id;
                        selected_group_members = available_groups_map[best_group_id];
                        // 更新轮转索引
                        for (size_t i = 0; i < rotation_order.size(); ++i) {
                            if (rotation_order[i] == best_group_id) {
                                current_rotation_index = (i + 1) % rotation_order.size();
                                break;
                            }
                        }
                    }
                }
                
                // 如果早出港没有找到临近机位的组，或者不是早出港，按正常轮转逻辑
                bool found_by_rotation = false;  // 移到外层作用域，以便后续使用
                if (selected_group_id < 0) {
                    // 计算实际开始时间（使用最早开始时间）
                    long actual_start = earliest_start;
                    
                    // 维护一个set来记录不可用的小组（1-8组）
                    set<int> unavailable_groups;
                    
                    // 从轮换数组的当前位置开始，依次检查对应的小组是否可用
                    for (int offset = 0; offset < 1000; ++offset) {
                        int idx = (current_rotation_index + offset) % 1000;
                        int candidate_group_value = rotation_array[idx];
                        
                        // 只处理1-8组
                        if (candidate_group_value < 1 || candidate_group_value > 8) {
                            continue;
                        }
                        
                        // 查找对应这个值的组
                        bool group_found = false;
                        for (const auto& group_pair : groups) {
                            int group_id = group_pair.first;
                            // 检查组ID是否匹配候选值（假设组ID范围是1-8）
                            if (group_id == candidate_group_value) {
                                group_found = true;
                                auto it = available_groups_map.find(group_id);
                                if (it != available_groups_map.end()) {
                                    // 找到可用的组，分配任务
                                    selected_group_id = group_id;
                                    selected_group_members = it->second;
                                    found_by_rotation = true;
                                    // 更新轮转索引到下一个
                                    current_rotation_index = (idx + 1) % 1000;
                                    break;
                                } else {
                                    // 组不可用，加入set
                                    unavailable_groups.insert(group_id);
                                    
                                    // 如果所有1-8组都不可用，找到最先结束的小组进行强制分配
                                    if (unavailable_groups.size() == 8) {
                                        long earliest_end_time = LONG_MAX;
                                        int earliest_end_group_id = -1;
                                        vector<string> earliest_end_group_members;
                                        
                                        for (const auto& group_pair : groups) {
                                            int gid = group_pair.first;
                                            // 只考虑1-8组
                                            if (gid < 1 || gid > 8) {
                                                continue;
                                            }
                                            
                                            const vector<string>& g_members = group_pair.second;
                                            
                                            // 检查组是否完整（必须有3个人）
                                            if (g_members.size() < GROUP_SIZE) {
                                                continue;
                                            }
                                            
                                            // 找到该组所有成员中最近结束的任务
                                            long group_last_end_time = -1;
                                            for (const string& emp_id : g_members) {
                                                auto emp_it = employee_map.find(emp_id);
                                                if (emp_it == employee_map.end()) {
                                                    continue;
                                                }
                                                
                                                const LoadEmployeeInfo* emp = emp_it->second;
                                                const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
                                                
                                                for (const string& assigned_task_id : assigned_task_ids) {
                                                    auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                                                    if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                                        continue;
                                                    }
                                                    
                                                    const LoadTask& assigned_task = *(assigned_task_it->second);
                                                    long assigned_end = assigned_task.getActualEndTime();
                                                    if (assigned_end > 0 && assigned_end > group_last_end_time) {
                                                        group_last_end_time = assigned_end;
                                                    }
                                                }
                                            }
                                            
                                            // 如果没有已分配任务，设为0（最早）
                                            if (group_last_end_time < 0) {
                                                group_last_end_time = 0;
                                            }
                                            
                                            // 选择最先结束任务的组
                                            if (group_last_end_time < earliest_end_time) {
                                                earliest_end_time = group_last_end_time;
                                                earliest_end_group_id = gid;
                                                earliest_end_group_members = g_members;
                                            }
                                        }
                                        
                                        if (earliest_end_group_id >= 0) {
                                            selected_group_id = earliest_end_group_id;
                                            selected_group_members = earliest_end_group_members;
                                            forced_assignment = true;
                                            // 更新轮转索引到下一个
                                            current_rotation_index = (idx + 1) % 1000;
                                        }
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        
                        if (found_by_rotation || forced_assignment) {
                            break;
                        }
                        
                        // 如果组不存在，也加入set（避免无限循环）
                        if (!group_found && candidate_group_value >= 1 && candidate_group_value <= 8) {
                            unavailable_groups.insert(candidate_group_value);
                            if (unavailable_groups.size() == 8) {
                                // 所有1-8组都不可用或不存在，找到最先结束的小组
                                long earliest_end_time = LONG_MAX;
                                int earliest_end_group_id = -1;
                                vector<string> earliest_end_group_members;
                                
                                for (const auto& group_pair : groups) {
                                    int gid = group_pair.first;
                                    if (gid < 1 || gid > 8) {
                                        continue;
                                    }
                                    
                                    const vector<string>& g_members = group_pair.second;
                                    if (g_members.size() < GROUP_SIZE) {
                                        continue;
                                    }
                                    
                                    long group_last_end_time = -1;
                                    for (const string& emp_id : g_members) {
                                        auto emp_it = employee_map.find(emp_id);
                                        if (emp_it == employee_map.end()) {
                                            continue;
                                        }
                                        
                                        const LoadEmployeeInfo* emp = emp_it->second;
                                        const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
                                        
                                        for (const string& assigned_task_id : assigned_task_ids) {
                                            auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                                            if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                                continue;
                                            }
                                            
                                            const LoadTask& assigned_task = *(assigned_task_it->second);
                                            long assigned_end = assigned_task.getActualEndTime();
                                            if (assigned_end > 0 && assigned_end > group_last_end_time) {
                                                group_last_end_time = assigned_end;
                                            }
                                        }
                                    }
                                    
                                    if (group_last_end_time < 0) {
                                        group_last_end_time = 0;
                                    }
                                    
                                    if (group_last_end_time < earliest_end_time) {
                                        earliest_end_time = group_last_end_time;
                                        earliest_end_group_id = gid;
                                        earliest_end_group_members = g_members;
                                    }
                                }
                                
                                if (earliest_end_group_id >= 0) {
                                    selected_group_id = earliest_end_group_id;
                                    selected_group_members = earliest_end_group_members;
                                    forced_assignment = true;
                                    current_rotation_index = (idx + 1) % 1000;
                                }
                                break;
                            }
                        }
                    }
                } else {
                    // 如果selected_group_id >= 0，说明已经通过前面的逻辑找到了，不是通过轮转
                    found_by_rotation = false;
                }
            
                // 如果按轮转顺序没有找到可用组，则按综合得分选择（减少调整）
                // 同时考虑：临近下班小组任务指派、小组休息时优先为当日工时较少的小组分配任务
                if (selected_group_id < 0) {
                    for (const auto& group_pair : available_groups) {
                    int group_id = group_pair.first;
                    
                    // 优先级1：轮转顺序（基于轮换数组）
                    int rotation_position = INT_MAX;
                    for (int i = 0; i < 1000; ++i) {
                        int idx = (current_rotation_index + i) % 1000;
                        if (rotation_array[idx] == group_id) {
                            rotation_position = i;
                            break;
                        }
                    }
                    if (rotation_position == INT_MAX) {
                        rotation_position = 10000;
                    }
                
                    // 优先级2：临近下班小组任务指派（尽量为小组分配不延误下班时间的机位任务）
                    // 检查任务结束时间是否在下班时间之后（这里简化处理，假设下班时间为22:00）
                    const long DEFAULT_OFF_DUTY_TIME = 22 * 3600;  // 22:00 = 79200秒
                    long actual_start = earliest_start;
                    long actual_end = actual_start + duration;
                    long task_day = actual_start / (24 * 3600);
                    long off_duty_time = task_day * (24 * 3600) + DEFAULT_OFF_DUTY_TIME;
                    bool task_delays_off_duty = (actual_end > off_duty_time);
                    
                    // 如果任务会延误下班，优先选择当日工时较少的组（这些组可能更早下班）
                    long group_daily_task_time = calculateGroupDailyTaskTime(group_pair.second, actual_start, task_ptr_map, employee_map);
                    
                    // 优先级3：计算连续工作时长
                    long continuous_work_duration = 0;
                    if (task_stand > 0) {
                        // 收集该组所有成员的所有已分配任务（在当前任务开始之前的）
                        vector<tuple<long, long, int>> prev_tasks;  // (start_time, end_time, stand)
                        
                        for (const string& emp_id : group_pair.second) {
                            auto emp_it = employee_map.find(emp_id);
                            if (emp_it == employee_map.end()) {
                                continue;
                            }
                            
                            const LoadEmployeeInfo* emp = emp_it->second;
                            const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
                            
                            for (const string& assigned_task_id : assigned_task_ids) {
                                auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                                if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                    continue;
                                }
                                
                                const LoadTask& assigned_task = *(assigned_task_it->second);
                                long assigned_end = assigned_task.getActualEndTime();
                                // 只考虑在当前任务开始之前的任务
                                if (assigned_end > 0 && assigned_end < earliest_start) {
                                    // 获取该任务的机位（直接从task中获取）
                                    int assigned_stand = assigned_task.getStand();
                                    
                                    prev_tasks.push_back({assigned_task.getActualStartTime(), 
                                                          assigned_end, 
                                                          assigned_stand});
                                }
                            }
                        }
                        
                        // 如果有上一个任务，计算连续工作时长
                        if (!prev_tasks.empty()) {
                            // 按开始时间排序
                            sort(prev_tasks.begin(), prev_tasks.end());
                            
                            // 从最近的任务开始，向前查找连续的任务链
                            long actual_start = earliest_start;
                            long actual_end = actual_start + duration;
                            long current_end = actual_end;
                            long current_start = actual_start;
                            int current_stand = task_stand;
                            long chain_start = actual_start;
                            
                            // 反向遍历，查找连续的任务
                            for (int i = static_cast<int>(prev_tasks.size()) - 1; i >= 0; --i) {
                                long prev_start = get<0>(prev_tasks[i]);
                                long prev_end = get<1>(prev_tasks[i]);
                                int prev_stand = get<2>(prev_tasks[i]);
                                
                                // 计算路程时间
                                long travel_time = 0;
                                if (prev_stand > 0 && current_stand > 0) {
                                    travel_time = StandDistance::getInstance().getTravelTime(prev_stand, current_stand);
                                } else if (prev_stand > 0) {
                                    travel_time = 5 * 60;  // 默认5分钟
                                }
                                
                                // 如果上一个任务结束时间 + 路程时间 <= 当前任务开始时间，认为是连续的
                                if (prev_end + travel_time <= current_start) {
                                    // 更新连续链的开始时间
                                    chain_start = prev_start;
                                    current_start = prev_start;
                                    current_stand = prev_stand;
                                    // 继续向前查找
                                } else {
                                    // 不连续，停止查找
                                    break;
                                }
                            }
                            
                            // 计算连续工作时长
                            continuous_work_duration = current_end - chain_start;
                        } else {
                            // 没有上一个任务，连续工作时长就是当前任务时长
                            continuous_work_duration = duration;
                        }
                    }
                    
                    // 优先级4：计算路程时间（机位远近）
                    long travel_time_score = 0;
                    if (task_stand > 0) {
                        // 获取该组上次任务的结束机位
                        int last_stand = 0;
                        long last_end_time = -1;
                        
                        for (const string& emp_id : group_pair.second) {
                            auto emp_it = employee_map.find(emp_id);
                            if (emp_it == employee_map.end()) {
                                continue;
                            }
                            
                            const LoadEmployeeInfo* emp = emp_it->second;
                            const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
                            
                            // 找到该组所有成员中最近结束的任务
                            for (const string& assigned_task_id : assigned_task_ids) {
                                auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                                if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                    continue;
                                }
                                
                                const LoadTask& assigned_task = *(assigned_task_it->second);
                                long assigned_end = assigned_task.getActualEndTime();
                                if (assigned_end > 0 && assigned_end < earliest_start && 
                                    assigned_end > last_end_time) {
                                    last_end_time = assigned_end;
                                    // 获取该任务的机位（直接从task中获取）
                                    last_stand = assigned_task.getStand();
                                }
                            }
                        }
                        
                        if (last_stand > 0) {
                            travel_time_score = StandDistance::getInstance().getTravelTime(last_stand, task_stand);
                        }
                    }
                    
                    // 综合得分：优先级1（轮转顺序）* 1000000 + 优先级2（连续工作时长）* 100 + 优先级3（路程时间）
                    // 得分越小越好
                    // 如果任务会延误下班，增加当日工时较少的组的权重（优先选择）
                    long off_duty_penalty = 0;
                    if (task_delays_off_duty) {
                        // 当日工时较少的组得分更低（更优先）
                        off_duty_penalty = group_daily_task_time / 100;  // 工时越少，惩罚越小
                    } else {
                        // 任务不会延误下班，优先选择当日工时较少的组（小组休息时优先分配）
                        off_duty_penalty = group_daily_task_time / 100;
                    }
                    
                    long total_score = rotation_position * 1000000 + 
                                         continuous_work_duration * 100 + 
                                         travel_time_score +
                                         off_duty_penalty;
                    
                    if (total_score < best_score) {
                        best_score = total_score;
                        selected_group_id = group_pair.first;
                        selected_group_members = group_pair.second;
                    }
                    }
                
                    // 如果通过综合得分选择了组，更新轮转索引（基于轮换数组）
                    if (!found_by_rotation && selected_group_id >= 0) {
                        for (int i = 0; i < 1000; ++i) {
                            int idx = (current_rotation_index + i) % 1000;
                            if (rotation_array[idx] == selected_group_id) {
                                current_rotation_index = (idx + 1) % 1000;
                                break;
                            }
                        }
                    }
                }
            }
            
            if (selected_group_id < 0) {
                if (task_index <= 10) {
                    cerr << "DEBUG: Task " << task_id << " no available groups found, marking as short-staffed" << endl;
                }
                task.setShortStaffed(true);
                break;
            }
            
            if (task_index <= 10) {
                cerr << "DEBUG: Task " << task_id << " selected group " << selected_group_id << " with " << selected_group_members.size() << " members" << endl;
            }
            
            // 计算实际开始时间（使用最早开始时间）
            long actual_start = earliest_start;
            long actual_end = actual_start + duration;
            
            // 检查约束：实际开始时间 + 时长 <= 最晚结束时间
            if (actual_end > latest_end) {
                if (task_index <= 10) {
                    cerr << "DEBUG: Task " << task_id << " cannot be scheduled: actual_end (" << actual_end 
                         << ") > latest_end (" << latest_end << ")" << endl;
                }
                task.setShortStaffed(true);
                break;
            }
            
            // 设置实际开始时间（只在第一次分配时设置）
            if (task.getActualStartTime() == 0) {
                task.setActualStartTime(actual_start);
            }
            
            // 分配该组的所有成员到任务
            int assigned_in_this_iteration = 0;  // 记录本次循环中实际分配的人数
            for (const string& emp_id : selected_group_members) {
                // 如果已经分配，跳过
                if (task.isAssignedToEmployee(emp_id)) {
                    continue;
                }
                
                // 分配任务给员工（即使是强制分配也执行）
                task.addAssignedEmployeeId(emp_id);
                
                // 维护双向映射：员工->任务
                auto emp_it = employee_map.find(emp_id);
                if (emp_it != employee_map.end()) {
                    const_cast<LoadEmployeeInfo*>(emp_it->second)->getEmployeeInfo()
                        .addAssignedTaskId(task_id);
                }
                
                assigned_count++;
                assigned_in_this_iteration++;
            }
            
            // 防止死循环：如果本次循环中没有任何新的人员被分配（所有成员都已分配），
            // 说明无法继续分配，应该退出循环
            if (assigned_in_this_iteration == 0) {
                // 本次循环没有分配任何人，说明已经没有可用的组了
                task.setShortStaffed(true);
                break;
            }
            
            // 如果使用了强制分配（时间段被占满），继续向后轮转
            if (forced_assignment) {
                // 找到被分配组在轮换数组中的位置，然后继续向后轮转
                for (int i = 0; i < 1000; ++i) {
                    int idx = (current_rotation_index + i) % 1000;
                    if (rotation_array[idx] == selected_group_id) {
                        current_rotation_index = (idx + 1) % 1000;
                        break;
                    }
                }
                // 如果没有在轮换数组中找到（理论上不应该发生），则不需要更新轮转索引
            }
        }
        
        // 更新任务状态
        if (assigned_count > 0) {
            task.setAssigned(true);
            if (task_index <= 10) {
                cerr << "DEBUG: Task " << task_id << " assigned " << assigned_count << " out of " << required_count << " required" << endl;
        }
        cout << "任务 ID " << task_id 
             << " (名称: " << task.getTaskName() 
             << ") 已分配 " << assigned_count 
             << " 人，需求 " << required_count << " 人。" << endl;
        } else {
            if (task_index <= 10) {
                cerr << "DEBUG: Task " << task_id << " failed to assign any employees" << endl;
            }
        }
        
        // 标记为已处理（无论是否成功分配，都标记为已处理，避免重复处理）
        processed_task_ids.insert(task_id);
    }
    
    cout << "装卸任务调度完成！" << endl;
}

// 已废弃：scheduleLoadTasksFromCommon函数已删除，请直接使用loadLoadTasksFromCSV加载LoadTask，然后调用scheduleLoadTasks

}  // namespace zhuangxie_class

