/**
 * @file test_load_scheduler.cpp
 * @brief 装卸任务调度器测试程序
 * 
 * 测试LoadScheduler的功能，包括任务生成、分配和调度
 */

#include "load_scheduler.h"
#include "load_employee_info.h"
#include "flight.h"
#include "../vip_first_class/shift.h"
#include "../vip_first_class/task_definition.h"
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
using namespace vip_first_class;
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

// 辅助函数：导出任务分配结果到CSV文件
static void exportToCSV(const vector<TaskDefinition>& tasks, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "错误：无法创建CSV文件 " << filename << endl;
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
    cout << "任务分配结果已导出到: " << filename << endl;
}

// 辅助函数：导出员工任务时间表到CSV
static void exportEmployeeScheduleToCSV(const vector<TaskDefinition>& tasks,
                                       const vector<LoadEmployeeInfo>& employees,
                                       const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "错误：无法创建CSV文件 " << filename << endl;
        return;
    }
    
    // 写入CSV表头
    file << "员工ID,员工姓名,装卸组,任务ID,任务名称,开始时间,结束时间\n";
    
    // 创建任务ID到任务的映射
    map<int64_t, const TaskDefinition*> task_map;
    for (const auto& task : tasks) {
        task_map[task.getTaskId()] = &task;
    }
    
    // 遍历所有员工
    for (const auto& emp : employees) {
        const auto& assigned_task_ids = emp.getEmployeeInfo().getAssignedTaskIds();
        
        if (assigned_task_ids.empty()) {
            // 如果没有分配任务，也输出一行
            file << emp.getEmployeeId() << ","
                 << emp.getEmployeeName() << ","
                 << emp.getLoadGroup() << ","
                 << ",,,\n";
        } else {
            // 按任务开始时间排序
            vector<pair<int64_t, int64_t>> task_times;  // {task_id, start_time}
            for (int64_t task_id : assigned_task_ids) {
                auto it = task_map.find(task_id);
                if (it != task_map.end() && it->second != nullptr) {
                    task_times.push_back({task_id, it->second->getStartTime()});
                }
            }
            
            sort(task_times.begin(), task_times.end(), 
                 [](const pair<int64_t, int64_t>& a, const pair<int64_t, int64_t>& b) {
                     return a.second < b.second;
                 });
            
            // 输出每个任务
            for (const auto& task_time : task_times) {
                int64_t task_id = task_time.first;
                auto it = task_map.find(task_id);
                if (it != task_map.end() && it->second != nullptr) {
                    const TaskDefinition& task = *(it->second);
                    file << emp.getEmployeeId() << ","
                         << emp.getEmployeeName() << ","
                         << emp.getLoadGroup() << ","
                         << task_id << ","
                         << task.getTaskName() << ","
                         << formatTime(task.getStartTime()) << ","
                         << formatTime(task.getEndTime()) << "\n";
                }
            }
        }
    }
    
    file.close();
    cout << "员工任务时间表已导出到: " << filename << endl;
}

