/**
 * @file load_scheduler.cpp
 * @brief 装卸任务调度类实现
 */

#include "load_scheduler.h"
#include "stand_distance.h"
#include "../CommonAdapterUtils.h"
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
                                     const vector<Flight>& flights,
                                     const vector<Shift>& shifts,
                                     vector<TaskDefinition>& tasks,
                                     long default_travel_time,
                                     const vector<ShiftBlockPeriod>& block_periods,
                                     const vector<TaskDefinition>* previous_tasks)
{
    // 1. 从航班信息生成任务，同时建立任务ID到航班索引的映射
    map<long, size_t> flight_task_map;
    generateTasksFromFlights(flights, tasks, default_travel_time, flight_task_map);
    
    // 2. 按任务保障优先级排序任务
    sortTasksByPriority(tasks, flights, flight_task_map);
    
    // 3. 分配任务给员工
    assignTasksToEmployees(tasks, employees, shifts, flights, block_periods, previous_tasks, flight_task_map);
}

void LoadScheduler::generateTasksFromFlights(const vector<Flight>& flights,
                                            vector<TaskDefinition>& tasks,
                                            long default_travel_time,
                                            map<long, size_t>& flight_task_map)
{
    long task_id = 1;
    const long MINUTES_5 = 5 * 60;      // 5分钟
    const long MINUTES_10 = 10 * 60;    // 10分钟
    const long MINUTES_15 = 15 * 60;    // 15分钟（远机位叫车时间）
    const long MINUTES_60 = 60 * 60;    // 60分钟
    const long MINUTES_90 = 90 * 60;    // 90分钟（长过站阈值）
    
    for (size_t flight_idx = 0; flight_idx < flights.size(); ++flight_idx) {
        const auto& flight = flights[flight_idx];
        int flight_type = flight.getFlightType();
        long arrival_time = flight.getArrivalTime();
        long departure_time = flight.getDepartureTime();
        long vip_travel_time = flight.getVipTravelTime();
        bool is_remote_stand = flight.isRemoteStand();
        double arrival_cargo = flight.getArrivalCargo();
        double departure_cargo = flight.getDepartureCargo();
        
        // 如果没有指定通勤时间，使用默认通勤时间
        if (vip_travel_time <= 0) {
            vip_travel_time = default_travel_time;
        }
        
        // 远机位需要提前15分钟叫车
        long vehicle_call_time = is_remote_stand ? MINUTES_15 : 0;
        
        bool is_domestic = (flight_type == static_cast<int>(FlightType::DOMESTIC_ARRIVAL) ||
                           flight_type == static_cast<int>(FlightType::DOMESTIC_DEPARTURE) ||
                           flight_type == static_cast<int>(FlightType::DOMESTIC_TRANSIT));
        bool is_international = (flight_type == static_cast<int>(FlightType::INTERNATIONAL_ARRIVAL) ||
                                flight_type == static_cast<int>(FlightType::INTERNATIONAL_DEPARTURE) ||
                                flight_type == static_cast<int>(FlightType::INTERNATIONAL_TRANSIT));
        
        if (flight_type == static_cast<int>(FlightType::DOMESTIC_ARRIVAL) ||
            flight_type == static_cast<int>(FlightType::INTERNATIONAL_ARRIVAL)) {
            // 进港任务：落地前5分钟 ~ 起飞前5分钟
            // 开始时间 = 进港时间 - 5分钟 - 通勤时间 - 远机位叫车时间
            // 结束时间 = 起飞前5分钟（如果有起飞时间），否则为落地后一定时间
            TaskDefinition task;
            task.setTaskId(task_id++);
            long task_start_time = arrival_time - MINUTES_5 - vip_travel_time - vehicle_call_time;
            long task_end_time;
            // 如果departure_time有效且大于arrival_time，使用起飞前5分钟；否则使用落地后30分钟
            if (departure_time > arrival_time) {
                task_end_time = departure_time - MINUTES_5;
            } else {
                task_end_time = arrival_time + 30 * 60;  // 落地后30分钟
            }
            
            // 确保开始时间 < 结束时间
            if (task_start_time >= task_end_time) {
                task_end_time = task_start_time + 60 * 60;  // 至少1小时
            }
            
            task.setStartTime(task_start_time);
            task.setEndTime(task_end_time);
            task.setTaskName(is_domestic ? "国内进港装卸" : "国际进港装卸");
            
            // 计算需要人数：正常3人，单进2.5t以上6人
            int required_count = 3;
            if (arrival_cargo >= 2.5) {
                required_count = 6;
            }
            
            // 国际通常派2个组保障（6人）
            if (is_international && required_count < 6) {
                required_count = 6;
            }
            
            task.setRequiredCount(required_count);
            task.setPreferMainShift(true);
            task.setAllowOverlap(false);
            task.setMaxOverlapTime(0);
            task.setCanNewEmployee(false);
            task.setAssigned(false);
            task.setShortStaffed(false);
            
            tasks.push_back(task);
            flight_task_map[task.getTaskId()] = flight_idx;
            
        } else if (flight_type == static_cast<int>(FlightType::DOMESTIC_DEPARTURE) ||
                   flight_type == static_cast<int>(FlightType::INTERNATIONAL_DEPARTURE)) {
            // 出港任务：起飞前60分钟 ~ 起飞前10分钟
            // 开始时间 = 出港时间 - 60分钟 - 通勤时间 - 远机位叫车时间
            // 结束时间 = 出港时间 - 10分钟
            TaskDefinition task;
            task.setTaskId(task_id++);
            long task_start_time = departure_time - MINUTES_60 - vip_travel_time - vehicle_call_time;
            long task_end_time = departure_time - MINUTES_10;
            
            // 确保开始时间 < 结束时间
            if (task_start_time >= task_end_time) {
                task_end_time = task_start_time + 60 * 60;  // 至少1小时
            }
            
            task.setStartTime(task_start_time);
            task.setEndTime(task_end_time);
            task.setTaskName(is_domestic ? "国内出港装卸" : "国际出港装卸");
            
            // 计算需要人数：正常3人，单出2t以上6人
            int required_count = 3;
            if (departure_cargo >= 2.0) {
                required_count = 6;
            }
            
            // 国际通常派2个组保障（6人）
            if (is_international && required_count < 6) {
                required_count = 6;
            }
            
            task.setRequiredCount(required_count);
            task.setPreferMainShift(true);
            task.setAllowOverlap(false);
            task.setMaxOverlapTime(0);
            task.setCanNewEmployee(false);
            task.setAssigned(false);
            task.setShortStaffed(false);
            
            tasks.push_back(task);
            flight_task_map[task.getTaskId()] = flight_idx;
            
        } else if (flight_type == static_cast<int>(FlightType::DOMESTIC_TRANSIT) ||
                   flight_type == static_cast<int>(FlightType::INTERNATIONAL_TRANSIT)) {
            // 过站任务
            long transit_duration = departure_time - arrival_time;
            bool is_long_transit = transit_duration > MINUTES_90;
            
            if (is_long_transit) {
                // 长过站（>90分钟）可分为两部分：进港卸货、出港装货
                
                // 进港卸货任务：落地前5分钟 ~ 落地后（简化处理，设为落地后10分钟）
                TaskDefinition arrival_task;
                arrival_task.setTaskId(task_id++);
                long arrival_task_start = arrival_time - MINUTES_5 - vip_travel_time - vehicle_call_time;
                long arrival_task_end = arrival_time + MINUTES_10;  // 落地后10分钟
                
                // 确保开始时间 < 结束时间
                if (arrival_task_start >= arrival_task_end) {
                    arrival_task_end = arrival_task_start + 60 * 60;  // 至少1小时
                }
                
                arrival_task.setStartTime(arrival_task_start);
                arrival_task.setEndTime(arrival_task_end);
                arrival_task.setTaskName(is_domestic ? "国内过站-进港卸货" : "国际过站-进港卸货");
                
                // 计算需要人数：正常3人，单进2.5t以上6人
                int arrival_count = 3;
                if (arrival_cargo >= 2.5) {
                    arrival_count = 6;
                }
                // 国际通常派2个组保障（6人）
                if (is_international && arrival_count < 6) {
                    arrival_count = 6;
                }
                arrival_task.setRequiredCount(arrival_count);
                
                arrival_task.setPreferMainShift(true);
                arrival_task.setAllowOverlap(false);
                arrival_task.setMaxOverlapTime(0);
                arrival_task.setCanNewEmployee(false);
                arrival_task.setAssigned(false);
                arrival_task.setShortStaffed(false);
                
                tasks.push_back(arrival_task);
                flight_task_map[arrival_task.getTaskId()] = flight_idx;
                
                // 出港装货任务：起飞前60分钟 ~ 起飞前10分钟
                TaskDefinition departure_task;
                departure_task.setTaskId(task_id++);
                long departure_task_start = departure_time - MINUTES_60 - vip_travel_time - vehicle_call_time;
                long departure_task_end = departure_time - MINUTES_10;
                
                // 确保开始时间 < 结束时间
                if (departure_task_start >= departure_task_end) {
                    departure_task_end = departure_task_start + 60 * 60;  // 至少1小时
                }
                
                departure_task.setStartTime(departure_task_start);
                departure_task.setEndTime(departure_task_end);
                departure_task.setTaskName(is_domestic ? "国内过站-出港装货" : "国际过站-出港装货");
                
                // 计算需要人数：正常3人，单出2t以上6人
                int departure_count = 3;
                if (departure_cargo >= 2.0) {
                    departure_count = 6;
                }
                // 国际通常派2个组保障（6人）
                if (is_international && departure_count < 6) {
                    departure_count = 6;
                }
                departure_task.setRequiredCount(departure_count);
                
                departure_task.setPreferMainShift(true);
                departure_task.setAllowOverlap(false);
                departure_task.setMaxOverlapTime(0);
                departure_task.setCanNewEmployee(false);
                departure_task.setAssigned(false);
                departure_task.setShortStaffed(false);
                
                tasks.push_back(departure_task);
                flight_task_map[departure_task.getTaskId()] = flight_idx;
                
            } else {
                // 短过站：落地前5分钟 ~ 起飞前5分钟
                TaskDefinition task;
                task.setTaskId(task_id++);
                long task_start_time = arrival_time - MINUTES_5 - vip_travel_time - vehicle_call_time;
                long task_end_time = departure_time - MINUTES_5;
                
                // 确保开始时间 < 结束时间
                if (task_start_time >= task_end_time) {
                    task_end_time = task_start_time + 60 * 60;  // 至少1小时
                }
                
                task.setStartTime(task_start_time);
                task.setEndTime(task_end_time);
                task.setTaskName(is_domestic ? "国内过站装卸" : "国际过站装卸");
                
                // 计算需要人数：正常3人，进出港货量和下2t以上需要6人
                int required_count = 3;
                double total_cargo = arrival_cargo + departure_cargo;
                if (total_cargo >= 2.0) {
                    required_count = 6;
                }
                
                // 国际通常派2个组保障（6人）
                if (is_international && required_count < 6) {
                    required_count = 6;
                }
                
                task.setRequiredCount(required_count);
                task.setPreferMainShift(true);
                task.setAllowOverlap(false);
                task.setMaxOverlapTime(0);
                task.setCanNewEmployee(false);
                task.setAssigned(false);
                task.setShortStaffed(false);
                
                tasks.push_back(task);
                flight_task_map[task.getTaskId()] = flight_idx;
            }
        }
    }
}

