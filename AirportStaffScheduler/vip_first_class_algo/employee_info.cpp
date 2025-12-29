/**
 * @file employee_info.cpp
 * @brief 员工信息类实现
 */

#include "employee_info.h"
#include <algorithm>

namespace vip_first_class {

EmployeeInfo::EmployeeInfo()
    : employee_id_("")
    , employee_name_("")
    , qualification_mask_(0)
    , total_work_time_(0)
{
}

EmployeeInfo::~EmployeeInfo()
{
}

void EmployeeInfo::addAssignedTaskId(const string& task_id)
{
    // 检查是否已存在
    for (const string& id : assigned_task_ids_) {
        if (id == task_id) {
            return;  // 已经存在，不重复添加
        }
    }
    assigned_task_ids_.push_back(task_id);
}

bool EmployeeInfo::removeAssignedTaskId(const string& task_id)
{
    auto it = std::find(assigned_task_ids_.begin(), assigned_task_ids_.end(), task_id);
    if (it != assigned_task_ids_.end()) {
        assigned_task_ids_.erase(it);
        return true;
    }
    return false;
}

bool EmployeeInfo::isAssignedToTask(const string& task_id) const
{
    return std::find(assigned_task_ids_.begin(), assigned_task_ids_.end(), task_id) 
           != assigned_task_ids_.end();
}

}  // namespace vip_first_class