int main() {
    cout << "开始测试装卸任务调度器..." << endl;
    
    // 1. 创建员工列表
    vector<LoadEmployeeInfo> employees;
    
    // 创建主班员工（3个组，每组3人）
    for (int group = 1; group <= 3; ++group) {
        for (int pos = 1; pos <= 3; ++pos) {
            LoadEmployeeInfo emp;
            ostringstream oss;
            oss << "main" << group << "_" << pos;
            emp.setEmployeeId(oss.str());
            emp.setEmployeeName("主班员工" + to_string(group) + "-" + to_string(pos));
            emp.setLoadGroup(group);
            emp.setQualificationMask(15);  // 所有资质
            employees.push_back(emp);
        }
    }
    
    // 创建副班员工（3个组，每组3人）
    for (int group = 1; group <= 3; ++group) {
        for (int pos = 1; pos <= 3; ++pos) {
            LoadEmployeeInfo emp;
            ostringstream oss;
            oss << "sub" << group << "_" << pos;
            emp.setEmployeeId(oss.str());
            emp.setEmployeeName("副班员工" + to_string(group) + "-" + to_string(pos));
            emp.setLoadGroup(group);
            emp.setQualificationMask(15);  // 所有资质
            employees.push_back(emp);
        }
    }
    
    cout << "创建了 " << employees.size() << " 个员工" << endl;
    
    // 2. 创建班次列表
    vector<Shift> shifts;
    
    // 主班1
    Shift main_shift1;
    main_shift1.setShiftType(1);  // 主班
    main_shift1.setEmployeeIdAtPosition(1, "main1_1");
    main_shift1.setEmployeeIdAtPosition(2, "main1_2");
    main_shift1.setEmployeeIdAtPosition(3, "main1_3");
    shifts.push_back(main_shift1);
    
    // 主班2
    Shift main_shift2;
    main_shift2.setShiftType(1);  // 主班
    main_shift2.setEmployeeIdAtPosition(1, "main2_1");
    main_shift2.setEmployeeIdAtPosition(2, "main2_2");
    main_shift2.setEmployeeIdAtPosition(3, "main2_3");
    shifts.push_back(main_shift2);
    
    // 主班3
    Shift main_shift3;
    main_shift3.setShiftType(1);  // 主班
    main_shift3.setEmployeeIdAtPosition(1, "main3_1");
    main_shift3.setEmployeeIdAtPosition(2, "main3_2");
    main_shift3.setEmployeeIdAtPosition(3, "main3_3");
    shifts.push_back(main_shift3);
    
    // 副班1
    Shift sub_shift1;
    sub_shift1.setShiftType(2);  // 副班
    sub_shift1.setEmployeeIdAtPosition(1, "sub1_1");
    sub_shift1.setEmployeeIdAtPosition(2, "sub1_2");
    sub_shift1.setEmployeeIdAtPosition(3, "sub1_3");
    shifts.push_back(sub_shift1);
    
    // 副班2
    Shift sub_shift2;
    sub_shift2.setShiftType(2);  // 副班
    sub_shift2.setEmployeeIdAtPosition(1, "sub2_1");
    sub_shift2.setEmployeeIdAtPosition(2, "sub2_2");
    sub_shift2.setEmployeeIdAtPosition(3, "sub2_3");
    shifts.push_back(sub_shift2);
    
    // 副班3
    Shift sub_shift3;
    sub_shift3.setShiftType(2);  // 副班
    sub_shift3.setEmployeeIdAtPosition(1, "sub3_1");
    sub_shift3.setEmployeeIdAtPosition(2, "sub3_2");
    sub_shift3.setEmployeeIdAtPosition(3, "sub3_3");
    shifts.push_back(sub_shift3);
    
    cout << "创建了 " << shifts.size() << " 个班次" << endl;
    
    // 3. 创建航班列表
    vector<Flight> flights;
    
    // 航班1：国内进港
    Flight flight1;
    flight1.setFlightTypeEnum(FlightType::DOMESTIC_ARRIVAL);
    flight1.setArrivalTime(parseTimeString("08:30"));
    flight1.setDepartureTime(parseTimeString("10:00"));  // 假设有起飞时间
    flight1.setStand(5);
    flight1.setReportTime(parseTimeString("08:25"));  // 已报时
    flight1.setArrivalCargo(1.5);  // 1.5吨
    flight1.setDepartureCargo(0);
    flight1.setRemoteStand(false);
    flight1.setVipTravelTime(480);  // 8分钟
    flights.push_back(flight1);
    
    // 航班2：国内出港
    Flight flight2;
    flight2.setFlightTypeEnum(FlightType::DOMESTIC_DEPARTURE);
    flight2.setArrivalTime(parseTimeString("09:00"));
    flight2.setDepartureTime(parseTimeString("11:00"));
    flight2.setStand(8);
    flight2.setReportTime(0);  // 未报时
    flight2.setArrivalCargo(0);
    flight2.setDepartureCargo(2.5);  // 2.5吨，需要6人
    flight2.setRemoteStand(false);
    flight2.setVipTravelTime(480);
    flights.push_back(flight2);
    
    // 航班3：国内过站（短过站）
    Flight flight3;
    flight3.setFlightTypeEnum(FlightType::DOMESTIC_TRANSIT);
    flight3.setArrivalTime(parseTimeString("10:30"));
    flight3.setDepartureTime(parseTimeString("11:00"));  // 30分钟，短过站
    flight3.setStand(12);
    flight3.setReportTime(parseTimeString("10:20"));  // 已报时
    flight3.setArrivalCargo(1.0);
    flight3.setDepartureCargo(1.5);
    flight3.setRemoteStand(false);
    flight3.setVipTravelTime(480);
    flights.push_back(flight3);
    
    // 航班4：国际进港
    Flight flight4;
    flight4.setFlightTypeEnum(FlightType::INTERNATIONAL_ARRIVAL);
    flight4.setArrivalTime(parseTimeString("12:00"));
    flight4.setDepartureTime(parseTimeString("14:00"));
    flight4.setStand(15);
    flight4.setReportTime(parseTimeString("11:55"));  // 已报时
    flight4.setArrivalCargo(1.0);
    flight4.setDepartureCargo(0);
    flight4.setRemoteStand(true);  // 远机位
    flight4.setVipTravelTime(480);
    flights.push_back(flight4);
    
    // 航班5：国际过站（长过站）
    Flight flight5;
    flight5.setFlightTypeEnum(FlightType::INTERNATIONAL_TRANSIT);
    flight5.setArrivalTime(parseTimeString("13:00"));
    flight5.setDepartureTime(parseTimeString("15:00"));  // 120分钟，长过站
    flight5.setStand(20);
    flight5.setReportTime(parseTimeString("12:50"));  // 已报时
    flight5.setArrivalCargo(2.0);
    flight5.setDepartureCargo(2.5);
    flight5.setRemoteStand(false);
    flight5.setVipTravelTime(480);
    flights.push_back(flight5);
    
    // 航班6：国内出港（远机位，货量大）
    Flight flight6;
    flight6.setFlightTypeEnum(FlightType::DOMESTIC_DEPARTURE);
    flight6.setArrivalTime(parseTimeString("14:00"));
    flight6.setDepartureTime(parseTimeString("16:00"));
    flight6.setStand(3);
    flight6.setReportTime(0);  // 未报时
    flight6.setArrivalCargo(0);
    flight6.setDepartureCargo(3.0);  // 3吨，需要6人
    flight6.setRemoteStand(true);  // 远机位
    flight6.setVipTravelTime(600);  // 10分钟（自定义通勤时间）
    flights.push_back(flight6);

    // 航班7：国内进港（已报时，货量正常）
    Flight flight7;
    flight7.setFlightTypeEnum(FlightType::DOMESTIC_ARRIVAL);
    flight7.setArrivalTime(parseTimeString("09:30"));
    flight7.setDepartureTime(parseTimeString("11:30"));
    flight7.setStand(6);
    flight7.setReportTime(parseTimeString("09:25"));
    flight7.setArrivalCargo(1.2);
    flight7.setDepartureCargo(0);
    flight7.setRemoteStand(false);
    flight7.setVipTravelTime(480);
    flights.push_back(flight7);
    
    // 航班8：国内出港（未报时，货量正常）
    Flight flight8;
    flight8.setFlightTypeEnum(FlightType::DOMESTIC_DEPARTURE);
    flight8.setArrivalTime(parseTimeString("10:00"));
    flight8.setDepartureTime(parseTimeString("12:30"));
    flight8.setStand(9);
    flight8.setReportTime(0);
    flight8.setArrivalCargo(0);
    flight8.setDepartureCargo(1.8);
    flight8.setRemoteStand(false);
    flight8.setVipTravelTime(480);
    flights.push_back(flight8);
    
    // 航班9：国内过站（短过站，已报时）
    Flight flight9;
    flight9.setFlightTypeEnum(FlightType::DOMESTIC_TRANSIT);
    flight9.setArrivalTime(parseTimeString("11:30"));
    flight9.setDepartureTime(parseTimeString("12:00"));  // 30分钟，短过站
    flight9.setStand(11);
    flight9.setReportTime(parseTimeString("11:20"));
    flight9.setArrivalCargo(0.8);
    flight9.setDepartureCargo(1.0);
    flight9.setRemoteStand(false);
    flight9.setVipTravelTime(480);
    flights.push_back(flight9);
    
    // 航班10：国际进港（已报时，货量大）
    Flight flight10;
    flight10.setFlightTypeEnum(FlightType::INTERNATIONAL_ARRIVAL);
    flight10.setArrivalTime(parseTimeString("13:30"));
    flight10.setDepartureTime(parseTimeString("15:30"));
    flight10.setStand(16);
    flight10.setReportTime(parseTimeString("13:25"));
    flight10.setArrivalCargo(2.8);  // 需要6人
    flight10.setDepartureCargo(0);
    flight10.setRemoteStand(false);
    flight10.setVipTravelTime(480);
    flights.push_back(flight10);
    
    // 航班11：国际出港（未报时，货量正常）
    Flight flight11;
    flight11.setFlightTypeEnum(FlightType::INTERNATIONAL_DEPARTURE);
    flight11.setArrivalTime(parseTimeString("14:30"));
    flight11.setDepartureTime(parseTimeString("17:00"));
    flight11.setStand(18);
    flight11.setReportTime(0);
    flight11.setArrivalCargo(0);
    flight11.setDepartureCargo(1.5);  // 国际通常需要6人
    flight11.setRemoteStand(false);
    flight11.setVipTravelTime(480);
    flights.push_back(flight11);
    
    // 航班12：国内过站（长过站，已报时）
    Flight flight12;
    flight12.setFlightTypeEnum(FlightType::DOMESTIC_TRANSIT);
    flight12.setArrivalTime(parseTimeString("15:30"));
    flight12.setDepartureTime(parseTimeString("17:30"));  // 120分钟，长过站
    flight12.setStand(2);
    flight12.setReportTime(parseTimeString("15:20"));
    flight12.setArrivalCargo(1.5);
    flight12.setDepartureCargo(2.2);  // 总货量3.7t，需要6人
    flight12.setRemoteStand(false);
    flight12.setVipTravelTime(480);
    flights.push_back(flight12);
    
    // 航班13：国内进港（远机位，已报时）
    Flight flight13;
    flight13.setFlightTypeEnum(FlightType::DOMESTIC_ARRIVAL);
    flight13.setArrivalTime(parseTimeString("16:00"));
    flight13.setDepartureTime(parseTimeString("18:00"));
    flight13.setStand(22);
    flight13.setReportTime(parseTimeString("15:55"));
    flight13.setArrivalCargo(1.0);
    flight13.setDepartureCargo(0);
    flight13.setRemoteStand(true);  // 远机位
    flight13.setVipTravelTime(480);
    flights.push_back(flight13);
    
    // 航班14：国际过站（长过站，未报时）
    Flight flight14;
    flight14.setFlightTypeEnum(FlightType::INTERNATIONAL_TRANSIT);
    flight14.setArrivalTime(parseTimeString("16:30"));
    flight14.setDepartureTime(parseTimeString("18:30"));  // 120分钟，长过站
    flight14.setStand(21);
    flight14.setReportTime(0);
    flight14.setArrivalCargo(2.0);
    flight14.setDepartureCargo(2.5);  // 总货量4.5t，需要6人
    flight14.setRemoteStand(false);
    flight14.setVipTravelTime(480);
    flights.push_back(flight14);
    
    // 航班15：国内出港（已报时，货量正常）
    Flight flight15;
    flight15.setFlightTypeEnum(FlightType::DOMESTIC_DEPARTURE);
    flight15.setArrivalTime(parseTimeString("17:00"));
    flight15.setDepartureTime(parseTimeString("19:00"));
    flight15.setStand(4);
    flight15.setReportTime(parseTimeString("16:55"));
    flight15.setArrivalCargo(0);
    flight15.setDepartureCargo(1.6);
    flight15.setRemoteStand(false);
    flight15.setVipTravelTime(480);
    flights.push_back(flight15);
    
    // 航班16：国内进港（未报时，货量正常）
    Flight flight16;
    flight16.setFlightTypeEnum(FlightType::DOMESTIC_ARRIVAL);
    flight16.setArrivalTime(parseTimeString("18:00"));
    flight16.setDepartureTime(parseTimeString("20:00"));
    flight16.setStand(7);
    flight16.setReportTime(0);
    flight16.setArrivalCargo(1.3);
    flight16.setDepartureCargo(0);
    flight16.setRemoteStand(false);
    flight16.setVipTravelTime(480);
    flights.push_back(flight16);
    
    // 航班17：国际出港（远机位，已报时，货量大）
    Flight flight17;
    flight17.setFlightTypeEnum(FlightType::INTERNATIONAL_DEPARTURE);
    flight17.setArrivalTime(parseTimeString("18:30"));
    flight17.setDepartureTime(parseTimeString("20:30"));
    flight17.setStand(23);
    flight17.setReportTime(parseTimeString("18:25"));
    flight17.setArrivalCargo(0);
    flight17.setDepartureCargo(3.5);  // 需要6人
    flight17.setRemoteStand(true);  // 远机位
    flight17.setVipTravelTime(600);  // 10分钟
    flights.push_back(flight17);
    
    // 航班18：国内过站（短过站，未报时）
    Flight flight18;
    flight18.setFlightTypeEnum(FlightType::DOMESTIC_TRANSIT);
    flight18.setArrivalTime(parseTimeString("19:00"));
    flight18.setDepartureTime(parseTimeString("19:30"));  // 30分钟，短过站
    flight18.setStand(13);
    flight18.setReportTime(0);
    flight18.setArrivalCargo(0.9);
    flight18.setDepartureCargo(1.1);  // 总货量2.0t，需要6人
    flight18.setRemoteStand(false);
    flight18.setVipTravelTime(480);
    flights.push_back(flight18);
    
    // 航班19：国际进港（已报时，货量正常）
    Flight flight19;
    flight19.setFlightTypeEnum(FlightType::INTERNATIONAL_ARRIVAL);
    flight19.setArrivalTime(parseTimeString("19:30"));
    flight19.setDepartureTime(parseTimeString("21:30"));
    flight19.setStand(17);
    flight19.setReportTime(parseTimeString("19:25"));
    flight19.setArrivalCargo(1.2);
    flight19.setDepartureCargo(0);
    flight19.setRemoteStand(false);
    flight19.setVipTravelTime(480);
    flights.push_back(flight19);
    
    // 航班20：国内出港（已报时，货量正常）
    Flight flight20;
    flight20.setFlightTypeEnum(FlightType::DOMESTIC_DEPARTURE);
    flight20.setArrivalTime(parseTimeString("20:00"));
    flight20.setDepartureTime(parseTimeString("22:00"));
    flight20.setStand(10);
    flight20.setReportTime(parseTimeString("19:55"));
    flight20.setArrivalCargo(0);
    flight20.setDepartureCargo(1.4);
    flight20.setRemoteStand(false);
    flight20.setVipTravelTime(480);
    flights.push_back(flight20);
    
    cout << "创建了 " << flights.size() << " 个航班" << endl;
    
    // 4. 创建班次占位时间段（疲劳度控制测试）
    vector<LoadScheduler::ShiftBlockPeriod> block_periods;
    LoadScheduler::ShiftBlockPeriod block1;
    block1.shift_type = 1;  // 主班
    block1.start_time = parseTimeString("12:00");
    block1.end_time = parseTimeString("13:00");
    block_periods.push_back(block1);
    
    cout << "创建了 " << block_periods.size() << " 个班次占位时间段" << endl;
    
    // 5. 调用任务调度
    LoadScheduler scheduler;
    vector<TaskDefinition> tasks;
    
    cout << "\n开始调度任务..." << endl;
    scheduler.scheduleLoadTasks(employees, flights, shifts, tasks, 480, block_periods, nullptr);
    
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
    
    cout << "\n=== 调度统计 ===" << endl;
    cout << "总任务数: " << total_tasks << endl;
    cout << "已分配任务: " << assigned_tasks << endl;
    cout << "未分配任务: " << unassigned_tasks << endl;
    cout << "缺人任务: " << short_staffed_tasks << endl;
    cout << "总需求人数: " << total_required << endl;
    cout << "总分配人数: " << total_assigned << endl;
    cout << "分配率: " << (total_required > 0 ? (total_assigned * 100.0 / total_required) : 0) << "%" << endl;
    
    // 7. 导出结果到CSV
    exportToCSV(tasks, "load_task_assignment_result.csv");
    exportEmployeeScheduleToCSV(tasks, employees, "load_employee_schedule.csv");
    
    cout << "\n测试完成！" << endl;
    cout << "已生成以下文件：" << endl;
    cout << "  1. load_task_assignment_result.csv - 任务分配结果" << endl;
    cout << "  2. load_employee_schedule.csv - 员工任务时间表" << endl;
    
    return 0;
}

