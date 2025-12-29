/**
 * @file load_task.h
 * @brief 装卸任务类
 * 
 * 集成航班信息和任务定义，用于装卸任务调度
 */

#ifndef ZHUANGXIE_CLASS_LOAD_TASK_H
#define ZHUANGXIE_CLASS_LOAD_TASK_H

#include <string>
#include <vector>

namespace zhuangxie_class {

using namespace std;

/**
 * @brief 航班类型枚举
 */
enum class FlightType {
    DOMESTIC_ARRIVAL = 0,      ///< 国内进港
    DOMESTIC_DEPARTURE = 1,    ///< 国内出港
    DOMESTIC_TRANSIT = 2,      ///< 国内过站
    INTERNATIONAL_ARRIVAL = 3, ///< 国际进港
    INTERNATIONAL_DEPARTURE = 4,///< 国际出港
    INTERNATIONAL_TRANSIT = 5  ///< 国际过站
};

/**
 * @brief 装卸任务类
 * 
 * 集成航班信息和任务定义的所有字段
 */
class LoadTask {
public:
    /**
     * @brief 构造函数
     */
    LoadTask();
    
    /**
     * @brief 析构函数
     */
    ~LoadTask();
    
    // ===== 任务基本信息 =====
    
    /**
     * @brief 获取任务ID
     * @return 任务ID（字符串类型）
     */
    const string& getTaskId() const { return task_id_; }
    
    /**
     * @brief 设置任务ID
     * @param id 任务ID（字符串类型）
     */
    void setTaskId(const string& id) { task_id_ = id; }
    
    /**
     * @brief 获取任务名称
     * @return 任务名称
     */
    const string& getTaskName() const { return task_name_; }
    
    /**
     * @brief 设置任务名称
     * @param name 任务名称
     */
    void setTaskName(const string& name) { task_name_ = name; }
    
    /**
     * @brief 是否优先主班
     * @return true表示优先主班，false表示不优先主班
     */
    bool isPreferMainShift() const { return prefer_main_shift_; }
    
    /**
     * @brief 设置是否优先主班
     * @param prefer true表示优先主班，false表示不优先主班
     */
    void setPreferMainShift(bool prefer) { prefer_main_shift_ = prefer; }
    
    // ===== 任务时间信息 =====
    
    /**
     * @brief 获取任务最早开始时间
     * @return 任务最早开始时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    long getEarliestStartTime() const { return earliest_start_time_; }
    
    /**
     * @brief 设置任务最早开始时间
     * @param time 任务最早开始时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    void setEarliestStartTime(long time) { earliest_start_time_ = time; }
    
    /**
     * @brief 获取任务最晚结束时间
     * @return 任务最晚结束时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    long getLatestEndTime() const { return latest_end_time_; }
    
    /**
     * @brief 设置任务最晚结束时间
     * @param time 任务最晚结束时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    void setLatestEndTime(long time) { latest_end_time_ = time; }
    
    /**
     * @brief 获取任务实际开始时间（分配后设置）
     * @return 任务实际开始时间（从2020年1月1日0点0分0秒开始的秒数），0表示未分配
     */
    long getActualStartTime() const { return actual_start_time_; }
    
    /**
     * @brief 设置任务实际开始时间
     * @param time 任务实际开始时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    void setActualStartTime(long time) { actual_start_time_ = time; }
    
    /**
     * @brief 获取任务时长
     * @return 任务时长（秒）
     */
    long getDuration() const { return duration_; }
    
    /**
     * @brief 设置任务时长
     * @param duration 任务时长（秒）
     */
    void setDuration(long duration) { duration_ = duration; }
    
    /**
     * @brief 获取任务实际结束时间（实际开始时间 + 时长）
     * @return 任务实际结束时间（从2020年1月1日0点0分0秒开始的秒数），0表示未分配
     */
    long getActualEndTime() const { 
        return actual_start_time_ > 0 ? actual_start_time_ + duration_ : 0; 
    }
    
    // ===== 航班信息 =====
    
    /**
     * @brief 获取航班类型
     * @return 航班类型（0-5对应不同的航班类型）
     */
    int getFlightType() const { return flight_type_; }
    
    /**
     * @brief 设置航班类型
     * @param type 航班类型（0-5）
     */
    void setFlightType(int type) { flight_type_ = type; }
    
