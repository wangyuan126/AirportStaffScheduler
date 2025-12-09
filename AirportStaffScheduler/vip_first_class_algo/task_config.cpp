/**
 * @file task_config.cpp
 * @brief 任务配置类实现
 */

#include "task_config.h"
#include "shift.h"
#include "task_definition.h"
#include "employee_manager.h"
#include <cstdint>
#include <set>
#include <algorithm>
#include <vector>
#include <string>

namespace vip_first_class {

using namespace std;

// 静态空列表定义
const vector<FixedPersonInfo> TaskConfig::empty_list_;
const vector<string> TaskConfig::empty_employee_list_;

TaskConfig& TaskConfig::getInstance()
{
    static TaskConfig instance;
    return instance;
}

TaskConfig::TaskConfig()
{
    initializeDefaultConfig();
    initializeTaskPriorities();
}

TaskConfig::~TaskConfig()
{
}

void TaskConfig::initializeDefaultConfig()
{
    // 清空现有配置
    clear();
    
    // 使用任务类型枚举值作为ID的基础
    // 1. 调度：仅固定主1
    int64_t task_id = static_cast<int64_t>(TaskType::DISPATCH);
    task_type_to_id_[TaskType::DISPATCH] = task_id;
    task_id_to_fixed_persons_[task_id].push_back(FixedPersonInfo(ShiftCategory::MAIN, 1));
    
    // 2. 国际前台早班：仅固定副1
    task_id = static_cast<int64_t>(TaskType::INTERNATIONAL_FRONT_DESK_EARLY);
    task_type_to_id_[TaskType::INTERNATIONAL_FRONT_DESK_EARLY] = task_id;
    task_id_to_fixed_persons_[task_id].push_back(FixedPersonInfo(ShiftCategory::SUB, 1));
    
    // 3. 国际前台晚班：仅固定副2
    task_id = static_cast<int64_t>(TaskType::INTERNATIONAL_FRONT_DESK_LATE);
    task_type_to_id_[TaskType::INTERNATIONAL_FRONT_DESK_LATE] = task_id;
    task_id_to_fixed_persons_[task_id].push_back(FixedPersonInfo(ShiftCategory::SUB, 2));
    
    // 4. 国际厅内早班：仅固定副3
    task_id = static_cast<int64_t>(TaskType::INTERNATIONAL_HALL_EARLY);
    task_type_to_id_[TaskType::INTERNATIONAL_HALL_EARLY] = task_id;
    task_id_to_fixed_persons_[task_id].push_back(FixedPersonInfo(ShiftCategory::SUB, 3));
    
    // 5. 国际厅内晚班：固定副5
    task_id = static_cast<int64_t>(TaskType::INTERNATIONAL_HALL_LATE);
    task_type_to_id_[TaskType::INTERNATIONAL_HALL_LATE] = task_id;
    task_id_to_fixed_persons_[task_id].push_back(FixedPersonInfo(ShiftCategory::SUB, 5));
    
    // 6. 国内厅内早班：固定副4
    task_id = static_cast<int64_t>(TaskType::DOMESTIC_HALL_EARLY);
    task_type_to_id_[TaskType::DOMESTIC_HALL_EARLY] = task_id;
    task_id_to_fixed_persons_[task_id].push_back(FixedPersonInfo(ShiftCategory::SUB, 4));
    
    // 7. 国内前台早班：有副6和主5
    task_id = static_cast<int64_t>(TaskType::DOMESTIC_FRONT_DESK_EARLY);
    task_type_to_id_[TaskType::DOMESTIC_FRONT_DESK_EARLY] = task_id;
    task_id_to_fixed_persons_[task_id].push_back(FixedPersonInfo(ShiftCategory::SUB, 6));
    task_id_to_fixed_persons_[task_id].push_back(FixedPersonInfo(ShiftCategory::MAIN, 5));

    
}

const vector<FixedPersonInfo>& TaskConfig::getFixedPersons(int64_t task_id) const
{
    auto it = task_id_to_fixed_persons_.find(task_id);
    if (it != task_id_to_fixed_persons_.end()) {
        return it->second;
    }
    return empty_list_;
}

const vector<FixedPersonInfo>& TaskConfig::getFixedPersonsByType(TaskType task_type) const
{
    auto type_it = task_type_to_id_.find(task_type);
    if (type_it != task_type_to_id_.end()) {
        return getFixedPersons(type_it->second);
    }
    return empty_list_;
}

void TaskConfig::addFixedPerson(int64_t task_id, const FixedPersonInfo& fixed_person)
{
    task_id_to_fixed_persons_[task_id].push_back(fixed_person);
}

void TaskConfig::addFixedPersonByType(TaskType task_type, const FixedPersonInfo& fixed_person)
{
    int64_t task_id = static_cast<int64_t>(task_type);
    
    // 如果类型还没有映射到ID，创建映射
    if (task_type_to_id_.find(task_type) == task_type_to_id_.end()) {
        task_type_to_id_[task_type] = task_id;
    } else {
        // 如果已有映射，使用已存在的ID
        task_id = task_type_to_id_[task_type];
    }
    
    addFixedPerson(task_id, fixed_person);
}

void TaskConfig::setTaskTypeToId(TaskType task_type, int64_t task_id)
{
    task_type_to_id_[task_type] = task_id;
}

int64_t TaskConfig::getTaskIdByType(TaskType task_type) const
{
    auto it = task_type_to_id_.find(task_type);
    if (it != task_type_to_id_.end()) {
        return it->second;
    }
    return 0;
}

bool TaskConfig::hasFixedPersonConfig(int64_t task_id) const
{
    return task_id_to_fixed_persons_.find(task_id) != task_id_to_fixed_persons_.end();
}

bool TaskConfig::hasFixedPersonConfigByType(TaskType task_type) const
{
    auto type_it = task_type_to_id_.find(task_type);
    if (type_it != task_type_to_id_.end()) {
        return hasFixedPersonConfig(type_it->second);
    }
    return false;
}

void TaskConfig::clear()
{
    task_id_to_fixed_persons_.clear();
    task_type_to_id_.clear();
    task_type_to_priority_.clear();
}

int32_t TaskConfig::getTaskPriority(TaskType task_type) const
{
    auto it = task_type_to_priority_.find(task_type);
    if (it != task_type_to_priority_.end()) {
        return it->second;
    }
    return 0;  // 默认优先级为0
}

void TaskConfig::setTaskPriority(TaskType task_type, int32_t priority)
{
    task_type_to_priority_[task_type] = priority;
}

void TaskConfig::initializeTaskPriorities()
{
    // 优先级顺序：调度类 > 外场 > 厅内 > 前台协助
    // 数值越大优先级越高
    
    // 调度类：优先级100（最高）
    setTaskPriority(TaskType::DISPATCH, 100);

    // 其他前台任务：优先级与调度相同
    setTaskPriority(TaskType::DOMESTIC_FRONT_DESK, 100);
    setTaskPriority(TaskType::DOMESTIC_FRONT_DESK_EARLY, 100);
    setTaskPriority(TaskType::INTERNATIONAL_FRONT_DESK_EARLY, 100);
    setTaskPriority(TaskType::INTERNATIONAL_FRONT_DESK_LATE, 100);
    
    // 外场类：优先级80
    setTaskPriority(TaskType::EXTERNAL_DOMESTIC_DEPARTURE_FEW, 80);
    setTaskPriority(TaskType::EXTERNAL_DOMESTIC_DEPARTURE_MANY, 80);
    setTaskPriority(TaskType::EXTERNAL_DOMESTIC_ARRIVAL_FEW, 80);
    setTaskPriority(TaskType::EXTERNAL_DOMESTIC_ARRIVAL_MANY, 80);
    setTaskPriority(TaskType::EXTERNAL_INTERNATIONAL_DEPARTURE_FEW, 80);
    setTaskPriority(TaskType::EXTERNAL_INTERNATIONAL_DEPARTURE_MANY, 80);
    setTaskPriority(TaskType::EXTERNAL_INTERNATIONAL_ARRIVAL_FEW, 80);
    setTaskPriority(TaskType::EXTERNAL_INTERNATIONAL_ARRIVAL_MANY, 80);
    
    // 厅内类：优先级60
    setTaskPriority(TaskType::INTERNATIONAL_HALL_EARLY, 60);
    setTaskPriority(TaskType::INTERNATIONAL_HALL_LATE, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_EARLY, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_0830_0930, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_0930_1030, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1030_1130, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1130_1230, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1230_1330, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1330_1430, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1430_1530, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1530_1630, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1630_1730, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1730_1830, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1830_1930, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_1930_2030, 60);
    setTaskPriority(TaskType::DOMESTIC_HALL_2030_AFTER, 60);
    
    // 前台协助类：优先级40（最低）
    setTaskPriority(TaskType::DOMESTIC_FRONT_DESK_ASSIST, 40);
    setTaskPriority(TaskType::DOMESTIC_FRONT_DESK_ASSIST2, 40);
    
    // 操作间类：优先级30
    setTaskPriority(TaskType::OPERATION_ROOM, 30);
}

void TaskConfig::setHallMaintenanceFixedPersons(const vector<Shift>& shifts, 
                                                const vector<TaskDefinition>& all_tasks)
{
    hall_maintenance_fixed_persons_.clear();
    
    // 收集所有有固定任务的员工ID
    set<string> employees_with_fixed_tasks;
    for (const auto& task : all_tasks) {
        const auto& fixed_persons = getFixedPersonsByType(task.getTaskType());
        if (!fixed_persons.empty()) {
            for (const auto& fixed_info : fixed_persons) {
                for (const auto& shift : shifts) {
                    int32_t shift_type = shift.getShiftType();
                    if (shift_type == 0) continue;
                    
                    ShiftCategory category = (shift_type == 1) ? ShiftCategory::MAIN : ShiftCategory::SUB;
                    if (fixed_info.shift_category == category) {
                        string employee_id = shift.getEmployeeIdAtPosition(fixed_info.position);
                        if (!employee_id.empty()) {
                            employees_with_fixed_tasks.insert(employee_id);
                        }
                    }
                }
            }
        }
    }
    
    // 收集所有主班员工（没有固定任务的）
    vector<string> main_shift_candidates;
    for (const auto& shift : shifts) {
        if (shift.getShiftType() == 1) {  // 主班
            const auto& position_map = shift.getPositionToEmployeeId();
            for (const auto& pos_pair : position_map) {
                const string& employee_id = pos_pair.second;
                if (employees_with_fixed_tasks.find(employee_id) == employees_with_fixed_tasks.end()) {
                    main_shift_candidates.push_back(employee_id);
                }
            }
        }
    }
    
    // 收集所有副班员工（没有固定任务的）
    vector<string> sub_shift_candidates_no_fixed;
    vector<string> sub_shift_candidates_with_fixed;
    for (const auto& shift : shifts) {
        if (shift.getShiftType() == 2) {  // 副班
            const auto& position_map = shift.getPositionToEmployeeId();
            for (const auto& pos_pair : position_map) {
                const string& employee_id = pos_pair.second;
                if (employees_with_fixed_tasks.find(employee_id) == employees_with_fixed_tasks.end()) {
                    sub_shift_candidates_no_fixed.push_back(employee_id);
                } else {
                    sub_shift_candidates_with_fixed.push_back(employee_id);
                }
            }
        }
    }
    
    // 按照优先级选择：优先主班，主班不足时从副班没有固定任务的人选中选，否则选副班有固定任务的人
    // 选择4个人
    for (size_t i = 0; i < 4 && i < main_shift_candidates.size(); ++i) {
        hall_maintenance_fixed_persons_.push_back(main_shift_candidates[i]);
    }
    
    // 如果主班不足，从副班没有固定任务的人选中选
    size_t remaining = 4 - hall_maintenance_fixed_persons_.size();
    for (size_t i = 0; i < remaining && i < sub_shift_candidates_no_fixed.size(); ++i) {
        hall_maintenance_fixed_persons_.push_back(sub_shift_candidates_no_fixed[i]);
    }
    
    // 如果还不够，从副班有固定任务的人选中选
    remaining = 4 - hall_maintenance_fixed_persons_.size();
    for (size_t i = 0; i < remaining && i < sub_shift_candidates_with_fixed.size(); ++i) {
        hall_maintenance_fixed_persons_.push_back(sub_shift_candidates_with_fixed[i]);
    }
}

const vector<string>& TaskConfig::getHallMaintenanceFixedPersons() const
{
    return hall_maintenance_fixed_persons_;
}

}  // namespace vip_first_class