void LoadScheduler::sortTasksByPriority(vector<TaskDefinition>& tasks,
                                       const vector<Flight>& flights,
                                       const map<long, size_t>& flight_task_map)
{
    // 任务保障优先级排序规则：
    // 1. 已报时 > 未报时
    // 2. 进港 > 出港
    // 3. 始发航班和过站航班 > 航后航班（这里假设没有航后航班，用进港出港区分）
    // 4. 落地时间早的 > 落地时间晚的
    
    sort(tasks.begin(), tasks.end(), [&](const TaskDefinition& a, const TaskDefinition& b) {
        auto a_it = flight_task_map.find(a.getTaskId());
        auto b_it = flight_task_map.find(b.getTaskId());
        
        if (a_it == flight_task_map.end() || b_it == flight_task_map.end()) {
            return a.getTaskId() < b.getTaskId();  // 如果找不到航班信息，按ID排序
        }
        
        const Flight& flight_a = flights[a_it->second];
        const Flight& flight_b = flights[b_it->second];
        
        // 1. 已报时 > 未报时
        bool a_reported = flight_a.hasReported();
        bool b_reported = flight_b.hasReported();
        if (a_reported != b_reported) {
            return a_reported > b_reported;
        }
        
        // 2. 进港 > 出港
        int type_a = flight_a.getFlightType();
        int type_b = flight_b.getFlightType();
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
        
        // 4. 落地时间早的 > 落地时间晚的
        long arrival_a = flight_a.getArrivalTime();
        long arrival_b = flight_b.getArrivalTime();
        if (arrival_a != arrival_b) {
            return arrival_a < arrival_b;
        }
        
        // 如果都相同，按任务ID排序
        return a.getTaskId() < b.getTaskId();
    });
}

