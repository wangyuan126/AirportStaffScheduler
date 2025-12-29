/**
 * @file test_load_scheduler.cpp
 * @brief 装卸任务调度器测试程序
 * 
 * 测试LoadScheduler的功能，包括任务生成、分配和调度
 */

#include "load_scheduler.h"
#include "load_employee_info.h"
#include "load_task.h"
#include "../vip_first_class_algo/shift.h"
#include "../CSVDataLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <algorithm>
#include <cstdint>

using namespace zhuangxie_class;
using namespace std;

// 辅助函数：将时间字符串（如"08:30"）转换为从2020-01-01 00:00:00开始的秒数
static int64_t parseTimeString(const string& time_str) {
    if (time_str.find("航后") != string::npos) {
        return -1;  // 航后任务
    }
    
    // 解析HH:MM格式
    size_t colon_pos = time_str.find(':');
    if (colon_pos == string::npos) {
        return 0;
    }
    
    int hours = stoi(time_str.substr(0, colon_pos));
    int minutes = stoi(time_str.substr(colon_pos + 1));
    
    // 转换为秒数（从2020-01-01 00:00:00开始）
    return hours * 3600 + minutes * 60;
}

// 辅助函数：将秒数转换为时间字符串（用于CSV输出）
static string formatTime(int64_t seconds) {
    if (seconds < 0) {
        return "航后";
    }
    
    int64_t hours = seconds / 3600;
    int64_t minutes = (seconds % 3600) / 60;
    
    ostringstream oss;
    oss << setfill('0') << setw(2) << hours 
        << ":" << setw(2) << minutes;
    return oss.str();
}

