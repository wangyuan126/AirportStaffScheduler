/**
 * @file test_task_scheduler.cpp
 * @brief 任务调度器测试程序
 * 
 * 根据 Task.txt 生成任务并进行调度，将结果导出为 CSV 文件
 */

#include "task_scheduler.h"
#include "task_config.h"
#include "employee_manager.h"
#include "employee_info.h"
#include "shift.h"
#include "task_definition.h"
#include "task_type.h"
#include "../CSVDataLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <algorithm>

using namespace vip_first_class;

// 辅助函数：将时间字符串（如"08:30"）转换为从2020-01-01 00:00:00开始的秒数
static int64_t parseTimeString(const std::string& time_str) {
    if (time_str.find("航后") != std::string::npos) {
        return -1;  // 航后任务
    }
    
    // 解析HH:MM格式
    size_t colon_pos = time_str.find(':');
    if (colon_pos == std::string::npos) {
        return 0;
    }
    
    int hours = std::stoi(time_str.substr(0, colon_pos));
    int minutes = std::stoi(time_str.substr(colon_pos + 1));
    
    // 转换为秒数（从2020-01-01 00:00:00开始）
    return hours * 3600 + minutes * 60;
}

// 辅助函数：将任务名称映射到TaskType
static TaskType parseTaskType(const std::string& task_name) {
    if (task_name == "调度") {
        return TaskType::DISPATCH;
    } else if (task_name == "国内前台") {
        return TaskType::DOMESTIC_FRONT_DESK;
    } else if (task_name == "国内前台协助") {
        return TaskType::DOMESTIC_FRONT_DESK_ASSIST;
    } else if (task_name == "国内前台协助2") {
        return TaskType::DOMESTIC_FRONT_DESK_ASSIST2;
    } else if (task_name == "国内前台早班") {
        return TaskType::DOMESTIC_FRONT_DESK_EARLY;
    } else if (task_name == "国际前台早班") {
        return TaskType::INTERNATIONAL_FRONT_DESK_EARLY;
    } else if (task_name == "国际前台晚班") {
        return TaskType::INTERNATIONAL_FRONT_DESK_LATE;
    } else if (task_name == "国际厅内早班") {
        return TaskType::INTERNATIONAL_HALL_EARLY;
    } else if (task_name == "国际厅内晚班") {
        return TaskType::INTERNATIONAL_HALL_LATE;
    } else if (task_name == "国内厅内早班") {
        return TaskType::DOMESTIC_HALL_EARLY;
    } else if (task_name == "国内厅内08:30-09:30") {
        return TaskType::DOMESTIC_HALL_0830_0930;
    } else if (task_name == "国内厅内09:30-10:30") {
        return TaskType::DOMESTIC_HALL_0930_1030;
    } else if (task_name == "国内厅内10:30-11:30") {
        return TaskType::DOMESTIC_HALL_1030_1130;
    } else if (task_name == "国内厅内11:30-12:30") {
        return TaskType::DOMESTIC_HALL_1130_1230;
    } else if (task_name == "国内厅内12:30-13:30") {
        return TaskType::DOMESTIC_HALL_1230_1330;
    } else if (task_name == "国内厅内13:30-14:30") {
        return TaskType::DOMESTIC_HALL_1330_1430;
    } else if (task_name == "国内厅内14:30-15:30") {
        return TaskType::DOMESTIC_HALL_1430_1530;
    } else if (task_name == "国内厅内15:30-16:30") {
        return TaskType::DOMESTIC_HALL_1530_1630;
    } else if (task_name == "国内厅内16:30-17:30") {
        return TaskType::DOMESTIC_HALL_1630_1730;
    } else if (task_name == "国内厅内17:30-18:30") {
        return TaskType::DOMESTIC_HALL_1730_1830;
    } else if (task_name == "国内厅内18:30-19:30") {
        return TaskType::DOMESTIC_HALL_1830_1930;
    } else if (task_name == "国内厅内19:30-20:30") {
        return TaskType::DOMESTIC_HALL_1930_2030;
    } else if (task_name == "国内厅内20:30-航后") {
        return TaskType::DOMESTIC_HALL_2030_AFTER;
    } else if (task_name == "外场（国内出港-少人）") {
        return TaskType::EXTERNAL_DOMESTIC_DEPARTURE_FEW;
    } else if (task_name == "外场（国内出港-多人）") {
        return TaskType::EXTERNAL_DOMESTIC_DEPARTURE_MANY;
    } else if (task_name == "外场（国内进港-少人）") {
        return TaskType::EXTERNAL_DOMESTIC_ARRIVAL_FEW;
    } else if (task_name == "外场（国内进港-多人）") {
        return TaskType::EXTERNAL_DOMESTIC_ARRIVAL_MANY;
    } else if (task_name == "外场（国际出港-少人）") {
        return TaskType::EXTERNAL_INTERNATIONAL_DEPARTURE_FEW;
    } else if (task_name == "外场（国际出港-多人）") {
        return TaskType::EXTERNAL_INTERNATIONAL_DEPARTURE_MANY;
    } else if (task_name == "外场（国际进港-少人）") {
        return TaskType::EXTERNAL_INTERNATIONAL_ARRIVAL_FEW;
    } else if (task_name == "外场（国际进港-多人）") {
        return TaskType::EXTERNAL_INTERNATIONAL_ARRIVAL_MANY;
    }
    
    return TaskType::DISPATCH;  // 默认
}