bool LoadScheduler::isShiftBlocked(int shift_type, long time,
                                   const vector<ShiftBlockPeriod>& block_periods) const
{
    for (const auto& period : block_periods) {
        if (period.shift_type == shift_type &&
            time >= period.start_time && time <= period.end_time) {
            return true;
        }
    }
    return false;
}

// 辅助函数：检查员工在指定时间段是否空闲
static bool isEmployeeAvailable(const string& employee_id, long task_start, long task_end,
                                const map<long, TaskDefinition*>& task_ptr_map,
                                const map<string, const LoadEmployeeInfo*>& employee_map)
{
    auto emp_it = employee_map.find(employee_id);
    if (emp_it == employee_map.end()) {
        return false;
    }
    
    const LoadEmployeeInfo* emp = emp_it->second;
    const auto& assigned_task_ids = emp->getEmployeeInfo().getAssignedTaskIds();
    
    for (long assigned_task_id : assigned_task_ids) {
        auto task_it = task_ptr_map.find(assigned_task_id);
        if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
            continue;
        }
        
        const TaskDefinition& assigned_task = *(task_it->second);
        long assigned_start = assigned_task.getStartTime();
        long assigned_end = assigned_task.getEndTime();
        
        // 检查时间段是否重叠
        if (!(assigned_end <= task_start || task_end <= assigned_start)) {
            return false;  // 时间段重叠，员工不空闲
        }
    }
    
    return true;  // 员工空闲
}

