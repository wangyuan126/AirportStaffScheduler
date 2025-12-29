/**
 * @file load_task.cpp
 * @brief 装卸任务类实现
 */

#include "load_task.h"

namespace zhuangxie_class {

using namespace std;

const long DEFAULT_TRAVEL_TIME = 8 * 60;  // 默认8分钟 = 480秒

LoadTask::LoadTask()
    : task_id_("")
    , task_name_("")
    , prefer_main_shift_(false)
    , earliest_start_time_(0)
    , latest_end_time_(0)
    , actual_start_time_(0)
    , duration_(0)
    , flight_type_(0)
    , arrival_time_(0)
    , departure_time_(0)
    , travel_time_(DEFAULT_TRAVEL_TIME)
    , is_remote_stand_(false)
    , stand_(0)
    , task_date_("")
    , arrival_flight_id_("")
    , departure_flight_id_("")
    , arrival_flight_number_("")
    , departure_flight_number_("")
    , terminal_("")
    , required_qualification_(0)
    , can_new_employee_(false)
    , required_count_(0)
    , is_assigned_(false)
    , is_short_staffed_(false)
{
}

LoadTask::~LoadTask()
{
}

void LoadTask::addAssignedEmployeeId(const string& employee_id)
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

void LoadTask::removeAssignedEmployeeId(const string& employee_id)
{
    auto it = assigned_employee_ids_.begin();
    while (it != assigned_employee_ids_.end()) {
        if (*it == employee_id) {
            assigned_employee_ids_.erase(it);
            break;
        }
        ++it;
    }
    is_assigned_ = !assigned_employee_ids_.empty();
}

bool LoadTask::isAssignedToEmployee(const string& employee_id) const
{
    for (const auto& id : assigned_employee_ids_) {
        if (id == employee_id) {
            return true;
        }
    }
    return false;
}

void LoadTask::clearAssignedEmployees()
{
    assigned_employee_ids_.clear();
    is_assigned_ = false;
}

}  // namespace zhuangxie_class

