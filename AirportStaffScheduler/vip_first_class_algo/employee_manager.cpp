/**
 * @file employee_manager.cpp
 * @brief 员工信息管理器实现
 */

#include "employee_manager.h"
#include <algorithm>

namespace vip_first_class {

EmployeeManager& EmployeeManager::getInstance()
{
    static EmployeeManager instance;
    return instance;
}

EmployeeManager::EmployeeManager()
{
}

EmployeeManager::~EmployeeManager()
{
}

void EmployeeManager::addOrUpdateEmployee(const std::string& employee_id, const EmployeeInfo& employee_info)
{
    employees_[employee_id] = employee_info;
}

EmployeeInfo* EmployeeManager::getEmployee(const std::string& employee_id)
{
    auto it = employees_.find(employee_id);
    if (it != employees_.end()) {
        return &(it->second);
    }
    return nullptr;
}

const EmployeeInfo* EmployeeManager::getEmployee(const std::string& employee_id) const
{
    auto it = employees_.find(employee_id);
    if (it != employees_.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool EmployeeManager::hasEmployee(const std::string& employee_id) const
{
    return employees_.find(employee_id) != employees_.end();
}

bool EmployeeManager::removeEmployee(const std::string& employee_id)
{
    auto it = employees_.find(employee_id);
    if (it != employees_.end()) {
        employees_.erase(it);
        return true;
    }
    return false;
}

std::vector<std::string> EmployeeManager::getAllEmployeeIds() const
{
    std::vector<std::string> ids;
    ids.reserve(employees_.size());
    for (const auto& pair : employees_) {
        ids.push_back(pair.first);
    }
    return ids;
}

size_t EmployeeManager::getEmployeeCount() const
{
    return employees_.size();
}

void EmployeeManager::clearAllEmployees()
{
    employees_.clear();
}

}  // namespace vip_first_class