// 辅助函数：计算组的当日任务总时长
static long calculateGroupDailyTaskTime(const vector<string>& group_members, long current_task_start,
                                           const map<long, TaskDefinition*>& task_ptr_map,
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
        
        for (long assigned_task_id : assigned_task_ids) {
            auto task_it = task_ptr_map.find(assigned_task_id);
            if (task_it == task_ptr_map.end() || task_it->second == nullptr) {
                continue;
            }
            
            const TaskDefinition& assigned_task = *(task_it->second);
            long assigned_start = assigned_task.getStartTime();
            long task_day = assigned_start / SECONDS_PER_DAY;
            
            if (task_day == current_day) {
                long assigned_end = assigned_task.getEndTime();
                long task_duration = assigned_end - assigned_start;
                if (task_duration > 0) {
                    total_time += task_duration;
                }
            }
        }
    }
    
    return total_time;
}

void LoadScheduler::assignTasksToEmployees(vector<TaskDefinition>& tasks,
                                          const vector<LoadEmployeeInfo>& employees,
                                          const vector<Shift>& shifts,
                                          const vector<Flight>& flights,
                                          const vector<ShiftBlockPeriod>& block_periods,
                                          const vector<TaskDefinition>* previous_tasks,
                                          const map<long, size_t>& flight_task_map)
{
    const int GROUP_SIZE = 3;  // 每个组3个人
    
    // 创建任务ID到TaskDefinition指针的映射
    map<long, TaskDefinition*> task_ptr_map;
    for (auto& task : tasks) {
        task_ptr_map[task.getTaskId()] = &task;
    }
    
    // 创建员工ID到LoadEmployeeInfo的映射
    map<string, const LoadEmployeeInfo*> employee_map;
    for (const auto& emp : employees) {
        employee_map[emp.getEmployeeId()] = &emp;
    }
    
    // 按装卸组组织员工（组ID -> 该组的所有员工ID列表）
    // 注意：同一load_group的主班和副班应该分开成组，不能合并
    map<int, vector<string>> groups;
    int internal_group_id = 1;
    
    for (const auto& shift : shifts) {
        // 跳过休息的班次
        if (shift.getShiftType() == 0) {
            continue;
        }
        
        const auto& position_map = shift.getPositionToEmployeeId();
        vector<string> shift_employees;
        
        // 收集该班次的所有员工
        for (const auto& pos_pair : position_map) {
            const string& employee_id = pos_pair.second;
            auto emp_it = employee_map.find(employee_id);
            if (emp_it == employee_map.end()) {
                continue;
            }
            shift_employees.push_back(employee_id);
        }
        
        // 按load_group分组
        map<int, vector<string>> load_group_employees;
        for (const string& employee_id : shift_employees) {
            auto emp_it = employee_map.find(employee_id);
            if (emp_it == employee_map.end()) {
                continue;
            }
            const LoadEmployeeInfo* emp = emp_it->second;
            int load_group = emp->getLoadGroup();
            
            // 如果员工没有组信息，尝试自动分配组
            if (load_group <= 0) {
                // 找到一个可用的load_group
                load_group = 1;
                while (true) {
                    // 检查该load_group是否已经有完整的组
                    bool found = false;
                    for (const auto& g_pair : groups) {
                        // 简单检查：如果有完整的组且数量合理，就使用下一个
                        // 这里简化处理，直接分配
                    }
                    break;
                }
                const_cast<LoadEmployeeInfo*>(emp)->setLoadGroup(load_group);
            }
            
            load_group_employees[load_group].push_back(employee_id);
        }
        
        // 为每个load_group创建一个独立的组（主班和副班分开，每个shift独立）
        for (const auto& lg_pair : load_group_employees) {
            const vector<string>& emp_list = lg_pair.second;
            
            // 按3人一组分割（通常一个shift就是一个完整的组）
            for (size_t i = 0; i < emp_list.size(); i += GROUP_SIZE) {
                vector<string> group_members;
                for (size_t j = i; j < emp_list.size() && j < i + GROUP_SIZE; ++j) {
                    group_members.push_back(emp_list[j]);
                }
                
                if (group_members.size() == GROUP_SIZE) {
                    // 完整的3人组，分配一个新的内部组ID
                    groups[internal_group_id] = group_members;
                    internal_group_id++;
                }
            }
        }
    }
    
    // 注意：任务已经按优先级排序，这里不再重新排序，保持优先级顺序
    // 使用任务ID集合来跟踪已处理的任务
    set<long> processed_task_ids;
    
    // 轮转机制：记录当前轮到哪个组
    // 按班次类型和load_group排序：主班1,2,3 -> 副班1,2,3
    vector<int> rotation_order;  // 按轮转顺序存储组ID
    
    // 构建轮转顺序：主班1,2,3 -> 副班1,2,3
    // 首先收集所有组的信息（load_group, shift_type）
    map<int, pair<int, int>> group_info_map;  // group_id -> (load_group, shift_type)
    
    for (const auto& group_pair : groups) {
        int group_id = group_pair.first;
        if (group_pair.second.empty()) {
            continue;
        }
        
        // 从第一个员工获取load_group和shift_type
        auto emp_it = employee_map.find(group_pair.second[0]);
        if (emp_it == employee_map.end()) {
            continue;
        }
        
        const LoadEmployeeInfo* emp = emp_it->second;
        int load_group = emp->getLoadGroup();
        int shift_type = 0;
        
        // 查找员工所在的班次类型
        for (const auto& shift : shifts) {
            const auto& position_map = shift.getPositionToEmployeeId();
            for (const auto& pos_pair : position_map) {
                if (pos_pair.second == group_pair.second[0]) {
                    shift_type = shift.getShiftType();
                    break;
                }
            }
            if (shift_type > 0) break;
        }
        
        if (shift_type > 0) {
            group_info_map[group_id] = make_pair(load_group, shift_type);
        }
    }
    
    // 按轮转顺序排序：白班航班派工（08:00后）：按 2组-3组-4组-5组-6组-7组-8组-1组循环指派
    // 首先按班次类型分组，然后按load_group排序，但需要调整为2-3-4-5-6-7-8-1的顺序
    for (int shift_type = 1; shift_type <= 2; ++shift_type) {  // 1=主班, 2=副班
        vector<pair<int, int>> temp_groups;  // (group_id, load_group)
        for (const auto& info_pair : group_info_map) {
            if (info_pair.second.second == shift_type) {
                temp_groups.push_back({info_pair.first, info_pair.second.first});
            }
        }
        // 按load_group排序
        sort(temp_groups.begin(), temp_groups.end(), 
             [](const pair<int, int>& a, const pair<int, int>& b) {
                 return a.second < b.second;
             });
        
        // 调整为2-3-4-5-6-7-8-1的顺序（如果load_group在1-8范围内）
        // 将load_group=1的组移到末尾
        vector<pair<int, int>> reordered_groups;
        for (const auto& tg : temp_groups) {
            if (tg.second == 1) {
                // load_group=1的组放到末尾
                continue;
            }
            reordered_groups.push_back(tg);
        }
        // 将load_group=1的组添加到末尾
        for (const auto& tg : temp_groups) {
            if (tg.second == 1) {
                reordered_groups.push_back(tg);
            }
        }
        
        // 添加到轮转顺序
        for (const auto& tg : reordered_groups) {
            rotation_order.push_back(tg.first);
        }
    }
    
    // 如果没有找到轮转顺序，按组ID排序
    if (rotation_order.empty()) {
        for (const auto& group_pair : groups) {
            rotation_order.push_back(group_pair.first);
        }
        sort(rotation_order.begin(), rotation_order.end());
    }
    
    int current_rotation_index = 0;  // 当前轮转索引
    
    // 遍历任务列表，逐个分配任务
    for (auto& task : tasks) {
        long task_id = task.getTaskId();
        
        // 跳过已经处理过的任务
        if (processed_task_ids.find(task_id) != processed_task_ids.end()) {
            continue;
        }
        
        // 跳过已经分配的任务
        if (task.isAssigned() && task.getAssignedEmployeeCount() > 0) {
            processed_task_ids.insert(task_id);
            continue;
        }
        
        int assigned_count = static_cast<int>(task.getAssignedEmployeeCount());
        int required_count = task.getRequiredCount();
        
        long task_start = task.getStartTime();
        long task_end = task.getEndTime();
        
        // 获取当前任务的机位信息
        int task_stand = 0;
        auto task_flight_it = flight_task_map.find(task_id);
        if (task_flight_it != flight_task_map.end() && task_flight_it->second < flights.size()) {
            task_stand = flights[task_flight_it->second].getStand();
        }
        
        // 判断是否是早出港任务（08:00前）
        const long EIGHT_AM_SECONDS = 8 * 3600;  // 08:00 = 28800秒（从当天0点开始）
        long task_day = task_start / (24 * 3600);
        long task_time_in_day = task_start % (24 * 3600);
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
                        for (const string& emp_id : prev_assigned) {
                            if (employee_map.find(emp_id) == employee_map.end() ||
                                task.isAssignedToEmployee(emp_id) ||
                                !isEmployeeAvailable(emp_id, task_start, task_end, task_ptr_map, employee_map)) {
                                can_reuse = false;
                                break;
                            }
                        }
                    }
                    
                    if (can_reuse) {
                        // 重用上一次的分配（整组重用）
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
                
                // 检查组对应的班次是否在任务时间段被占位（疲劳度控制）
                bool shift_blocked = false;
                for (const string& emp_id : group_members) {
                    // 查找员工所在的班次
                    for (const auto& shift : shifts) {
                        const auto& position_map = shift.getPositionToEmployeeId();
                        for (const auto& pos_pair : position_map) {
                            if (pos_pair.second == emp_id) {
                                int shift_type = shift.getShiftType();
                                if (isShiftBlocked(shift_type, task_start, block_periods)) {
                                    shift_blocked = true;
                                    break;
                                }
                            }
                        }
                        if (shift_blocked) break;
                    }
                    if (shift_blocked) break;
                }
                if (shift_blocked) {
                    continue;  // 班次被占位，跳过
                }
                
                // 检查组资质是否匹配（硬约束：主要看小组资质是否匹配）
                // 这里简化处理：检查组内所有成员是否都有任务要求的资质
                // 实际应该检查组整体资质，但这里假设组内成员资质相同
                bool qualification_match = true;
                int required_qualification = task.getRequiredQualification();
                if (required_qualification > 0) {
                    for (const string& emp_id : group_members) {
                        auto emp_it = employee_map.find(emp_id);
                        if (emp_it == employee_map.end()) {
                            qualification_match = false;
                            break;
                        }
                        const LoadEmployeeInfo* emp = emp_it->second;
                        int emp_qualification = emp->getQualificationMask();
                        // 检查员工是否具有任务要求的所有资质
                        if ((emp_qualification & required_qualification) != required_qualification) {
                            qualification_match = false;
                            break;
                        }
                    }
                }
                if (!qualification_match) {
                    continue;  // 资质不匹配，跳过
                }
                
                // 检查组内所有成员在任务时间段是否都空闲
                bool all_available = true;
                for (const string& emp_id : group_members) {
                    // 如果已经分配给当前任务，跳过
                    if (task.isAssignedToEmployee(emp_id)) {
                        continue;
                    }
                    
                    if (!isEmployeeAvailable(emp_id, task_start, task_end, task_ptr_map, employee_map)) {
                        all_available = false;
                        break;
                    }
                }
                
                if (!all_available) {
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
                        for (long assigned_task_id : assigned_task_ids) {
                            auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                            if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                continue;
                            }
                            
                            const TaskDefinition& assigned_task = *(assigned_task_it->second);
                            if (assigned_task.getEndTime() < task_start && 
                                assigned_task.getEndTime() > last_end_time) {
                                last_end_time = assigned_task.getEndTime();
                                // 获取该任务的机位
                                auto last_task_flight_it = flight_task_map.find(assigned_task_id);
                                if (last_task_flight_it != flight_task_map.end() && 
                                    last_task_flight_it->second < flights.size()) {
                                    last_stand = flights[last_task_flight_it->second].getStand();
                                }
                            }
                        }
                    }
                    
                    // 如果找到上次任务，验证是否有足够时间到达当前任务
                    // 放宽条件：允许有5分钟的缓冲时间（300秒）
                    if (last_stand > 0 && last_end_time > 0) {
                        long travel_time = StandDistance::getInstance().getTravelTime(last_stand, task_stand);
                        const long BUFFER_TIME = 5 * 60;  // 5分钟缓冲
                        if ((last_end_time + travel_time + BUFFER_TIME) > task_start) {
                            // 无法按时到达，跳过该组
                            continue;
                        }
                    }
                }
                
                // 组可用且能按时到达
                available_groups.push_back({group_id, group_members});
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
                    
                    // 检查组对应的班次是否在任务时间段被占位（疲劳度控制）
                    bool shift_blocked = false;
                    for (const string& emp_id : group_members) {
                        for (const auto& shift : shifts) {
                            const auto& position_map = shift.getPositionToEmployeeId();
                            for (const auto& pos_pair : position_map) {
                                if (pos_pair.second == emp_id) {
                                    int shift_type = shift.getShiftType();
                                    if (isShiftBlocked(shift_type, task_start, block_periods)) {
                                        shift_blocked = true;
                                        break;
                                    }
                                }
                            }
                            if (shift_blocked) break;
                        }
                        if (shift_blocked) break;
                    }
                    if (shift_blocked) {
                        continue;  // 班次被占位，跳过
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
                        
                        for (long assigned_task_id : assigned_task_ids) {
                            auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                            if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                continue;
                            }
                            
                            const TaskDefinition& assigned_task = *(assigned_task_it->second);
                            if (assigned_task.getEndTime() > group_last_end_time) {
                                group_last_end_time = assigned_task.getEndTime();
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
                            
                            for (long assigned_task_id : assigned_task_ids) {
                                auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                                if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                    continue;
                                }
                                const TaskDefinition& assigned_task = *(assigned_task_it->second);
                                if (assigned_task.getEndTime() < task_start && 
                                    assigned_task.getEndTime() > last_end_time) {
                                    last_end_time = assigned_task.getEndTime();
                                    auto last_flight_it = flight_task_map.find(assigned_task_id);
                                    if (last_flight_it != flight_task_map.end() && 
                                        last_flight_it->second < flights.size()) {
                                        last_stand = flights[last_flight_it->second].getStand();
                                    }
                                }
                            }
                        }
                        
                        if (last_stand > 0) {
                            // 计算机位距离（绝对值）
                            int stand_distance = abs(last_stand - task_stand);
                            if (stand_distance < min_stand_distance) {
                                min_stand_distance = stand_distance;
                                best_group_id = group_id;
                                min_group_task_time = calculateGroupDailyTaskTime(group_members, task_start, task_ptr_map, employee_map);
                            } else if (stand_distance == min_stand_distance) {
                                // 如果距离相同，选择当日工时较少的组（均衡疲劳度）
                                long group_task_time = calculateGroupDailyTaskTime(group_members, task_start, task_ptr_map, employee_map);
                                if (group_task_time < min_group_task_time) {
                                    min_group_task_time = group_task_time;
                                    best_group_id = group_id;
                                }
                            }
                        } else {
                            // 如果该组没有上一个任务，也考虑（选择当日工时较少的组）
                            long group_task_time = calculateGroupDailyTaskTime(group_members, task_start, task_ptr_map, employee_map);
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
                    // 按轮转顺序查找可用组（从当前轮转索引开始）
                    for (size_t offset = 0; offset < rotation_order.size(); ++offset) {
                        size_t idx = (current_rotation_index + offset) % rotation_order.size();
                        int candidate_group_id = rotation_order[idx];
                        
                        auto it = available_groups_map.find(candidate_group_id);
                        if (it != available_groups_map.end()) {
                            // 找到第一个在轮转顺序中且可用的组
                            selected_group_id = candidate_group_id;
                            selected_group_members = it->second;
                            found_by_rotation = true;
                            // 更新轮转索引到下一个
                            current_rotation_index = (idx + 1) % rotation_order.size();
                            break;
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
                    
                    // 优先级1：轮转顺序
                    int rotation_position = INT_MAX;
                    for (size_t i = 0; i < rotation_order.size(); ++i) {
                        if (rotation_order[i] == group_id) {
                            int distance = static_cast<int>(i) - current_rotation_index;
                            if (distance < 0) {
                                distance += static_cast<int>(rotation_order.size());
                            }
                            rotation_position = distance;
                            break;
                        }
                    }
                    if (rotation_position == INT_MAX) {
                        rotation_position = 10000;
                    }
                
                    // 优先级2：临近下班小组任务指派（尽量为小组分配不延误下班时间的机位任务）
                    // 检查任务结束时间是否在下班时间之后（这里简化处理，假设下班时间为22:00）
                    const long DEFAULT_OFF_DUTY_TIME = 22 * 3600;  // 22:00 = 79200秒
                    long task_day = task_start / (24 * 3600);
                    long off_duty_time = task_day * (24 * 3600) + DEFAULT_OFF_DUTY_TIME;
                    bool task_delays_off_duty = (task_end > off_duty_time);
                    
                    // 如果任务会延误下班，优先选择当日工时较少的组（这些组可能更早下班）
                    long group_daily_task_time = calculateGroupDailyTaskTime(group_pair.second, task_start, task_ptr_map, employee_map);
                    
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
                            
                            for (long assigned_task_id : assigned_task_ids) {
                                auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                                if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                    continue;
                                }
                                
                                const TaskDefinition& assigned_task = *(assigned_task_it->second);
                                // 只考虑在当前任务开始之前的任务
                                if (assigned_task.getEndTime() < task_start) {
                                    // 获取该任务的机位
                                    int assigned_stand = 0;
                                    auto assigned_flight_it = flight_task_map.find(assigned_task_id);
                                    if (assigned_flight_it != flight_task_map.end() && 
                                        assigned_flight_it->second < flights.size()) {
                                        assigned_stand = flights[assigned_flight_it->second].getStand();
                                    }
                                    
                                    prev_tasks.push_back({assigned_task.getStartTime(), 
                                                          assigned_task.getEndTime(), 
                                                          assigned_stand});
                                }
                            }
                        }
                        
                        // 如果有上一个任务，计算连续工作时长
                        if (!prev_tasks.empty()) {
                            // 按开始时间排序
                            sort(prev_tasks.begin(), prev_tasks.end());
                            
                            // 从最近的任务开始，向前查找连续的任务链
                            long current_end = task_end;
                            long current_start = task_start;
                            int current_stand = task_stand;
                            long chain_start = task_start;
                            
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
                            for (long assigned_task_id : assigned_task_ids) {
                                auto assigned_task_it = task_ptr_map.find(assigned_task_id);
                                if (assigned_task_it == task_ptr_map.end() || assigned_task_it->second == nullptr) {
                                    continue;
                                }
                                
                                const TaskDefinition& assigned_task = *(assigned_task_it->second);
                                if (assigned_task.getEndTime() < task_start && 
                                    assigned_task.getEndTime() > last_end_time) {
                                    last_end_time = assigned_task.getEndTime();
                                    // 获取该任务的机位
                                    auto last_task_flight_it = flight_task_map.find(assigned_task_id);
                                    if (last_task_flight_it != flight_task_map.end() && 
                                        last_task_flight_it->second < flights.size()) {
                                        last_stand = flights[last_task_flight_it->second].getStand();
                                    }
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
                
                    // 如果通过综合得分选择了组，更新轮转索引
                    if (!found_by_rotation && selected_group_id >= 0) {
                        for (size_t i = 0; i < rotation_order.size(); ++i) {
                            if (rotation_order[i] == selected_group_id) {
                                current_rotation_index = (i + 1) % rotation_order.size();
                                break;
                            }
                        }
                    }
                }
            }
            
            if (selected_group_id < 0) {
                task.setShortStaffed(true);
                break;
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
                // 找到被分配组在轮转顺序中的位置，然后继续向后轮转
                for (size_t i = 0; i < rotation_order.size(); ++i) {
                    if (rotation_order[i] == selected_group_id) {
                        current_rotation_index = (i + 1) % rotation_order.size();
                        break;
                    }
                }
                // 如果没有在轮转顺序中找到（理论上不应该发生），则不需要更新轮转索引
            }
        }
        
        // 更新任务状态
        if (assigned_count > 0) {
            task.setAssigned(true);
        }
        
        // 标记为已处理
        processed_task_ids.insert(task_id);
        
        cout << "任务 ID " << task_id 
             << " (名称: " << task.getTaskName() 
             << ") 已分配 " << assigned_count 
             << " 人，需求 " << required_count << " 人。" << endl;
    }
    
    cout << "装卸任务调度完成！" << endl;
}

