/**
 * @file task_definition.cpp
 * @brief 任务定义类实现
 */

#include "task_definition.h"
#include "task_config.h"
#include <algorithm>

namespace vip_first_class {

TaskDefinition::TaskDefinition()
    : task_id_(0)
    , task_name_("")
    , task_type_(TaskType::DISPATCH)
    , prefer_main_shift_(false)
    , start_time_(0)
    , end_time_(0)
    , required_qualification_(0)
    , can_new_employee_(false)
    , allow_overlap_(false)
    , max_overlap_time_(0)
    , required_count_(1)
    , is_assigned_(false)
    , is_short_staffed_(false)
{
}

TaskDefinition::~TaskDefinition()
{
}

void TaskDefinition::addAssignedEmployeeId(const std::string& employee_id)
{
    // 检查是否已存在
    for (const auto& id : assigned_employee_ids_) {
        if (id == employee_id) {
            return;  // 已经存在，不重复添加
        }
    }
    assigned_employee_ids_.push_back(employee_id);
    is_assigned_ = !assigned_employee_ids_.empty();
}

bool TaskDefinition::removeAssignedEmployeeId(const std::string& employee_id, 
                                               const std::vector<Shift>& shifts)
{
    // 先检查是否是固定人选
    if (isFixedPerson(employee_id, shifts)) {
        return false;  // 是固定人选，不能移除，返回false
    }
    
    // 不是固定人选，执行移除操作
    auto it = std::find(assigned_employee_ids_.begin(), assigned_employee_ids_.end(), employee_id);
    if (it != assigned_employee_ids_.end()) {
        assigned_employee_ids_.erase(it);
        is_assigned_ = !assigned_employee_ids_.empty();
        return true;
    }
    return false;  // 未分配到此任务
}

bool TaskDefinition::isFixedPerson(const std::string& employee_id, 
                                    const std::vector<Shift>& shifts) const
{
    // 获取任务的固定人选配置
    const auto& fixed_persons = TaskConfig::getInstance().getFixedPersonsByType(task_type_);
    if (fixed_persons.empty()) {
        return false;  // 没有固定人选配置
    }
    
    // 在班次列表中找到该员工所在的班次
    for (const auto& shift : shifts) {
        const auto& position_map = shift.getPositionToEmployeeId();
        
        // 遍历该班次的所有位置，查找员工
        for (const auto& pos_pair : position_map) {
            if (pos_pair.second == employee_id) {
                int32_t position = pos_pair.first;
                int32_t shift_type = shift.getShiftType();
                
                // 将ShiftType转换为ShiftCategory
                ShiftCategory category;
                if (shift_type == 1) {  // MAIN
                    category = ShiftCategory::MAIN;
                } else if (shift_type == 2) {  // SUB
                    category = ShiftCategory::SUB;
                } else {
                    continue;  // REST类型不考虑
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
    
    return false;  // 不是固定人选
}

bool TaskDefinition::isAssignedToEmployee(const std::string& employee_id) const
{
    return std::find(assigned_employee_ids_.begin(), assigned_employee_ids_.end(), employee_id) 
           != assigned_employee_ids_.end();
}

}  // namespace vip_first_class

