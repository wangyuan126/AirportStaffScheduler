/**
 * @file employee_info.h
 * @brief 员工信息类
 * 
 * 定义排班系统中的员工详细信息和统计数据
 */

#ifndef VIP_FIRST_CLASS_EMPLOYEE_INFO_H
#define VIP_FIRST_CLASS_EMPLOYEE_INFO_H

#include <string>
#include <vector>
#include <map>

namespace vip_first_class {

using namespace std;

/**
 * @brief 资质类型枚举（位掩码）
 */
enum class QualificationMask {
    HALL_INTERNAL = 1,    ///< 厅内资质
    EXTERNAL = 2,         ///< 外场资质
    FRONT_DESK = 4,       ///< 前台资质
    DISPATCH = 8          ///< 调度资质
};

/**
 * @brief 员工信息类
 * 
 * 记录员工的资质、工作时长和统计信息
 */
class EmployeeInfo {
public:
    /**
     * @brief 构造函数
     */
    EmployeeInfo();
    
    /**
     * @brief 析构函数
     */
    ~EmployeeInfo();
    
    /**
     * @brief 获取员工ID
     * @return 员工ID
     */
    const string& getEmployeeId() const { return employee_id_; }
    
    /**
     * @brief 设置员工ID
     * @param id 员工ID
     */
    void setEmployeeId(const string& id) { employee_id_ = id; }
    
    /**
     * @brief 获取员工姓名
     * @return 员工姓名
     */
    const string& getEmployeeName() const { return employee_name_; }
    
    /**
     * @brief 设置员工姓名
     * @param name 员工姓名
     */
    void setEmployeeName(const string& name) { employee_name_ = name; }
    
    /**
     * @brief 获取资质掩码
     * @return 资质掩码（位掩码，参见QualificationMask）
     */
    int getQualificationMask() const { return qualification_mask_; }
    
    /**
     * @brief 设置资质掩码
     * @param mask 资质掩码（位掩码）
     */
    void setQualificationMask(int mask) { qualification_mask_ = mask; }
    
    /**
     * @brief 添加资质
     * @param qual 资质类型（QualificationMask枚举值）
     */
    void addQualification(QualificationMask qual) {
        qualification_mask_ |= static_cast<int>(qual);
    }
    
    /**
     * @brief 移除资质
     * @param qual 资质类型（QualificationMask枚举值）
     */
    void removeQualification(QualificationMask qual) {
        qualification_mask_ &= ~static_cast<int>(qual);
    }
    
    /**
     * @brief 检查是否具有某种资质
     * @param qual 资质类型（QualificationMask枚举值）
     * @return true表示具有该资质，false表示不具有
     */
    bool hasQualification(QualificationMask qual) const {
        return (qualification_mask_ & static_cast<int>(qual)) != 0;
    }
    
    /**
     * @brief 获取累计工作时长
     * @return 累计工作时长（秒）
     */
    long getTotalWorkTime() const { return total_work_time_; }
    
    /**
     * @brief 设置累计工作时长
     * @param time 累计工作时长（秒）
     */
    void setTotalWorkTime(long time) { total_work_time_ = time; }
    
    /**
     * @brief 增加累计工作时长
     * @param time 增加的工作时长（秒）
     */
    void addWorkTime(long time) { total_work_time_ += time; }
    
    /**
     * @brief 获取班次类型次数统计
     * @return 班次类型次数统计的常量引用（键为"主班1"、"主班2"、"副班1"等，值为次数）
     */
    const map<string, int>& getShiftTypeCounts() const { return shift_type_counts_; }
    
    /**
     * @brief 获取班次类型次数统计（非常量版本）
     * @return 班次类型次数统计的引用
     */
    map<string, int>& getShiftTypeCounts() { return shift_type_counts_; }
    
