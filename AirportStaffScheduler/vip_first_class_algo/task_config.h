/**
 * @file task_config.h
 * @brief 任务配置类
 * 
 * 定义任务的固定人选配置信息
 */

#ifndef VIP_FIRST_CLASS_TASK_CONFIG_H
#define VIP_FIRST_CLASS_TASK_CONFIG_H


#include <map>
#include <vector>
#include <cstdint>
#include <string>
#include "task_type.h"
namespace vip_first_class {

using namespace std;

// 前向声明
class Shift;
class TaskDefinition;

/**
 * @brief 班次类型枚举（用于固定人选）
 */
enum class ShiftCategory {
    MAIN = 0,   ///< 主班
    SUB = 1     ///< 副班
};

/**
 * @brief 固定人选信息结构
 */
struct FixedPersonInfo {
    ShiftCategory shift_category;  ///< 班次类型（主班/副班）
    int32_t position;              ///< 固定人选的位置（第几个人，从1开始）
    
    FixedPersonInfo() : shift_category(ShiftCategory::MAIN), position(0) {}
    FixedPersonInfo(ShiftCategory category, int32_t pos) 
        : shift_category(category), position(pos) {}
};

/**
 * @brief 任务配置类（单例模式）
 * 
 * 管理任务的固定人选配置
 */
class TaskConfig {
public:
    /**
     * @brief 获取单例实例
     * @return TaskConfig单例引用
     */
    static TaskConfig& getInstance();
    
    /**
     * @brief 初始化默认配置
     * 
     * 根据业务需求初始化任务固定人选配置
     */
    void initializeDefaultConfig();
    
    /**
     * @brief 根据任务ID获取固定人选列表
     * @param task_id 任务ID
     * @return 固定人选信息列表的常量引用
     */
    const vector<FixedPersonInfo>& getFixedPersons(int64_t task_id) const;
    
    /**
     * @brief 根据任务类型获取固定人选列表
     * @param task_type 任务类型
     * @return 固定人选信息列表的常量引用
     */
    const vector<FixedPersonInfo>& getFixedPersonsByType(TaskType task_type) const;
    
    /**
     * @brief 添加任务固定人选配置
     * @param task_id 任务ID
     * @param fixed_person 固定人选信息
     */
    void addFixedPerson(int64_t task_id, const FixedPersonInfo& fixed_person);
    
    /**
     * @brief 添加任务固定人选配置（通过任务类型）
     * @param task_type 任务类型
     * @param fixed_person 固定人选信息
     */
    void addFixedPersonByType(TaskType task_type, const FixedPersonInfo& fixed_person);
    
    /**
     * @brief 设置任务类型到任务ID的映射
     * @param task_type 任务类型
     * @param task_id 任务ID
     */
    void setTaskTypeToId(TaskType task_type, int64_t task_id);
    
    /**
     * @brief 根据任务类型获取任务ID
     * @param task_type 任务类型
     * @return 任务ID，如果不存在则返回0
     */
    int64_t getTaskIdByType(TaskType task_type) const;
    
    /**
     * @brief 检查任务是否有固定人选配置
     * @param task_id 任务ID
     * @return true表示有配置，false表示没有配置
     */
    bool hasFixedPersonConfig(int64_t task_id) const;
    
    /**
     * @brief 检查任务是否有固定人选配置（通过任务类型）
     * @param task_type 任务类型
     * @return true表示有配置，false表示没有配置
     */
    bool hasFixedPersonConfigByType(TaskType task_type) const;
    
    /**
     * @brief 清除所有配置
     */
    void clear();
    
    /**
     * @brief 获取任务优先级
     * @param task_type 任务类型
     * @return 任务优先级，数值越大优先级越高
     */
    int32_t getTaskPriority(TaskType task_type) const;
    
    /**
     * @brief 设置任务优先级
     * @param task_type 任务类型
     * @param priority 优先级，数值越大优先级越高
     */
    void setTaskPriority(TaskType task_type, int32_t priority);
    
    /**
     * @brief 初始化任务优先级配置
     * 
     * 根据业务需求初始化所有任务的优先级
     * 优先级顺序：调度类 > 外场 > 厅内 > 前台协助
     */
    void initializeTaskPriorities();
    
    /**
     * @brief 动态设定厅内保障任务的4个固定人选
     * 
     * 逻辑：非有固定任务的人，优先主班，如果有主班不足，则从副班没有固定任务的人选中选，否则选副班有固定任务的人
     * @param shifts 班次列表，用于查找员工
     * @param all_tasks 所有任务列表，用于判断员工是否有固定任务
     */
    void setHallMaintenanceFixedPersons(const vector<Shift>& shifts, 
                                        const vector<TaskDefinition>& all_tasks);
    
    /**
     * @brief 获取厅内保障任务的固定人选列表
     * @return 厅内保障任务固定人选的员工ID列表
     */
    const vector<string>& getHallMaintenanceFixedPersons() const;

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    TaskConfig();
    
    /**
     * @brief 私有析构函数
     */
    ~TaskConfig();
    
    /**
     * @brief 禁止拷贝构造
     */
    TaskConfig(const TaskConfig&) = delete;
    
    /**
     * @brief 禁止赋值操作
     */
    TaskConfig& operator=(const TaskConfig&) = delete;
    
    // 任务ID到固定人选列表的映射
    map<int64_t, vector<FixedPersonInfo>> task_id_to_fixed_persons_;
    
    // 任务类型到任务ID的映射（用于通过类型查找）
    map<TaskType, int64_t> task_type_to_id_;
    
    // 任务类型到优先级的映射（数值越大优先级越高）
    map<TaskType, int32_t> task_type_to_priority_;
    
    // 空的固定人选列表（用于返回默认值）
    static const vector<FixedPersonInfo> empty_list_;
    
    // 厅内保障任务的固定人选列表（4个人）
    vector<string> hall_maintenance_fixed_persons_;
    
    // 空的员工ID列表（用于返回默认值）
    static const vector<string> empty_employee_list_;
};

}  // namespace vip_first_class

#endif  // VIP_FIRST_CLASS_TASK_CONFIG_H