// 辅助函数：将秒数转换为时间字符串（用于CSV输出）
static std::string formatTime(int64_t seconds) {
    if (seconds < 0) {
        return "航后";
    }
    
    int64_t hours = seconds / 3600;
    int64_t minutes = (seconds % 3600) / 60;
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours 
        << ":" << std::setw(2) << minutes;
    return oss.str();
}

// 辅助函数：将秒数和任务日期转换为日期时间字符串（YYYY-MM-DD HH:MM:SS格式）
// 基准日期：2020-01-01 00:00:00
static std::string formatDateTime(int64_t seconds, const std::string& task_date = "") {
    if (seconds <= 0) {
        return "";
    }
    
    // 如果提供了任务日期，使用任务日期作为日期部分
    if (!task_date.empty()) {
        // 从任务日期中提取日期部分（YYYY-MM-DD）
        std::string date_part = task_date;
        // 移除可能的引号
        if (date_part.front() == '"' && date_part.back() == '"') {
            date_part = date_part.substr(1, date_part.length() - 2);
        }
        
        // 计算时间部分（从当天00:00:00开始的秒数）
        int64_t days = seconds / 86400;
        int64_t remaining_seconds = seconds % 86400;
        
        // 如果秒数超过一天，需要调整日期
        // 这里简化处理：假设任务日期就是正确的日期
        int hours = remaining_seconds / 3600;
        int minutes = (remaining_seconds % 3600) / 60;
        int secs = remaining_seconds % 60;
        
        std::ostringstream oss;
        oss << date_part << " "
            << std::setfill('0') << std::setw(2) << hours << ":"
            << std::setfill('0') << std::setw(2) << minutes << ":"
            << std::setfill('0') << std::setw(2) << secs;
        return oss.str();
    }
    
    // 如果没有提供任务日期，使用基准日期计算
    int64_t days = seconds / 86400;
    int64_t remaining_seconds = seconds % 86400;
    
    // 使用基准日期2020-01-01
    int year = 2020;
    int month = 1;
    int day = 1;
    
    // 累加天数（简化处理，每月30天）
    day += static_cast<int>(days);
    while (day > 30) {
        day -= 30;
        month++;
        if (month > 12) {
            month = 1;
            year++;
        }
    }
    
    int hours = static_cast<int>(remaining_seconds / 3600);
    int minutes = static_cast<int>((remaining_seconds % 3600) / 60);
    int secs = static_cast<int>(remaining_seconds % 60);
    
    std::ostringstream oss;
    oss << year << "-" 
        << std::setfill('0') << std::setw(2) << month << "-"
        << std::setfill('0') << std::setw(2) << day << " "
        << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << secs;
    return oss.str();
}

