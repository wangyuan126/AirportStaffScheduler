/**
 * @file CommonAdapterUtils.h
 * @brief 公共类适配工具
 * 
 * 提供将AirportStaffScheduler公共类转换为模块内部类的转换函数
 */

#pragma once

#include "Staff.h"
#include "Task.h"
#include "Shift.h"
#include "DateTimeUtils.h"
#include "vip_first_class_algo/employee_info.h"
#include "vip_first_class_algo/task_definition.h"
#include "vip_first_class_algo/shift.h"
#include "vip_first_class_algo/task_type.h"
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>

namespace AirportStaffScheduler {
namespace Adapter {

using DateTime = std::chrono::system_clock::time_point;

// 基准时间：2020-01-01 00:00:00
const DateTime EPOCH_TIME = Utils::ParseDateTime("2020-01-01 00:00:00");

/**
 * @brief 获取基准时间（2020-01-01 00:00:00）
 */
inline DateTime GetEpochTime() {
    return EPOCH_TIME;
}

/**
 * @brief 将DateTime转换为从2020-01-01开始的秒数
 */
inline long DateTimeToSeconds(const DateTime& dt) {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(dt - EPOCH_TIME);
    return duration.count();
}

/**
 * @brief 将从2020-01-01开始的秒数转换为DateTime
 */
inline DateTime SecondsToDateTime(long seconds) {
    return EPOCH_TIME + std::chrono::seconds(seconds);
}

/**
 * @brief 资质字符串到位掩码的映射
 * 
 * 支持多种可能的资质字符串格式：
 * - 中文：厅内资质、外场资质、前台资质、调度资质
 * - 英文：HALL_INTERNAL、EXTERNAL、FRONT_DESK、DISPATCH
 * - 包含关键词：包含"厅内"、"外场"、"前台"、"调度"
 */
inline int QualificationStringToMask(const std::string& qual) {
    std::string qual_lower = qual;
    std::transform(qual_lower.begin(), qual_lower.end(), qual_lower.begin(), ::tolower);
    
    // 检查是否包含关键词（支持部分匹配）
    bool has_hall = qual.find("厅内") != std::string::npos || qual_lower.find("hall") != std::string::npos;
    bool has_external = qual.find("外场") != std::string::npos || qual_lower.find("external") != std::string::npos;
    bool has_front = qual.find("前台") != std::string::npos || qual_lower.find("front") != std::string::npos;
    bool has_dispatch = qual.find("调度") != std::string::npos || qual_lower.find("dispatch") != std::string::npos;
    
    int mask = 0;
    if (has_hall) {
        mask |= static_cast<int>(vip_first_class::QualificationMask::HALL_INTERNAL);
    }
    if (has_external) {
        mask |= static_cast<int>(vip_first_class::QualificationMask::EXTERNAL);
    }
    if (has_front) {
        mask |= static_cast<int>(vip_first_class::QualificationMask::FRONT_DESK);
    }
    if (has_dispatch) {
        mask |= static_cast<int>(vip_first_class::QualificationMask::DISPATCH);
    }
    
    return mask;
}

/**
 * @brief 将资质字符串列表转换为位掩码
 */
inline int QualificationsToMask(const std::vector<std::string>& quals) {
    int mask = 0;
    for (const auto& qual : quals) {
        mask |= QualificationStringToMask(qual);
    }
    return mask;
}

/**
 * @brief 将位掩码转换为资质字符串列表
 */
inline std::vector<std::string> MaskToQualifications(int mask) {
    std::vector<std::string> quals;
    if (mask & static_cast<int>(vip_first_class::QualificationMask::HALL_INTERNAL)) {
        quals.push_back("厅内资质");
    }
    if (mask & static_cast<int>(vip_first_class::QualificationMask::EXTERNAL)) {
        quals.push_back("外场资质");
    }
    if (mask & static_cast<int>(vip_first_class::QualificationMask::FRONT_DESK)) {
        quals.push_back("前台资质");
    }
    if (mask & static_cast<int>(vip_first_class::QualificationMask::DISPATCH)) {
        quals.push_back("调度资质");
    }
    return quals;
}

/**
 * @brief 将Staff转换为vip_first_class::EmployeeInfo
 */
inline vip_first_class::EmployeeInfo StaffToEmployeeInfo(const Staff& staff) {
    vip_first_class::EmployeeInfo emp_info;
    emp_info.setEmployeeId(staff.getStaffId());
    emp_info.setEmployeeName(staff.getName());
    
    // 转换资质：将字符串列表转换为位掩码
    std::vector<std::string> qual_list(staff.getQualifications().begin(), 
                                       staff.getQualifications().end());
    int mask = QualificationsToMask(qual_list);
    emp_info.setQualificationMask(mask);
    
    return emp_info;
}

/**
 * @brief 将vip_first_class::EmployeeInfo转换为Staff
 */
inline Staff EmployeeInfoToStaff(const vip_first_class::EmployeeInfo& emp_info) {
    // 转换资质：将位掩码转换为字符串列表
    std::vector<std::string> qual_list = MaskToQualifications(emp_info.getQualificationMask());
    
    Staff staff(emp_info.getEmployeeId(),
                emp_info.getEmployeeName(),
                "",  // gender 需要从其他地方获取
                qual_list);
    
    return staff;
}

/**
 * @brief 将Task转换为vip_first_class::TaskDefinition
 * 
 * 注意：Task类型信息需要通过task_name映射到TaskType，这里提供一个基础版本
 * 实际使用时可能需要根据task_name进行映射
 */
inline vip_first_class::TaskDefinition TaskToTaskDefinition(
    const Task& task,
    vip_first_class::TaskType task_type = vip_first_class::TaskType::DISPATCH) {
    
    vip_first_class::TaskDefinition task_def;
    
    // 转换任务ID（从string到long，如果失败则使用hash）
    try {
        long task_id = std::stol(task.getTaskId());
        task_def.setTaskId(task_id);
    } catch (...) {
        // 如果转换失败，使用hash值
        std::hash<std::string> hasher;
        task_def.setTaskId(static_cast<long>(hasher(task.getTaskId())));
    }
    
    task_def.setTaskName(task.getTaskName());
    task_def.setTaskType(task_type);
    
    // 转换时间
    long start_seconds = DateTimeToSeconds(task.getTaskStartTime());
    long end_seconds = DateTimeToSeconds(task.getTaskEndTime());
    task_def.setStartTime(start_seconds);
    
    // 检查是否为航后任务（这里简化处理，实际可能需要根据业务逻辑判断）
    // 如果结束时间是一个特殊值，设置为-1
    if (end_seconds < start_seconds) {
        task_def.setAfterFlight();  // 设置为航后
    } else {
        task_def.setEndTime(end_seconds);
    }
    
    // 转换资质要求
    int qual_mask = QualificationsToMask(task.getRequiredQualifications());
    task_def.setRequiredQualification(qual_mask);
    
    // 设置其他属性（根据Task的属性映射）
    task_def.setCanNewEmployee(true);  // 默认允许新员工，可根据需要调整
    task_def.setAllowOverlap(false);   // 默认不允许重叠，可根据需要调整
    task_def.setRequiredCount(1);      // 默认需要1人，可根据需要调整
    
    return task_def;
}

/**
 * @brief 将vip_first_class::TaskDefinition转换为Task
 */
inline Task TaskDefinitionToTask(const vip_first_class::TaskDefinition& task_def) {
    // 转换时间
    DateTime start_time = SecondsToDateTime(task_def.getStartTime());
    DateTime end_time;
    if (task_def.isAfterFlight()) {
        // 航后任务：设置一个默认的结束时间（当天22:30）
        long task_day = task_def.getStartTime() / (24 * 3600);
        long default_end = task_day * (24 * 3600) + 22 * 3600 + 30 * 60;
        end_time = SecondsToDateTime(default_end);
    } else {
        end_time = SecondsToDateTime(task_def.getEndTime());
    }
    
    // 转换资质
    std::vector<std::string> qual_list = MaskToQualifications(task_def.getRequiredQualification());
    
    // 计算时长（分钟）
    long duration_seconds = DateTimeToSeconds(end_time) - DateTimeToSeconds(start_time);
    int duration_minutes = static_cast<int>(duration_seconds / 60);
    
    // 创建Task对象
    Task task(
        std::to_string(task_def.getTaskId()),  // taskId
        "",  // taskDefId
        task_def.getTaskName(),  // taskName
        start_time,  // taskStartTime
        end_time,  // taskEndTime
        duration_minutes,  // durationMinutes
        "",  // inOrOutPort
        "",  // flightAttribute
        true,  // mustAssign
        false,  // isLocked
        start_time,  // arrivalEstimatedLandingTime
        start_time,  // arrivalScheduledLandingTime
        end_time,  // departureScheduledTakeoffTime
        end_time   // departureEstimatedTakeoffTime
    );
    
    task.setRequiredQualifications(qual_list);
    
    return task;
}

/**
 * @brief 将AirportStaffScheduler::Shift转换为vip_first_class::Shift
 * 
 * 注意：AirportStaffScheduler::Shift是单个员工的班次，
 * 而vip_first_class::Shift包含多个位置到员工的映射。
 * 这个函数创建一个只包含一个位置的Shift。
 */
inline vip_first_class::Shift ShiftToVipShift(
    const AirportStaffScheduler::Shift& shift,
    int position = 1) {
    
    vip_first_class::Shift vip_shift;
    
    // 根据shiftName推断shift_type（这里简化处理）
    // 实际使用时可能需要更复杂的映射逻辑
    const std::string& shift_name = shift.getShiftName();
    if (shift_name.find("主班") != std::string::npos) {
        vip_shift.setShiftType(1);  // 主班
    } else if (shift_name.find("副班") != std::string::npos) {
        vip_shift.setShiftType(2);  // 副班
    } else {
        vip_shift.setShiftType(0);  // 休息
    }
    
    // 设置位置到员工的映射
    vip_shift.setEmployeeIdAtPosition(position, shift.getStaffId());
    
    return vip_shift;
}

/**
 * @brief 将vip_first_class::Shift转换为AirportStaffScheduler::Shift列表
 * 
 * 因为vip_first_class::Shift可能包含多个位置，所以返回一个列表
 */
inline std::vector<AirportStaffScheduler::Shift> VipShiftToShifts(
    const vip_first_class::Shift& vip_shift,
    const std::string& shift_id_prefix = "",
    const DateTime& start_time = EPOCH_TIME,
    const DateTime& end_time = EPOCH_TIME,
    const std::string& bound_terminal = "") {
    
    std::vector<AirportStaffScheduler::Shift> shifts;
    
    const auto& pos_map = vip_shift.getPositionToEmployeeId();
    for (const auto& pos_pair : pos_map) {
        int position = pos_pair.first;
        const std::string& staff_id = pos_pair.second;
        
        // 根据shift_type生成shift_name
        std::string shift_name;
        if (vip_shift.getShiftType() == 1) {
            shift_name = "主班" + std::to_string(position);
        } else if (vip_shift.getShiftType() == 2) {
            shift_name = "副班" + std::to_string(position);
        } else {
            shift_name = "休息";
        }
        
        std::string shift_id = shift_id_prefix + "_" + std::to_string(position);
        
        AirportStaffScheduler::Shift shift(
            shift_id,
            shift_name,
            staff_id,
            start_time,
            end_time,
            bound_terminal,
            false,  // allowOvertime
            false   // avoidAssignIfPossible
        );
        
        shifts.push_back(shift);
    }
    
    return shifts;
}

}  // namespace Adapter
}  // namespace AirportStaffScheduler