// 辅助函数：将秒数和任务日期转换为日期时间字符串（YYYY-MM-DD HH:MM:SS格式）
// 注意：seconds 是从当天00:00:00开始的秒数（parseDateTimeString返回的值）
// 基准日期：2020-01-01 00:00:00
static string formatDateTime(int64_t seconds, const string& task_date = "") {
    if (seconds <= 0) {
        return "";
    }
    
    // 如果提供了任务日期，使用任务日期作为日期部分
    if (!task_date.empty()) {
        // 从任务日期中提取日期部分（YYYY-MM-DD）
        string date_part = task_date;
        // 移除可能的引号
        if (date_part.length() >= 2 && date_part.front() == '"' && date_part.back() == '"') {
            date_part = date_part.substr(1, date_part.length() - 2);
        }
        
        // seconds 是从当天00:00:00开始的秒数（parseDateTimeString返回的值）
        // 直接计算时间部分
        int64_t total_seconds = seconds;
        
        // 处理跨天的情况：如果秒数超过一天，需要调整日期
        int64_t days = total_seconds / 86400;
        int64_t remaining_seconds = total_seconds % 86400;
        
        // 如果跨天了，需要调整日期（简化处理：直接加天数）
        if (days > 0) {
            // 解析任务日期
            size_t dash1 = date_part.find('-');
            size_t dash2 = date_part.find('-', dash1 + 1);
            if (dash1 != string::npos && dash2 != string::npos) {
                int year = stoi(date_part.substr(0, dash1));
                int month = stoi(date_part.substr(dash1 + 1, dash2 - dash1 - 1));
                int day = stoi(date_part.substr(dash2 + 1));
                
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
                
                ostringstream date_oss;
                date_oss << year << "-"
                         << setfill('0') << setw(2) << month << "-"
                         << setfill('0') << setw(2) << day;
                date_part = date_oss.str();
            }
        }
        
        int hours = static_cast<int>(remaining_seconds / 3600);
        int minutes = static_cast<int>((remaining_seconds % 3600) / 60);
        int secs = static_cast<int>(remaining_seconds % 60);
        
        ostringstream oss;
        oss << date_part << " "
            << setfill('0') << setw(2) << hours << ":"
            << setfill('0') << setw(2) << minutes << ":"
            << setfill('0') << setw(2) << secs;
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
    day += days;
    while (day > 30) {
        day -= 30;
        month++;
        if (month > 12) {
            month = 1;
            year++;
        }
    }
    
    int hours = remaining_seconds / 3600;
    int minutes = (remaining_seconds % 3600) / 60;
    int secs = remaining_seconds % 60;
    
    ostringstream oss;
    oss << year << "-" 
        << setfill('0') << setw(2) << month << "-"
        << setfill('0') << setw(2) << day << " "
        << setfill('0') << setw(2) << hours << ":"
        << setfill('0') << setw(2) << minutes << ":"
        << setfill('0') << setw(2) << secs;
    return oss.str();
}

// 辅助函数：导出任务分配结果到CSV文件
static void exportToCSV(const vector<LoadTask>& tasks, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "错误：无法创建CSV文件 " << filename << endl;
        return;
    }
    
    // 写入CSV表头
    file << "任务ID,任务名称,任务类型,最早开始时间,最晚结束时间,任务时长,实际开始时间,实际结束时间,需要人数,已分配人数,是否已分配,是否缺人,分配的员工ID\n";
    
    // 写入每个任务的详细信息
    for (const auto& task : tasks) {
        file << task.getTaskId() << ","
             << task.getTaskName() << ","
             << task.getFlightType() << ","
             << formatTime(task.getEarliestStartTime()) << ","
             << formatTime(task.getLatestEndTime()) << ","
             << task.getDuration() << ","
             << formatTime(task.getActualStartTime()) << ","
             << formatTime(task.getActualEndTime()) << ","
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
    cout << "任务分配结果已导出到: " << filename << endl;
}

// 辅助函数：导出员工任务时间表到CSV（按照soln_shift.csv格式）
static void exportEmployeeScheduleToCSV(const vector<LoadTask>& tasks,
                                       const vector<LoadEmployeeInfo>& employees,
                                       const vector<vip_first_class::Shift>& shifts,
                                       const string& filename) {
    // 尝试以二进制模式打开文件，避免Windows下的换行符问题
    ofstream file(filename, ios::out | ios::trunc);
    if (!file.is_open()) {
        cerr << "错误：无法创建CSV文件 " << filename << endl;
        cerr << "请检查文件路径和权限" << endl;
        return;
    }
    
    // 写入CSV表头（按照soln_shift.csv格式）
    file << "班期日期,班期开始时间,班期结束时间,人员编号,人员姓名,车辆（车牌号）,车辆类型,任务ID,任务名称,任务日期,任务开始时间,任务结束时间,到达航班ID,出发航班ID,到达航班号,出发航班号,航站楼,区域,机位,其他位置,双机航班号,是否加班\n";
    
    // 创建任务ID到任务的映射
    map<string, const LoadTask*> task_map;
    for (const auto& task : tasks) {
        task_map[task.getTaskId()] = &task;
    }
    
    // 遍历所有员工
    for (const auto& emp : employees) {
        const auto& assigned_task_ids = emp.getEmployeeInfo().getAssignedTaskIds();
        
        if (assigned_task_ids.empty()) {
            // 如果没有分配任务（休息），不输出或输出空行
            // 根据用户要求：如果是休息就不输出
            continue;
        }
        
            // 按任务开始时间排序
        vector<pair<string, long>> task_times;  // {task_id, start_time}
        for (const string& task_id : assigned_task_ids) {
                auto it = task_map.find(task_id);
                if (it != task_map.end() && it->second != nullptr) {
                long start_time = it->second->getActualStartTime();
                if (start_time > 0) {
                    task_times.push_back({task_id, start_time});
                }
            }
        }
        
        if (task_times.empty()) {
            continue;
            }
            
            sort(task_times.begin(), task_times.end(), 
             [](const pair<string, long>& a, const pair<string, long>& b) {
                     return a.second < b.second;
                 });
            
        // 计算班期开始时间和结束时间
        long shift_start_time = task_times.front().second;  // 第一个任务的开始时间
        long shift_end_time = 0;
        
        // 遍历所有任务，找到最晚的结束时间（确保是那个人最晚的任务的结束时间）
        for (const auto& task_time : task_times) {
            auto it = task_map.find(task_time.first);
            if (it != task_map.end() && it->second != nullptr) {
                long end_time = it->second->getActualEndTime();
                // 如果实际结束时间为0，尝试使用实际开始时间 + 时长
                if (end_time == 0) {
                    long actual_start = it->second->getActualStartTime();
                    long duration = it->second->getDuration();
                    if (actual_start > 0 && duration > 0) {
                        end_time = actual_start + duration;
                    }
                }
                // 更新最晚的结束时间
                if (end_time > shift_end_time) {
                    shift_end_time = end_time;
                }
            }
        }
        
        // 获取第一个任务的日期（用于班期日期）
        string first_task_date = "";
        auto first_task_it = task_map.find(task_times.front().first);
        if (first_task_it != task_map.end() && first_task_it->second != nullptr) {
            first_task_date = first_task_it->second->getTaskDate();
        }
        
            // 输出每个任务
            for (const auto& task_time : task_times) {
            const string& task_id = task_time.first;
                auto it = task_map.find(task_id);
                if (it != task_map.end() && it->second != nullptr) {
                const LoadTask& task = *(it->second);
                
                // 班期日期（使用第一个任务的日期）
                file << "\"" << first_task_date << "\",";
                
                // 班期开始时间（第一个任务的开始时间）
                string shift_start_str = formatDateTime(shift_start_time, first_task_date);
                file << "\"" << shift_start_str << "\",";
                
                // 班期结束时间（最后一个任务的结束时间）
                string shift_end_str = formatDateTime(shift_end_time, first_task_date);
                file << "\"" << shift_end_str << "\",";
                
                // 人员编号
                file << "\"" << emp.getEmployeeId() << "\",";
                
                // 人员姓名
                file << "\"" << emp.getEmployeeName() << "\",";
                
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
                string task_start_str = formatDateTime(task.getActualStartTime(), task.getTaskDate());
                file << "\"" << task_start_str << "\",";
                
                // 任务结束时间（实际开始时间 + 时长）
                string task_end_str = formatDateTime(task.getActualEndTime(), task.getTaskDate());
                file << "\"" << task_end_str << "\",";
                
                // 到达航班ID（如果无就不输出）
                string arrival_flight_id = task.getArrivalFlightId();
                file << "\"" << (arrival_flight_id.empty() ? "" : arrival_flight_id) << "\",";
                
                // 出发航班ID（如果无就不输出）
                string departure_flight_id = task.getDepartureFlightId();
                file << "\"" << (departure_flight_id.empty() ? "" : departure_flight_id) << "\",";
                
                // 到达航班号（如果无就不输出）
                string arrival_flight_number = task.getArrivalFlightNumber();
                file << "\"" << (arrival_flight_number.empty() ? "" : arrival_flight_number) << "\",";
                
                // 出发航班号（如果无就不输出）
                string departure_flight_number = task.getDepartureFlightNumber();
                file << "\"" << (departure_flight_number.empty() ? "" : departure_flight_number) << "\",";
                
                // 航站楼（如果无就不输出）
                string terminal = task.getTerminal();
                file << "\"" << (terminal.empty() ? "" : terminal) << "\",";
                
                // 区域 - 不输出
                file << "\"\",";
                
                // 机位（如果无就不输出）
                int stand = task.getStand();
                if (stand > 0) {
                    file << "\"" << stand << "\",";
                } else {
                    file << "\"\",";
                }
                
                // 其他位置 - 不输出
                file << "\"\",";
                
                // 双机航班号 - 不输出
                file << "\"\",";
                
                // 是否加班 - 全部都是否
                file << "\"否\",";
                
                file << "\n";
            }
        }
    }
    
    file.close();
    cout << "员工任务时间表已导出到: " << filename << endl;
}

int main(int argc, char* argv[]) {
    // 确保输出立即刷新
    std::ios::sync_with_stdio(true);
    std::cout.setf(std::ios::unitbuf);  // 无缓冲输出
    
    cout << "=== Loading CSV Test Program ===" << endl;
    cout << "Starting load scheduler test..." << endl;
    cout.flush();
    
    // 确定CSV文件路径
    std::string input_dir = "../input/";
    if (argc > 1) {
        input_dir = argv[1];
        if (input_dir.back() != '/' && input_dir.back() != '\\') {
            input_dir += "/";
        }
    }
    
    std::string staff_csv = input_dir + "staff.csv";
    std::string shift_csv = input_dir + "shift.csv";
    std::string task_csv = input_dir + "task.csv";
    
    // 1. 从shift.csv加载班次列表和员工信息
    cout << "Step 1: Loading shifts and employees from CSV..." << endl;
    cout << "CSV file path: " << shift_csv << endl;
    cout.flush();
    
    vector<vip_first_class::Shift> shifts;
    vector<LoadEmployeeInfo> employees;
    map<string, vector<string>> group_name_to_employees;
    
    try {
        shifts = AirportStaffScheduler::CSVLoader::loadShiftsFromCSV(shift_csv);
        if (shifts.empty()) {
            throw std::runtime_error("CSV file contains no valid shift data");
        }
        
        // 从shift.csv中读取员工信息和班组信息
        bool success = AirportStaffScheduler::CSVLoader::loadEmployeesFromShiftCSV(
            shift_csv, employees, group_name_to_employees);
        if (!success || employees.empty()) {
            throw std::runtime_error("Failed to load employees from shift.csv");
        }
        
        cout << "Successfully loaded " << employees.size() << " employees from shift.csv" << endl;
        cout << "Found " << group_name_to_employees.size() << " groups" << endl;
    } catch (const std::exception& e) {
        cerr << "ERROR: Failed to load shifts/employees: " << e.what() << endl;
        cerr << "Using default test data..." << endl;
        
        // 使用默认测试数据作为后备
        shifts.clear();
        employees.clear();
        group_name_to_employees.clear();
        
        // 创建主班员工（3个组，每组3人）
        for (int group = 1; group <= 3; ++group) {
            string group_name = "1." + to_string(group);
            for (int pos = 1; pos <= 3; ++pos) {
                LoadEmployeeInfo emp;
                ostringstream oss;
                oss << "main" << group << "_" << pos;
                emp.setEmployeeId(oss.str());
                emp.setEmployeeName("主班员工" + to_string(group) + "-" + to_string(pos));
                emp.setLoadGroup(group);
                emp.setQualificationMask(15);  // 所有资质
                employees.push_back(emp);
                group_name_to_employees[group_name].push_back(oss.str());
            }
        }
        // 创建副班员工（3个组，每组3人）
        for (int group = 1; group <= 3; ++group) {
            string group_name = "2." + to_string(group);
            for (int pos = 1; pos <= 3; ++pos) {
                LoadEmployeeInfo emp;
                ostringstream oss;
                oss << "sub" << group << "_" << pos;
                emp.setEmployeeId(oss.str());
                emp.setEmployeeName("副班员工" + to_string(group) + "-" + to_string(pos));
                emp.setLoadGroup(group);
                emp.setQualificationMask(15);  // 所有资质
                employees.push_back(emp);
                group_name_to_employees[group_name].push_back(oss.str());
            }
        }
    }
    
    cout << "Total employees: " << employees.size() << endl;
    cout << "Total shifts: " << shifts.size() << endl;
    cout.flush();
    
    // 3. 从task.csv加载任务列表（LoadTask对象）
    cout << "Step 3: Loading tasks from task.csv..." << endl;
    cout << "CSV file path: " << task_csv << endl;
    cout.flush();
    
    vector<LoadTask> tasks;
    std::string stand_pos_csv = input_dir + "stand_pos.csv";
    try {
        bool success = AirportStaffScheduler::CSVLoader::loadLoadTasksFromCSV(task_csv, tasks, stand_pos_csv);
        if (!success || tasks.empty()) {
            throw std::runtime_error("CSV file contains no valid task data");
        }
        cout << "Successfully loaded " << tasks.size() << " tasks from task.csv" << endl;
    } catch (const std::exception& e) {
        cerr << "ERROR: Failed to load tasks: " << e.what() << endl;
        cerr << "Using empty task list..." << endl;
        tasks.clear();
    }
    
    cout << "Total tasks: " << tasks.size() << endl;
    cout.flush();
    
    // 4. 创建班次占位时间段（疲劳度控制测试）
    vector<LoadScheduler::ShiftBlockPeriod> block_periods;
    LoadScheduler::ShiftBlockPeriod block1;
    block1.shift_type = 1;  // 主班
    block1.start_time = parseTimeString("12:00");
    block1.end_time = parseTimeString("13:00");
    block_periods.push_back(block1);
    
    cout << "Block periods: " << block_periods.size() << endl;
    cout.flush();
    
    // 5. 调用任务调度
    cout << "Step 4: Starting task scheduling..." << endl;
    cout.flush();
    
    LoadScheduler scheduler;
    scheduler.scheduleLoadTasks(employees, tasks, shifts, block_periods, nullptr, &group_name_to_employees);
    
    // 6. 输出统计信息
    int total_tasks = tasks.size();
    int assigned_tasks = 0;
    int unassigned_tasks = 0;
    int short_staffed_tasks = 0;
    int total_required = 0;
    int total_assigned = 0;
    
    for (const auto& task : tasks) {
        if (task.isAssigned() && task.getAssignedEmployeeCount() > 0) {
            assigned_tasks++;
        } else {
            unassigned_tasks++;
        }
        if (task.isShortStaffed()) {
            short_staffed_tasks++;
        }
        total_required += task.getRequiredCount();
        total_assigned += static_cast<int>(task.getAssignedEmployeeCount());
    }
    
    cout << "\n=== Scheduling Statistics ===" << endl;
    cout << "Total tasks: " << total_tasks << endl;
    cout << "Assigned tasks: " << assigned_tasks << endl;
    cout << "Unassigned tasks: " << unassigned_tasks << endl;
    cout << "Short-staffed tasks: " << short_staffed_tasks << endl;
    cout << "Total required staff: " << total_required << endl;
    cout << "Total assigned staff: " << total_assigned << endl;
    cout << "Assignment rate: " << (total_required > 0 ? (total_assigned * 100.0 / total_required) : 0) << "%" << endl;
    cout.flush();
    
    // 7. 导出结果到CSV（只输出一个文件，格式与soln_shift.csv一致）
    cout << "\nStep 5: Exporting results to CSV file..." << endl;
    cout.flush();
    
    exportEmployeeScheduleToCSV(tasks, employees, shifts, "result.csv");
    
    cout << "\n=== Test Completed Successfully ===" << endl;
    cout << "Generated file:" << endl;
    cout << "  result.csv - Employee schedule (soln_shift.csv format)" << endl;
    cout.flush();
    
    return 0;
}