// 辅助函数：导出任务分配结果到CSV文件
static void exportToCSV(const std::vector<TaskDefinition>& tasks, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误：无法创建CSV文件 " << filename << std::endl;
        return;
    }
    // 写入CSV表头
    file << "任务ID,任务名称,任务类型,开始时间,结束时间,需要人数,已分配人数,是否已分配,是否缺人,分配的员工ID\n";
    
    // 写入每个任务的详细信息
    for (const auto& task : tasks) {
        file << task.getTaskId() << ","
             << task.getTaskName() << ","
             << static_cast<int>(task.getTaskType()) << ","
             << formatTime(task.getStartTime()) << ","
             << formatTime(task.getEndTime()) << ","
             << task.getRequiredCount() << ","
             << task.getAssignedEmployeeCount() << ","
             << (task.isAssigned() ? "是" : "否") << ","
             << (task.isShortStaffed() ? "是" : "否") << ",";
        
        // 写入分配的员工ID列表
        const auto& assigned_ids = task.getAssignedEmployeeIds();
        for (size_t i = 0; i < assigned_ids.size(); ++i) {
            if (i > 0) {
                file << ";";
            }
            file << assigned_ids[i];
        }
        file << "\n";
    }
    
    file.close();
    std::cout << "任务分配结果已导出到: " << filename << std::endl;
}

// 辅助结构：员工任务时间段
struct EmployeeTaskSlot {
    std::string task_id;
    std::string task_name;
    int64_t start_time;
    int64_t end_time;
    const TaskDefinition* task_ptr;  // 指向任务的指针，用于获取更多信息
    
    EmployeeTaskSlot(const std::string& id, const std::string& name, int64_t start, int64_t end, const TaskDefinition* task = nullptr)
        : task_id(id), task_name(name), start_time(start), end_time(end), task_ptr(task) {}
};