    /**
     * @brief 设置航班类型（使用枚举）
     * @param type 航班类型枚举值
     */
    void setFlightTypeEnum(FlightType type) { flight_type_ = static_cast<int>(type); }
    
    /**
     * @brief 获取进港时间
     * @return 进港时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    long getArrivalTime() const { return arrival_time_; }
    
    /**
     * @brief 设置进港时间
     * @param time 进港时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    void setArrivalTime(long time) { arrival_time_ = time; }
    
    /**
     * @brief 获取出港时间
     * @return 出港时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    long getDepartureTime() const { return departure_time_; }
    
    /**
     * @brief 设置出港时间
     * @param time 出港时间（从2020年1月1日0点0分0秒开始的秒数）
     */
    void setDepartureTime(long time) { departure_time_ = time; }
    
    /**
     * @brief 获取通勤时间
     * @return 通勤时间（秒，默认8分钟=480秒）
     */
    long getTravelTime() const { return travel_time_; }
    
    /**
     * @brief 设置通勤时间
     * @param time 通勤时间（秒）
     */
    void setTravelTime(long time) { travel_time_ = time; }
    
    /**
     * @brief 获取是否远机位
     * @return true表示远机位，false表示近机位
     */
    bool isRemoteStand() const { return is_remote_stand_; }
    
    /**
     * @brief 设置是否远机位
     * @param remote true表示远机位，false表示近机位
     */
    void setRemoteStand(bool remote) { is_remote_stand_ = remote; }
    
    
    /**
     * @brief 获取机位
     * @return 机位编号，0表示未分配机位
     */
    int getStand() const { return stand_; }
    
    /**
     * @brief 设置机位
     * @param stand 机位编号，0表示未分配机位
     */
    void setStand(int stand) { stand_ = stand; }
    
    /**
     * @brief 获取任务日期
     * @return 任务日期（字符串，格式：YYYY-MM-DD）
     */
    const string& getTaskDate() const { return task_date_; }
    
    /**
     * @brief 设置任务日期
     * @param date 任务日期（字符串，格式：YYYY-MM-DD）
     */
    void setTaskDate(const string& date) { task_date_ = date; }
    
    /**
     * @brief 获取到达航班ID
     * @return 到达航班ID
     */
    const string& getArrivalFlightId() const { return arrival_flight_id_; }
    
    /**
     * @brief 设置到达航班ID
     * @param id 到达航班ID
     */
    void setArrivalFlightId(const string& id) { arrival_flight_id_ = id; }
    
    /**
     * @brief 获取出发航班ID
     * @return 出发航班ID
     */
    const string& getDepartureFlightId() const { return departure_flight_id_; }
    
    /**
     * @brief 设置出发航班ID
     * @param id 出发航班ID
     */
    void setDepartureFlightId(const string& id) { departure_flight_id_ = id; }
    
    /**
     * @brief 获取到达航班号
     * @return 到达航班号
     */
    const string& getArrivalFlightNumber() const { return arrival_flight_number_; }
    
    /**
     * @brief 设置到达航班号
     * @param number 到达航班号
     */
    void setArrivalFlightNumber(const string& number) { arrival_flight_number_ = number; }
    
    /**
     * @brief 获取出发航班号
     * @return 出发航班号
     */
    const string& getDepartureFlightNumber() const { return departure_flight_number_; }
    
    /**
     * @brief 设置出发航班号
     * @param number 出发航班号
     */
    void setDepartureFlightNumber(const string& number) { departure_flight_number_ = number; }
    
    /**
     * @brief 获取航站楼
     * @return 航站楼
     */
    const string& getTerminal() const { return terminal_; }
    
    /**
     * @brief 设置航站楼
     * @param terminal 航站楼
     */
    void setTerminal(const string& terminal) { terminal_ = terminal; }
    
    // ===== 任务要求 =====
    
    /**
     * @brief 获取需要的资质类型
     * @return 需要的资质类型（位掩码）
     */
    int getRequiredQualification() const { return required_qualification_; }
    
    /**
     * @brief 设置需要的资质类型
     * @param qualification 需要的资质类型（位掩码）
     */
    void setRequiredQualification(int qualification) { required_qualification_ = qualification; }
    
    /**
     * @brief 是否可以由新员工担任
     * @return true表示可以，false表示不可以
     */
    bool canNewEmployee() const { return can_new_employee_; }
    