// 使用公共类的适配器函数实现
void LoadScheduler::scheduleLoadTasksFromCommon(
    const std::vector<AirportStaffScheduler::Staff>& common_staffs,
    const std::vector<AirportStaffScheduler::Task>& common_tasks,
    const std::vector<AirportStaffScheduler::Shift>& common_shifts,
    long default_travel_time,
    const vector<ShiftBlockPeriod>& block_periods)
{
    using namespace AirportStaffScheduler::Adapter;
    
    // 1. 转换Staff到LoadEmployeeInfo
    vector<LoadEmployeeInfo> employees;
    employees.reserve(common_staffs.size());
    for (const auto& staff : common_staffs) {
        LoadEmployeeInfo load_emp;
        EmployeeInfo emp_info = StaffToEmployeeInfo(staff);
        load_emp.getEmployeeInfo() = emp_info;
        // 从staff的teamName或其他属性中提取load_group（这里简化处理）
        // 实际使用时可能需要从其他字段获取组ID
        load_emp.setLoadGroup(0);  // 默认组，实际使用时需要从数据中提取
        employees.push_back(load_emp);
    }
    
    // 2. 转换Task到Flight（需要从Task中提取航班信息）
    // 注意：这里需要从Task的属性中提取航班信息，简化处理
    vector<Flight> flights;
    // 实际使用时需要根据Task的属性创建Flight对象
    // 这里只是示例，实际实现需要根据业务逻辑
    
    // 3. 转换Shift：将公共Shift列表按班次类型分组
    map<int, vip_first_class::Shift> shift_map;  // shift_type -> Shift对象
    
    for (const auto& common_shift : common_shifts) {
        const std::string& shift_name = common_shift.getShiftName();
        int shift_type = 0;  // 默认休息
        if (shift_name.find("主班") != string::npos) {
            shift_type = 1;  // 主班
        } else if (shift_name.find("副班") != string::npos) {
            shift_type = 2;  // 副班
        }
        
        if (shift_map.find(shift_type) == shift_map.end()) {
            vip_first_class::Shift vip_shift;
            vip_shift.setShiftType(shift_type);
            shift_map[shift_type] = vip_shift;
        }
        
        int position = shift_map[shift_type].getPositionToEmployeeId().size() + 1;
        shift_map[shift_type].setEmployeeIdAtPosition(position, common_shift.getStaffId());
    }
    
    vector<vip_first_class::Shift> shifts;
    for (auto& pair : shift_map) {
        shifts.push_back(pair.second);
    }
    
    // 4. 转换Task到TaskDefinition（如果需要从Task生成装卸任务）
    vector<vip_first_class::TaskDefinition> tasks;
    tasks.reserve(common_tasks.size());
    for (const auto& common_task : common_tasks) {
        vip_first_class::TaskDefinition task_def = TaskToTaskDefinition(common_task, vip_first_class::TaskType::DISPATCH);
        tasks.push_back(task_def);
    }
    
    // 5. 调用原有的调度函数
    // 注意：这里假设flights已经正确填充，实际使用时需要从common_tasks中提取航班信息
    scheduleLoadTasks(employees, flights, shifts, tasks, default_travel_time, block_periods, nullptr);
}

}  // namespace zhuangxie_class