// 辅助函数：导出员工时间表（按照soln_shift.csv格式）到CSV文件
static void exportEmployeeScheduleToCSV(const std::vector<TaskDefinition>& tasks, 
                                         const std::vector<Shift>& shifts,
                                         const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误：无法创建CSV文件 " << filename << std::endl;
        return;
    }
    
    // 写入CSV表头（按照soln_shift.csv格式）
    file << "班期日期,班期开始时间,班期结束时间,人员编号,人员姓名,车辆（车牌号）,车辆类型,任务ID,任务名称,任务日期,任务开始时间,任务结束时间,到达航班ID,出发航班ID,到达航班号,出发航班号,航站楼,区域,机位,其他位置,双机航班号,是否加班\n";
    
    // 收集每个员工的任务时间段
    std::map<std::string, std::vector<EmployeeTaskSlot>> employee_schedule;
    
    // 遍历所有任务，收集员工的任务分配
    for (const auto& task : tasks) {
        const auto& assigned_ids = task.getAssignedEmployeeIds();
        if (assigned_ids.empty()) {
            continue;  // 跳过未分配的任务
        }
        for (const auto& employee_id : assigned_ids) {
            // 使用实际开始时间和结束时间
            int64_t start_time = task.getActualStartTime() > 0 ? task.getActualStartTime() : task.getStartTime();
            int64_t end_time = task.getActualEndTime();
            
            // 如果实际结束时间为0，尝试使用开始时间+时长
            if (end_time <= 0 && start_time > 0) {
                end_time = start_time + task.getDuration();
            }
            
            // 如果还是0，使用原始结束时间
            if (end_time <= 0) {
                end_time = task.getEndTime();
            }
            
            // 如果结束时间仍然无效，跳过这个任务
            if (end_time <= 0 && start_time <= 0) {
                continue;
            }
            
            employee_schedule[employee_id].push_back(
                EmployeeTaskSlot(task.getTaskId(), task.getTaskName(), 
                                start_time, end_time, &task)
            );
        }
    }
    
    // 对每个员工的任务按开始时间排序
    for (auto& emp_pair : employee_schedule) {
        std::sort(emp_pair.second.begin(), emp_pair.second.end(),
                  [](const EmployeeTaskSlot& a, const EmployeeTaskSlot& b) {
                      return a.start_time < b.start_time;
                  });
    }
    
    // 获取所有员工的ID（从EmployeeManager）
    std::vector<std::string> all_employee_ids;
    // 遍历所有shifts，收集员工ID
    for (const auto& shift : shifts) {
        const auto& position_map = shift.getPositionToEmployeeId();
        for (const auto& pos_pair : position_map) {
            const std::string& emp_id = pos_pair.second;
            if (std::find(all_employee_ids.begin(), all_employee_ids.end(), emp_id) == all_employee_ids.end()) {
                all_employee_ids.push_back(emp_id);
            }
        }
    }
    
    // 写入每个员工的时间表
    for (const auto& employee_id : all_employee_ids) {
        auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
        if (!employee) {
            continue;
        }
        
        // 获取该员工的任务列表
        auto schedule_it = employee_schedule.find(employee_id);
        
        // 如果没有任务，跳过（根据要求：如果是休息就不输出）
        if (schedule_it == employee_schedule.end() || schedule_it->second.empty()) {
            continue;
        }
        
        // 获取班期日期（从第一个任务的日期推断）
        std::string shift_date = "";
        if (!schedule_it->second.empty() && schedule_it->second[0].task_ptr) {
            shift_date = schedule_it->second[0].task_ptr->getTaskDate();
        }
        
        // 获取班期开始时间（第一个任务的开始时间）
        int64_t shift_start_time = schedule_it->second[0].start_time;
        std::string shift_start_str = formatDateTime(shift_start_time, shift_date);
        
        // 获取班期结束时间（最晚的任务的结束时间）
        int64_t shift_end_time = 0;
        for (const auto& task_slot : schedule_it->second) {
            if (task_slot.end_time > shift_end_time) {
                shift_end_time = task_slot.end_time;
            }
        }
        std::string shift_end_str = formatDateTime(shift_end_time, shift_date);
        
        // 输出每个任务
        for (const auto& task_slot : schedule_it->second) {
            if (!task_slot.task_ptr) {
                continue;
            }
            
            const TaskDefinition& task = *(task_slot.task_ptr);
            
            // 班期日期
            file << "\"" << shift_date << "\",";
            
            // 班期开始时间
            file << "\"" << shift_start_str << "\",";
            
            // 班期结束时间
            file << "\"" << shift_end_str << "\",";
            
            // 人员编号
            file << "\"" << employee_id << "\",";
            
            // 人员姓名
            file << "\"" << employee->getEmployeeName() << "\",";
            
            // 车辆（车牌号）- 不输出
            file << "\"\",";
            
            // 车辆类型 - 不输出
            file << "\"\",";
            
            // 任务ID
            file << "\"" << task.getTaskId() << "\",";
            
            // 任务名称
            file << "\"" << task.getTaskName() << "\",";
            
            // 任务日期
            file << "\"" << task.getTaskDate() << "\",";
            
            // 任务开始时间（实际开始时间）
            std::string task_start_str = formatDateTime(task_slot.start_time, task.getTaskDate());
            file << "\"" << task_start_str << "\",";
            
            // 任务结束时间（实际开始时间 + 时长）
            std::string task_end_str = formatDateTime(task_slot.end_time, task.getTaskDate());
            file << "\"" << task_end_str << "\",";
            
            // 到达航班ID
            std::string arrival_flight_id = task.getArrivalFlightId();
            if (arrival_flight_id.empty()) {
                file << "\"\",";
            } else {
                file << "\"" << arrival_flight_id << "\",";
            }
            
            // 出发航班ID
            std::string departure_flight_id = task.getDepartureFlightId();
            if (departure_flight_id.empty()) {
                file << "\"\",";
            } else {
                file << "\"" << departure_flight_id << "\",";
            }
            
            // 到达航班号
            std::string arrival_flight_number = task.getArrivalFlightNumber();
            if (arrival_flight_number.empty()) {
                file << "\"\",";
            } else {
                file << "\"" << arrival_flight_number << "\",";
            }
            
            // 出发航班号
            std::string departure_flight_number = task.getDepartureFlightNumber();
            if (departure_flight_number.empty()) {
                file << "\"\",";
            } else {
                file << "\"" << departure_flight_number << "\",";
            }
            
            // 航站楼
            std::string terminal = task.getTerminal();
            if (terminal.empty()) {
                file << "\"\",";
            } else {
                file << "\"" << terminal << "\",";
            }
            
            // 区域 - 不输出
            file << "\"\",";
            
            // 机位
            int stand = task.getStand();
            if (stand == 0) {
                file << "\"\",";
            } else {
                file << "\"" << stand << "\",";
            }
            
            // 其他位置 - 不输出
            file << "\"\",";
            
            // 双机航班号 - 不输出
            file << "\"\",";
            
            // 是否加班 - 全部都是"否"
            file << "\"否\"\n";
        }
    }
    
    file.close();
    std::cout << "员工时间表已导出到: " << filename << std::endl;
}

