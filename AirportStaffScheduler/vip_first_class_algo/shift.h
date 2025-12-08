/**
 * @file shift.h
 * @brief 班次类
 * 
 * 定义排班系统中的班次信息
 */

#ifndef VIP_FIRST_CLASS_SHIFT_H
#define VIP_FIRST_CLASS_SHIFT_H

#include <string>
#include <map>
#include <cstdint>

namespace vip_first_class {

/**
 * @brief 班次类型枚举
 */
enum class ShiftType {
    REST = 0,    ///< 休息
    MAIN = 1,    ///< 主班
    SUB = 2      ///< 副班
};

/**
 * @brief 班次类
 * 
 * 记录班次类型以及位置到人员ID的映射
 */
class Shift {
public:
    /**
     * @brief 构造函数
     */
    Shift();
    
    /**
     * @brief 析构函数
     */
    ~Shift();
    
    /**
     * @brief 获取班次类型
     * @return 班次类型（0为休息，1为主班，2为副班）
     */
    int32_t getShiftType() const { return shift_type_; }
    
    /**
     * @brief 设置班次类型
     * @param type 班次类型（0为休息，1为主班，2为副班）
     */
    void setShiftType(int32_t type) { shift_type_ = type; }
    
    /**
     * @brief 设置班次类型（使用枚举）
     * @param type 班次类型枚举值
     */
    void setShiftTypeEnum(ShiftType type) { shift_type_ = static_cast<int32_t>(type); }
    
    /**
     * @brief 获取位置到人员ID的映射
     * @return 位置到人员ID映射的常量引用（键为位置编号，值为人员ID）
     */
    const std::map<int32_t, std::string>& getPositionToEmployeeId() const { return position_to_employee_id_; }
    
    /**
     * @brief 获取位置到人员ID的映射（非常量版本）
     * @return 位置到人员ID映射的引用
     */
    std::map<int32_t, std::string>& getPositionToEmployeeId() { return position_to_employee_id_; }
    
    /**
     * @brief 设置指定位置的人员ID
     * @param position 位置编号（如主班1为1，副班2为2等）
     * @param employee_id 人员ID
     */
    void setEmployeeIdAtPosition(int32_t position, const std::string& employee_id) {
        position_to_employee_id_[position] = employee_id;
    }
    
    /**
     * @brief 获取指定位置的人员ID
     * @param position 位置编号
     * @return 人员ID，如果该位置没有分配人员则返回空字符串
     */
    std::string getEmployeeIdAtPosition(int32_t position) const {
        auto it = position_to_employee_id_.find(position);
        return (it != position_to_employee_id_.end()) ? it->second : "";
    }
    
    /**
     * @brief 移除指定位置的人员分配
     * @param position 位置编号
     */
    void removeEmployeeAtPosition(int32_t position) {
        position_to_employee_id_.erase(position);
    }
    
    /**
     * @brief 检查指定位置是否已分配人员
     * @param position 位置编号
     * @return true表示已分配，false表示未分配
     */
    bool hasEmployeeAtPosition(int32_t position) const {
        return position_to_employee_id_.find(position) != position_to_employee_id_.end();
    }
    
    /**
     * @brief 清除所有人员分配
     */
    void clearAllAssignments() {
        position_to_employee_id_.clear();
    }
    
    /**
     * @brief 获取已分配位置的数量
     * @return 已分配位置的数量
     */
    size_t getAssignedPositionCount() const {
        return position_to_employee_id_.size();
    }

private:
    int32_t shift_type_;                                      ///< 班次类型（0为休息，1为主班，2为副班）
    std::map<int32_t, std::string> position_to_employee_id_; ///< 位置到人员ID的映射（键为位置编号，值为人员ID）
};

}  // namespace vip_first_class

#endif  // VIP_FIRST_CLASS_SHIFT_H

