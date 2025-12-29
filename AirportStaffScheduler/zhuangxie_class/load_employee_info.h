/**
 * @file load_employee_info.h
 * @brief 装卸员工信息类
 * 
 * 基于vip_first_class的EmployeeInfo，添加装卸组字段
 */

#ifndef ZHUANGXIE_CLASS_LOAD_EMPLOYEE_INFO_H
#define ZHUANGXIE_CLASS_LOAD_EMPLOYEE_INFO_H

#include "../vip_first_class_algo/employee_info.h"
#include <string>

namespace zhuangxie_class {

using namespace std;

/**
 * @brief 装卸员工信息类
 * 
 * 继承或组合vip_first_class::EmployeeInfo，添加装卸组字段
 */
class LoadEmployeeInfo {
public:
    /**
     * @brief 构造函数
     */
    LoadEmployeeInfo();
    
    /**
     * @brief 析构函数
     */
    ~LoadEmployeeInfo();
    
    /**
     * @brief 获取员工信息对象（引用）
     * @return EmployeeInfo的引用
     */
    vip_first_class::EmployeeInfo& getEmployeeInfo() { return employee_info_; }
    
    /**
     * @brief 获取员工信息对象（常量引用）
     * @return EmployeeInfo的常量引用
     */
    const vip_first_class::EmployeeInfo& getEmployeeInfo() const { return employee_info_; }
    
    /**
     * @brief 获取装卸组ID
     * @return 装卸组ID（只有装卸员工使用）
     */
    int getLoadGroup() const { return load_group_; }
    
    /**
     * @brief 设置装卸组ID
     * @param group 装卸组ID
     */
    void setLoadGroup(int group) { load_group_ = group; }
    
    // 便捷方法：直接访问EmployeeInfo的常用方法
    const string& getEmployeeId() const { return employee_info_.getEmployeeId(); }
    void setEmployeeId(const string& id) { employee_info_.setEmployeeId(id); }
    
    const string& getEmployeeName() const { return employee_info_.getEmployeeName(); }
    void setEmployeeName(const string& name) { employee_info_.setEmployeeName(name); }
    
    int getQualificationMask() const { return employee_info_.getQualificationMask(); }
    void setQualificationMask(int mask) { employee_info_.setQualificationMask(mask); }

private:
    vip_first_class::EmployeeInfo employee_info_;  ///< 员工信息对象（引用vip_first_class）
    int load_group_;                          ///< 装卸组ID（只有装卸员工使用）
};

}  // namespace zhuangxie_class

#endif  // ZHUANGXIE_CLASS_LOAD_EMPLOYEE_INFO_H