// 辅助函数：计算时间在时间轴上的位置（从05:00开始，每30分钟一个单位）
static int getTimePosition(int64_t seconds) {
    if (seconds < 0) {
        return 36;  // 航后任务位置（22:30之后）
    }
    
    const int64_t START_TIME = 5 * 3600;  // 05:00 = 18000秒
    const int64_t MINUTES_30 = 30 * 60;   // 30分钟 = 1800秒
    
    int64_t diff = seconds - START_TIME;
    if (diff < 0) {
        return 0;
    }
    
    return static_cast<int>(diff / MINUTES_30);
}

// 辅助函数：生成甘特图样式的文本时间表
static void exportGanttChartText(const std::vector<TaskDefinition>& tasks,
                                  const std::vector<Shift>& shifts,
                                  const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误：无法创建文件 " << filename << std::endl;
        return;
    }
    
    // 收集每个员工的任务时间段
    std::map<std::string, std::vector<EmployeeTaskSlot>> employee_schedule;
    
    for (const auto& task : tasks) {
        const auto& assigned_ids = task.getAssignedEmployeeIds();
        for (const auto& employee_id : assigned_ids) {
            employee_schedule[employee_id].push_back(
                EmployeeTaskSlot(task.getTaskId(), task.getTaskName(), 
                                task.getStartTime(), task.getEndTime())
            );
        }
    }
    
    // 对每个员工的任务按开始时间排序
    for (auto& emp_pair : employee_schedule) {
        std::sort(emp_pair.second.begin(), emp_pair.second.end(),
                  [](const EmployeeTaskSlot& a, const EmployeeTaskSlot& b) {
                      return a.start_time < b.start_time;
                  });
    }
    
    // 获取所有员工ID并排序（只显示有任务的员工）
    std::vector<std::string> all_employee_ids;
    
    // 先添加主班员工
    for (int i = 1; i <= 8; ++i) {
        std::string emp_id = "main" + std::to_string(i);
        if (EmployeeManager::getInstance().hasEmployee(emp_id) &&
            employee_schedule.find(emp_id) != employee_schedule.end() &&
            !employee_schedule[emp_id].empty()) {
            all_employee_ids.push_back(emp_id);
        }
    }
    
    // 再添加副班员工
    for (int i = 1; i <= 8; ++i) {
        std::string emp_id = "sub" + std::to_string(i);
        if (EmployeeManager::getInstance().hasEmployee(emp_id) &&
            employee_schedule.find(emp_id) != employee_schedule.end() &&
            !employee_schedule[emp_id].empty()) {
            all_employee_ids.push_back(emp_id);
        }
    }
    
    file << "员工时间表（甘特图格式）\n";
    file << std::string(120, '=') << "\n\n";
    
    // 生成时间轴标题（每2小时一个标记）
    file << std::setw(15) << "员工";
    for (int hour = 5; hour <= 22; hour += 2) {
        file << std::setw(12) << "";
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << hour << ":00";
        file << oss.str();
    }
    file << "\n";
    
    file << std::string(120, '-') << "\n";
    
    // 为每个员工生成时间线
    for (const auto& employee_id : all_employee_ids) {
        auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
        if (!employee) {
            continue;
        }
        
        // 确定班次类型
        std::string shift_type_str = "未知";
        if (employee_id.find("main") == 0) {
            shift_type_str = "主班";
        } else if (employee_id.find("sub") == 0) {
            shift_type_str = "副班";
        }
        
        // 输出员工信息（限制长度）
        std::string emp_display = employee->getEmployeeName() + "(" + employee_id + ")";
        if (emp_display.length() > 14) {
            emp_display = emp_display.substr(0, 11) + "...";
        }
        file << std::setw(15) << emp_display;
        
        // 创建时间线数组（从05:00到22:30，每30分钟一个单位，共36个单位）
        const int TIME_SLOTS = 36;
        std::vector<std::string> timeline(TIME_SLOTS, "  ");  // 每个时间段用2个字符表示
        
        auto schedule_it = employee_schedule.find(employee_id);
        if (schedule_it != employee_schedule.end()) {
            for (const auto& task_slot : schedule_it->second) {
                int start_pos = getTimePosition(task_slot.start_time);
                int end_pos;
                
                if (task_slot.end_time < 0) {
                    end_pos = TIME_SLOTS;  // 航后任务延伸到时间轴末尾
                } else {
                    end_pos = getTimePosition(task_slot.end_time);
                    if (end_pos >= TIME_SLOTS) {
                        end_pos = TIME_SLOTS - 1;
                    }
                }
                
                // 在时间线上标记任务（使用任务ID的最后一个字符作为标识）
                char task_char = task_slot.task_id.empty() ? '?' : task_slot.task_id.back();
                for (int pos = start_pos; pos < end_pos && pos < TIME_SLOTS; ++pos) {
                    if (timeline[pos] == "  ") {
                        timeline[pos] = std::string(1, task_char) + " ";
                    } else {
                        timeline[pos] = "**";  // 重叠任务
                    }
                }
            }
        }
        
        // 输出时间线
        for (int i = 0; i < TIME_SLOTS; ++i) {
            file << timeline[i];
        }
        file << "\n";
    }
    
    file << std::string(120, '-') << "\n\n";
    file << "图例说明：\n";
    file << "- 数字表示任务ID的最后一位\n";
    file << "- ** 表示任务重叠\n";
    file << "- 空白表示空闲时间\n\n";
    
    // 输出详细任务列表
    file << "详细任务列表：\n";
    file << std::string(120, '-') << "\n";
    
    for (const auto& employee_id : all_employee_ids) {
        auto* employee = EmployeeManager::getInstance().getEmployee(employee_id);
        if (!employee) {
            continue;
        }
        
        auto schedule_it = employee_schedule.find(employee_id);
        if (schedule_it != employee_schedule.end() && !schedule_it->second.empty()) {
            file << employee->getEmployeeName() << " (" << employee_id << "):\n";
            for (const auto& task_slot : schedule_it->second) {
                file << "  [" << formatTime(task_slot.start_time) << "-" 
                     << formatTime(task_slot.end_time) << "] " 
                     << task_slot.task_name << " (任务ID: " << task_slot.task_id << ")\n";
            }
            file << "\n";
        }
    }
    
    file.close();
    std::cout << "甘特图文本时间表已导出到: " << filename << std::endl;
}