    /**
     * @brief 设置是否可以由新员工担任
     * @param can true表示可以，false表示不可以
     */
    void setCanNewEmployee(bool can) { can_new_employee_ = can; }
    
    
    /**
     * @brief 获取需要的人员数量
     * @return 需要的人员数量
     */
    int getRequiredCount() const { return required_count_; }
    
    /**
     * @brief 设置需要的人员数量
     * @param count 需要的人员数量
     */
    void setRequiredCount(int count) { required_count_ = count; }
    
    // ===== 任务分配状态 =====
    
    /**
     * @brief 是否已经分配
     * @return true表示已分配，false表示未分配
     */
    bool isAssigned() const { return is_assigned_; }
    
    /**
     * @brief 设置是否已经分配
     * @param assigned true表示已分配，false表示未分配
     */
    void setAssigned(bool assigned) { is_assigned_ = assigned; }
    
    /**
     * @brief 是否缺少人手
     * @return true表示缺少人手，false表示不缺少
     */
    bool isShortStaffed() const { return is_short_staffed_; }
    
    /**
     * @brief 设置是否缺少人手
     * @param short_staffed true表示缺少人手，false表示不缺少
     */
    void setShortStaffed(bool short_staffed) { is_short_staffed_ = short_staffed; }
    
    /**
     * @brief 获取已分配的员工ID列表
     * @return 已分配的员工ID列表
     */
    const vector<string>& getAssignedEmployeeIds() const { return assigned_employee_ids_; }
    
    /**
     * @brief 获取已分配的员工数量
     * @return 已分配的员工数量
     */
    size_t getAssignedEmployeeCount() const { return assigned_employee_ids_.size(); }
    
    /**
     * @brief 添加已分配的员工ID
     * @param employee_id 员工ID
     */
    void addAssignedEmployeeId(const string& employee_id);
    
    /**
     * @brief 移除已分配的员工ID
     * @param employee_id 员工ID
     */
    void removeAssignedEmployeeId(const string& employee_id);
    
    /**
     * @brief 检查是否已分配给指定员工
     * @param employee_id 员工ID
     * @return true表示已分配，false表示未分配
     */
    bool isAssignedToEmployee(const string& employee_id) const;
    
    /**
     * @brief 清空所有已分配的员工
     */
    void clearAssignedEmployees();

private:
    // 任务基本信息
    string task_id_;                    ///< 任务ID（字符串类型）
    string task_name_;                  ///< 任务名称
    bool prefer_main_shift_;            ///< 是否优先主班
    
    // 任务时间信息
    long earliest_start_time_;          ///< 任务最早开始时间（从2020年1月1日0点0分0秒开始的秒数）
    long latest_end_time_;              ///< 任务最晚结束时间（从2020年1月1日0点0分0秒开始的秒数）
    long actual_start_time_;            ///< 任务实际开始时间（分配后设置，从2020年1月1日0点0分0秒开始的秒数），0表示未分配
    long duration_;                     ///< 任务时长（秒）
    
    // 航班信息
    int flight_type_;                   ///< 航班类型（0-5）
    long arrival_time_;                 ///< 进港时间（从2020年1月1日0点0分0秒开始的秒数）
    long departure_time_;               ///< 出港时间（从2020年1月1日0点0分0秒开始的秒数）
    long travel_time_;                   ///< 通勤时间（秒，默认8分钟=480秒）
    bool is_remote_stand_;              ///< 是否远机位
    int stand_;                         ///< 机位编号，0表示未分配机位
    string task_date_;                  ///< 任务日期（格式：YYYY-MM-DD）
    string arrival_flight_id_;         ///< 到达航班ID
    string departure_flight_id_;       ///< 出发航班ID
    string arrival_flight_number_;     ///< 到达航班号
    string departure_flight_number_;   ///< 出发航班号
    string terminal_;                   ///< 航站楼
    
    // 任务要求
    int required_qualification_;         ///< 需要的资质类型（位掩码）
    bool can_new_employee_;             ///< 是否可以由新员工担任
    int required_count_;                ///< 需要的人员数量（直接从CSV读取，不再从货量计算）
    
    // 任务分配状态
    bool is_assigned_;                  ///< 是否已经分配
    bool is_short_staffed_;             ///< 是否缺少人手
    vector<string> assigned_employee_ids_;  ///< 已分配的员工ID列表
};

}  // namespace zhuangxie_class

#endif  // ZHUANGXIE_CLASS_LOAD_TASK_H

