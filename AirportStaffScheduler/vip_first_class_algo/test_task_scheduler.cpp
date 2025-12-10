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

// 辅助函数：导出任务分配结果到CSV文件
static void exportToCSV(const std::vector<TaskDefinition>& tasks, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误：无法创建CSV文件 " << filename << std::endl;
        return;
    }
    cout<<"1111！"<<endl;
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
    int64_t task_id;
    std::string task_name;
    int64_t start_time;
    int64_t end_time;
    
    EmployeeTaskSlot(int64_t id, const std::string& name, int64_t start, int64_t end)
        : task_id(id), task_name(name), start_time(start), end_time(end) {}
};

// 辅助函数：导出员工时间表（甘特图格式）到CSV文件
static void exportEmployeeScheduleToCSV(const std::vector<TaskDefinition>& tasks, 
                                         const std::vector<Shift>& shifts,
                                         const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误：无法创建CSV文件 " << filename << std::endl;
        return;
    }
    
    // 收集每个员工的任务时间段
    std::map<std::string, std::vector<EmployeeTaskSlot>> employee_schedule;
    
    // 遍历所有任务，收集员工的任务分配
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
    
    // 获取所有员工的ID并排序（主班、副班、休息分开）
    std::vector<std::string> all_employee_ids;
    
    // 先添加主班员工
    for (int i = 1; i <= 8; ++i) {
        std::string emp_id = "main" + std::to_string(i);
        if (EmployeeManager::getInstance().hasEmployee(emp_id)) {
            all_employee_ids.push_back(emp_id);
        }
    }
    
    // 再添加副班员工
    for (int i = 1; i <= 8; ++i) {
        std::string emp_id = "sub" + std::to_string(i);
        if (EmployeeManager::getInstance().hasEmployee(emp_id)) {
            all_employee_ids.push_back(emp_id);
        }
    }
    
    // 最后添加休息员工（如果有任务分配）
    for (int i = 1; i <= 8; ++i) {
        std::string emp_id = "rest" + std::to_string(i);
        if (EmployeeManager::getInstance().hasEmployee(emp_id) && 
            employee_schedule.find(emp_id) != employee_schedule.end()) {
            all_employee_ids.push_back(emp_id);
        }
    }
    
    // 写入CSV表头
    file << "员工ID,员工姓名,班次类型,任务ID,任务名称,开始时间,结束时间,持续时间(分钟)\n";
    
    // 写入每个员工的时间表
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
        } else if (employee_id.find("rest") == 0) {
            shift_type_str = "休息";
        }
        
        // 获取该员工的任务列表
        auto schedule_it = employee_schedule.find(employee_id);
        if (schedule_it == employee_schedule.end() || schedule_it->second.empty()) {
            // 没有任务，只输出员工信息
            file << employee_id << ","
                 << employee->getEmployeeName() << ","
                 << shift_type_str << ","
                 << ",,,,\n";
        } else {
            // 有任务，输出每个任务时间段
            for (const auto& task_slot : schedule_it->second) {
                file << employee_id << ","
                     << employee->getEmployeeName() << ","
                     << shift_type_str << ","
                     << task_slot.task_id << ","
                     << task_slot.task_name << ","
                     << formatTime(task_slot.start_time) << ","
                     << formatTime(task_slot.end_time) << ",";
                
                // 计算持续时间（分钟）
                if (task_slot.end_time < 0) {
                    file << "航后\n";
                } else {
                    int64_t duration_seconds = task_slot.end_time - task_slot.start_time;
                    int64_t duration_minutes = duration_seconds / 60;
                    file << duration_minutes << "\n";
                }
            }
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
                char task_char = '0' + (task_slot.task_id % 10);
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
    
    // 1. 初始化员工信息
    // 主班员工：main1-8
    // 副班员工：sub1-8
    // 休息员工：rest1-8（虽然休息，但需要记录）
    for (int i = 1; i <= 8; ++i) {
        // 主班员工
        EmployeeInfo main_emp;
        main_emp.setEmployeeId("main" + std::to_string(i));
        main_emp.setEmployeeName("主班" + std::to_string(i));
        main_emp.setQualificationMask(15);  // 所有资质（1+2+4+8=15）
        EmployeeManager::getInstance().addOrUpdateEmployee("main" + std::to_string(i), main_emp);
        
        // 副班员工
        EmployeeInfo sub_emp;
        sub_emp.setEmployeeId("sub" + std::to_string(i));
        sub_emp.setEmployeeName("副班" + std::to_string(i));
        sub_emp.setQualificationMask(15);  // 所有资质
        EmployeeManager::getInstance().addOrUpdateEmployee("sub" + std::to_string(i), sub_emp);
        
        // 休息员工（虽然休息，但也创建员工信息）
        EmployeeInfo rest_emp;
        rest_emp.setEmployeeId("rest" + std::to_string(i));
        rest_emp.setEmployeeName("休息" + std::to_string(i));
        rest_emp.setQualificationMask(15);  // 所有资质
        EmployeeManager::getInstance().addOrUpdateEmployee("rest" + std::to_string(i), rest_emp);
    }
    
    // 2. 创建Shift对象
    // 主班Shift：位置1-8对应main1-8
    Shift main_shift;
    main_shift.setShiftType(1);  // 主班
    for (int i = 1; i <= 8; ++i) {
        main_shift.setEmployeeIdAtPosition(i, "main" + std::to_string(i));
    }
    
    // 副班Shift：位置1-8对应sub1-8
    Shift sub_shift;
    sub_shift.setShiftType(2);  // 副班
    for (int i = 1; i <= 8; ++i) {
        sub_shift.setEmployeeIdAtPosition(i, "sub" + std::to_string(i));
    }
    
    // 休息Shift：位置1-8对应rest1-8
    Shift rest_shift;
    rest_shift.setShiftType(0);  // 休息
    for (int i = 1; i <= 8; ++i) {
        rest_shift.setEmployeeIdAtPosition(i, "rest" + std::to_string(i));
    }
    
    std::vector<Shift> shifts;
    shifts.push_back(main_shift);
    shifts.push_back(sub_shift);
    shifts.push_back(rest_shift);
    
    // 3. 根据Task.txt创建任务列表
    std::vector<TaskDefinition> tasks;
    
    // 从Task.txt定义的任务
    struct TaskData {
        std::string name;
        std::string start_time;
        std::string end_time;
        int required_count;
    };
    
    // 从Task.txt定义的任务
    // 外场任务使用模拟时间（实际应用中需要根据航班信息计算）
    std::vector<TaskData> task_data_list = {
        {"调度", "08:30", "航后", 1},
        {"国内前台", "08:30", "航后", 1},
        {"国内前台协助", "07:00", "航后", 1},
        {"国内前台协助2", "06:30", "航后", 1},
        {"国内前台早班", "05:30", "08:30", 2},
        {"国际前台早班", "06:00", "14:00", 1},
        {"国际前台晚班", "14:00", "航后", 1},
        {"国际厅内早班", "06:00", "14:00", 2},
        {"国际厅内晚班", "14:00", "航后", 2},
        {"国内厅内早班", "05:30", "08:30", 2},
        {"国内厅内08:30-09:30", "08:30", "09:30", 2},
        {"国内厅内09:30-10:30", "09:30", "10:30", 2},
        {"国内厅内10:30-11:30", "10:30", "11:30", 3},
        {"国内厅内11:30-12:30", "11:30", "12:30", 3},
        {"国内厅内12:30-13:30", "12:30", "13:30", 3},
        {"国内厅内13:30-14:30", "13:30", "14:30", 2},
        {"国内厅内14:30-15:30", "14:30", "15:30", 2},
        {"国内厅内15:30-16:30", "15:30", "16:30", 2},
        {"国内厅内16:30-17:30", "16:30", "17:30", 3},
        {"国内厅内17:30-18:30", "17:30", "18:30", 3},
        {"国内厅内18:30-19:30", "18:30", "19:30", 3},
        {"国内厅内19:30-20:30", "19:30", "20:30", 2},
        {"国内厅内20:30-航后", "20:30", "航后", 2},
        // 外场任务（使用模拟时间，实际应用中需要根据航班信息计算）
        // 假设起飞时间10:00，则开始时间为09:00（起飞前60分钟），结束时间为10:30（登机结束后约15-20分钟）
        {"外场（国内出港-少人）", "09:00", "10:30", 2},
        {"外场（国内出港-多人）", "09:30", "11:00", 4},
        // 假设落地时间11:00，则开始时间为10:45（落地前15分钟），结束时间为11:20（登机结束后约15-20分钟）
        {"外场（国内进港-少人）", "10:45", "11:20", 2},
        {"外场（国内进港-多人）", "11:00", "11:40", 4},
        // 假设起飞时间14:00，则开始时间为13:00（起飞前60分钟），结束时间为14:30
        {"外场（国际出港-少人）", "13:00", "14:30", 2},
        {"外场（国际出港-少人）", "13:25", "14:30", 2},
        {"外场（国际出港-少人）", "18:00", "19:30", 2},
        {"外场（国际出港-少人）", "15:00", "17:30", 2},
        {"外场（国际出港-多人）", "20:30", "22:00", 4},
        {"外场（国际出港-少人）", "13:00", "14:30", 2},
        {"外场（国际出港-少人）", "13:25", "14:30", 2},
        {"外场（国际出港-少人）", "18:00", "19:30", 2},
        {"外场（国际出港-少人）", "15:00", "17:30", 2},
        {"外场（国际出港-多人）", "20:30", "22:00", 4},
        // 假设落地时间15:00，则开始时间为14:45（落地前15分钟），结束时间为15:20
        {"外场（国际进港-少人）", "14:45", "15:20", 2},
        {"外场（国际进港-多人）", "15:00", "15:40", 4}
    };
    
    int64_t task_id = 1;
    for (const auto& task_data : task_data_list) {
        TaskDefinition task;
        task.setTaskId(task_id++);
        task.setTaskName(task_data.name);
        task.setTaskType(parseTaskType(task_data.name));
        task.setStartTime(parseTimeString(task_data.start_time));
        
        if (task_data.end_time.find("航后") != std::string::npos) {
            task.setAfterFlight();  // 设置为航后
        } else {
            task.setEndTime(parseTimeString(task_data.end_time));
        }
        
        task.setRequiredCount(task_data.required_count);
        task.setPreferMainShift(true);  // 默认优先主班
        
        // 设置资质要求：外场任务需要外场资质（2），其他任务需要所有资质（15）
        if (task_data.name.find("外场") != std::string::npos) {
            task.setRequiredQualification(2);  // 外场资质
            task.setCanNewEmployee(false);  // 外场任务新员工不可
        } else {
            task.setRequiredQualification(15);  // 所有资质
            task.setCanNewEmployee(true);  // 默认允许新员工
        }
        
        task.setAllowOverlap(false);  // 默认不允许重叠
        task.setMaxOverlapTime(0);
        
        tasks.push_back(task);
    }
    
    // 4. 初始化TaskConfig（如果还没有初始化）
    TaskConfig::getInstance().initializeTaskPriorities();
    
    // 5. 调用任务调度
    TaskScheduler scheduler;
    scheduler.scheduleTasks(tasks, shifts);
    
    // 6. 导出结果到CSV
    exportToCSV(tasks, "task_assignment_result.csv");
    
    // 7. 导出员工时间表（甘特图格式）
    exportEmployeeScheduleToCSV(tasks, shifts, "employee_schedule.csv");
    exportGanttChartText(tasks, shifts, "employee_schedule_gantt.txt");
    
    std::cout << "\n任务调度测试完成！" << std::endl;
    std::cout << "已生成以下文件：" << std::endl;
    std::cout << "  1. task_assignment_result.csv - 任务分配结果" << std::endl;
    std::cout << "  2. employee_schedule.csv - 员工时间表（详细列表）" << std::endl;
    std::cout << "  3. employee_schedule_gantt.txt - 员工时间表（甘特图文本）" << std::endl;
    
    return 0;
}