    /**
     * @brief 增加班次类型次数
     * @param shift_type 班次类型（如"主班1"、"主班2"、"副班1"等）
     * @param count 增加的次数（默认为1）
     */
    void addShiftTypeCount(const string& shift_type, int count = 1) {
        shift_type_counts_[shift_type] += count;
    }
    
    /**
     * @brief 获取班次类型的次数
     * @param shift_type 班次类型（如"主班1"、"主班2"、"副班1"等）
     * @return 该班次类型的次数
     */
    int getShiftTypeCount(const string& shift_type) const {
        auto it = shift_type_counts_.find(shift_type);
        return (it != shift_type_counts_.end()) ? it->second : 0;
    }
    
    /**
     * @brief 获取厅房任务次数统计
     * @return 厅房任务次数统计的常量引用（键为厅房名称，值为次数）
     */
    const map<string, int>& getHallTaskCounts() const { return hall_task_counts_; }
    
    /**
     * @brief 获取厅房任务次数统计（非常量版本）
     * @return 厅房任务次数统计的引用
     */
    map<string, int>& getHallTaskCounts() { return hall_task_counts_; }
    
    /**
     * @brief 增加厅房任务次数
     * @param hall_name 厅房名称（如"国内厅内"、"国际厅内"等）
     * @param count 增加的次数（默认为1）
     */
    void addHallTaskCount(const string& hall_name, int count = 1) {
        hall_task_counts_[hall_name] += count;
    }
    
    /**
     * @brief 获取厅房任务的次数
     * @param hall_name 厅房名称（如"国内厅内"、"国际厅内"等）
     * @return 该厅房的任务次数
     */
    int getHallTaskCount(const string& hall_name) const {
        auto it = hall_task_counts_.find(hall_name);
        return (it != hall_task_counts_.end()) ? it->second : 0;
    }
    
    /**
     * @brief 重置所有统计数据
     */
    void resetStatistics() {
        shift_type_counts_.clear();
        hall_task_counts_.clear();
        total_work_time_ = 0;
    }
    
    /**
     * @brief 获取分配的任务ID列表
     * @return 任务ID列表的常量引用
     */
    const vector<long>& getAssignedTaskIds() const { return assigned_task_ids_; }
    
    /**
     * @brief 获取分配的任务ID列表（非常量版本）
     * @return 任务ID列表的引用
     */
    vector<long>& getAssignedTaskIds() { return assigned_task_ids_; }
    
    /**
     * @brief 添加分配的任务ID
     * @param task_id 任务ID
     */
    void addAssignedTaskId(long task_id);
    
    /**
     * @brief 移除分配的任务ID
     * @param task_id 任务ID
     * @return true表示移除成功，false表示该任务未分配给此员工
     */
    bool removeAssignedTaskId(long task_id);
    
    /**
     * @brief 检查是否已分配指定任务
     * @param task_id 任务ID
     * @return true表示已分配，false表示未分配
     */
    bool isAssignedToTask(long task_id) const;
    
    /**
     * @brief 获取分配的任务数量
     * @return 分配的任务数量
     */
    size_t getAssignedTaskCount() const { return assigned_task_ids_.size(); }
    
    /**
     * @brief 清除所有任务分配
     */
    void clearAssignedTasks() {
        assigned_task_ids_.clear();
    }

private:
    string employee_id_;                                 ///< 员工ID
    string employee_name_;                               ///< 员工姓名
    int qualification_mask_;                              ///< 资质掩码（位掩码）
    long total_work_time_;                                 ///< 累计工作时长（秒）
    map<string, int> shift_type_counts_;       ///< 班次类型次数统计（键为"主班1"、"主班2"、"副班1"等）
    map<string, int> hall_task_counts_;        ///< 厅房任务次数统计（键为厅房名称）
    vector<long> assigned_task_ids_;                 ///< 分配的任务ID列表
};

}  // namespace vip_first_class

#endif  // VIP_FIRST_CLASS_EMPLOYEE_INFO_H

