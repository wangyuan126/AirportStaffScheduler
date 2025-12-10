/**
 * @file employee_manager.h
 * @brief 员工信息管理器（单例模式）
 * 
 * 管理所有员工信息的全局单例类
 */

#ifndef VIP_FIRST_CLASS_EMPLOYEE_MANAGER_H
#define VIP_FIRST_CLASS_EMPLOYEE_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "employee_info.h"

namespace vip_first_class {

using namespace std;

/**
 * @brief 员工信息管理器（单例模式）
 * 
 * 管理所有员工信息的全局访问点
 */
class EmployeeManager {
public:
    /**
     * @brief 获取单例实例
     * @return EmployeeManager单例引用
     */
    static EmployeeManager& getInstance();
    
    /**
     * @brief 添加或更新员工信息
     * @param employee_id 员工ID
     * @param employee_info 员工信息对象
     */
    void addOrUpdateEmployee(const string& employee_id, const EmployeeInfo& employee_info);
    
    /**
     * @brief 获取员工信息
     * @param employee_id 员工ID
     * @return 员工信息指针，如果不存在则返回nullptr
     */
    EmployeeInfo* getEmployee(const string& employee_id);
    
    /**
     * @brief 获取员工信息（常量版本）
     * @param employee_id 员工ID
     * @return 员工信息常量指针，如果不存在则返回nullptr
     */
    const EmployeeInfo* getEmployee(const string& employee_id) const;
    
    /**
     * @brief 检查员工是否存在
     * @param employee_id 员工ID
     * @return true表示存在，false表示不存在
     */
    bool hasEmployee(const string& employee_id) const;
    
    /**
     * @brief 删除员工信息
     * @param employee_id 员工ID
     * @return true表示删除成功，false表示员工不存在
     */
    bool removeEmployee(const string& employee_id);
    
    /**
     * @brief 获取所有员工ID列表
     * @return 员工ID列表
     */
    vector<string> getAllEmployeeIds() const;
    
    /**
     * @brief 获取员工总数
     * @return 员工总数
     */
    size_t getEmployeeCount() const;
    
    /**
     * @brief 清除所有员工信息
     */
    void clearAllEmployees();
    
    /**
     * @brief 获取所有员工信息的映射
     * @return 员工ID到员工信息的映射的常量引用
     */
    const map<string, EmployeeInfo>& getAllEmployees() const {
        return employees_;
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    EmployeeManager();
    
    /**
     * @brief 私有析构函数
     */
    ~EmployeeManager();
    
    /**
     * @brief 禁止拷贝构造
     */
    EmployeeManager(const EmployeeManager&) = delete;
    
    /**
     * @brief 禁止赋值操作
     */
    EmployeeManager& operator=(const EmployeeManager&) = delete;
    
    // 员工ID到员工信息的映射
    map<string, EmployeeInfo> employees_;
};

}  // namespace vip_first_class

#endif  // VIP_FIRST_CLASS_EMPLOYEE_MANAGER_H