int main() {
    std::cout << "开始任务调度测试..." << std::endl;
    
    // 设置输入文件路径
    std::string input_dir = "../input/";
    std::string vip_shift_csv = input_dir + "vip_first_class_shift.csv";
    std::string vip_task_csv = input_dir + "vip_first_class_task.csv";
    
    // 1. 从CSV加载班次信息
    std::cout << "Step 1: Loading shifts from CSV..." << std::endl;
    std::cout << "CSV file path: " << vip_shift_csv << std::endl;
    
    // 检查文件是否存在
    std::ifstream test_file(vip_shift_csv);
    if (!test_file.is_open()) {
        std::cerr << "ERROR: Cannot open file: " << vip_shift_csv << std::endl;
        std::cerr << "Please check if the file exists and the path is correct." << std::endl;
        return 1;
    }
    test_file.close();
    
    std::vector<Shift> shifts;
    try {
        shifts = AirportStaffScheduler::CSVLoader::loadShiftsFromCSV(vip_shift_csv);
        if (shifts.empty()) {
            std::cerr << "WARNING: CSV file contains no valid shift data" << std::endl;
            std::cerr << "This might be due to encoding issues or empty file." << std::endl;
        } else {
            std::cout << "Successfully loaded " << shifts.size() << " shifts from CSV" << std::endl;
        }
        
        // 从班次信息中提取员工信息并创建EmployeeInfo对象
        for (const auto& shift : shifts) {
            const auto& position_map = shift.getPositionToEmployeeId();
            for (const auto& pos_pair : position_map) {
                const std::string& emp_id = pos_pair.second;
                // 如果员工不存在，创建员工信息
                if (!EmployeeManager::getInstance().hasEmployee(emp_id)) {
                    EmployeeInfo emp;
                    emp.setEmployeeId(emp_id);
                    // 从员工ID推断员工姓名（简化处理）
                    std::string emp_name = emp_id;
                    if (emp_id.find("main") == 0) {
                        emp_name = "主班" + emp_id.substr(4);
                    } else if (emp_id.find("sub") == 0) {
                        emp_name = "副班" + emp_id.substr(3);
                    } else if (emp_id.find("rest") == 0) {
                        emp_name = "休息" + emp_id.substr(4);
                    }
                    emp.setEmployeeName(emp_name);
                    emp.setQualificationMask(15);  // 所有资质（1+2+4+8=15）
                    EmployeeManager::getInstance().addOrUpdateEmployee(emp_id, emp);
                }
            }
        }
        std::cout << "Created/updated " << EmployeeManager::getInstance().getEmployeeCount() 
                  << " employees from shifts" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to load shifts: " << e.what() << std::endl;
        std::cerr << "Using empty shift list..." << std::endl;
    }
    
    // 2. 从CSV加载VIP任务
    std::cout << "Step 2: Loading VIP tasks from CSV..." << std::endl;
    std::cout << "CSV file path: " << vip_task_csv << std::endl;
    
    // 检查文件是否存在
    std::ifstream test_task_file(vip_task_csv);
    if (!test_task_file.is_open()) {
        std::cerr << "ERROR: Cannot open file: " << vip_task_csv << std::endl;
        std::cerr << "Please check if the file exists and the path is correct." << std::endl;
        return 1;
    }
    test_task_file.close();
    
    std::vector<TaskDefinition> tasks;
    try {
        bool success = AirportStaffScheduler::CSVLoader::loadVIPTasksFromCSV(vip_task_csv, tasks);
        if (!success || tasks.empty()) {
            std::cerr << "WARNING: CSV file contains no valid task data" << std::endl;
            std::cerr << "This might be due to encoding issues or empty file." << std::endl;
        } else {
            std::cout << "Successfully loaded " << tasks.size() << " tasks from CSV" << std::endl;
        }
        
        // 设置任务类型（根据任务名称推断）
        for (auto& task : tasks) {
            task.setTaskType(parseTaskType(task.getTaskName()));
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to load tasks: " << e.what() << std::endl;
        std::cerr << "Using empty task list..." << std::endl;
    }
    
    // 检查是否有数据
    if (shifts.empty()) {
        std::cerr << "ERROR: No shifts loaded. Cannot proceed with scheduling." << std::endl;
        return 1;
    }
    if (tasks.empty()) {
        std::cerr << "ERROR: No tasks loaded. Cannot proceed with scheduling." << std::endl;
        return 1;
    }
    if (EmployeeManager::getInstance().getEmployeeCount() == 0) {
        std::cerr << "ERROR: No employees in manager. Cannot proceed with scheduling." << std::endl;
        return 1;
    }
    
    // 3. 初始化TaskConfig（如果还没有初始化）
    TaskConfig::getInstance().initializeTaskPriorities();
    
    // 4. 调用任务调度
    std::cout << "Step 3: Starting task scheduling..." << std::endl;
    std::cout << "Total tasks before scheduling: " << tasks.size() << std::endl;
    std::cout << "Total shifts: " << shifts.size() << std::endl;
    std::cout << "Total employees in manager: " << EmployeeManager::getInstance().getEmployeeCount() << std::endl;
    
    TaskScheduler scheduler;
    scheduler.scheduleTasks(tasks, shifts);
    
    // 检查任务分配情况
    int assigned_count = 0;
    int total_required = 0;
    int total_assigned = 0;
    for (const auto& task : tasks) {
        int assigned = task.getAssignedEmployeeCount();
        int required = task.getRequiredCount();
        total_required += required;
        total_assigned += assigned;
        if (assigned > 0) {
            assigned_count++;
        }
    }
    std::cout << "Scheduling completed:" << std::endl;
    std::cout << "  Tasks with assignments: " << assigned_count << " / " << tasks.size() << std::endl;
    std::cout << "  Total required staff: " << total_required << std::endl;
    std::cout << "  Total assigned staff: " << total_assigned << std::endl;
    
    // 5. 导出结果到CSV
    std::cout << "Step 4: Exporting results to CSV file..." << std::endl;
    exportToCSV(tasks, "task_assignment_result.csv");
    
    // 7. 导出员工时间表（按照soln_shift.csv格式）
    exportEmployeeScheduleToCSV(tasks, shifts, "result.csv");
    exportGanttChartText(tasks, shifts, "employee_schedule_gantt.txt");
    
    std::cout << "\n任务调度测试完成！" << std::endl;
    std::cout << "已生成以下文件：" << std::endl;
    std::cout << "  1. task_assignment_result.csv - 任务分配结果" << std::endl;
    std::cout << "  2. result.csv - 员工时间表（soln_shift.csv格式）" << std::endl;
    std::cout << "  3. employee_schedule_gantt.txt - 员工时间表（甘特图文本）" << std::endl;
    
    return 0;
}

