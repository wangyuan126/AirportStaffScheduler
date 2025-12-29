/**
 * @file flight.h
 * @brief 航班信息类
 * 
 * 定义装卸排班系统中的航班信息
 */

#ifndef ZHUANGXIE_CLASS_FLIGHT_H
#define ZHUANGXIE_CLASS_FLIGHT_H

#include <cfloat>  // for double constants

namespace zhuangxie_class {

using namespace std;

/**
 * @brief 航班类型枚举
 */
enum class FlightType {
    DOMESTIC_ARRIVAL = 0,      ///< 国内进港
    DOMESTIC_DEPARTURE = 1,    ///< 国内出港
    DOMESTIC_TRANSIT = 2,      ///< 国内过站
    INTERNATIONAL_ARRIVAL = 3, ///< 国外进港
    INTERNATIONAL_DEPARTURE = 4,///< 国外出港
    INTERNATIONAL_TRANSIT = 5  ///< 国外过站
};

/**
 * @brief 航班信息类
 * 
 * 描述一个航班的基本信息
 */
class Flight {
public:
    /**
     * @brief 构造函数
     */
    Flight();
    
    /**
     * @brief 析构函数
     */
    ~Flight();
    
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
     * @brief 获取贵宾通勤时间
     * @return 贵宾通勤时间（秒，默认8分钟=480秒）
     */
    long getVipTravelTime() const { return vip_travel_time_; }
    
    /**
     * @brief 设置贵宾通勤时间
     * @param time 贵宾通勤时间（秒）
     */
    void setVipTravelTime(long time) { vip_travel_time_ = time; }
    
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
     * @brief 获取进港货量（吨）
     * @return 进港货量（吨）
     */
    double getArrivalCargo() const { return arrival_cargo_; }
    
    /**
     * @brief 设置进港货量（吨）
     * @param cargo 进港货量（吨）
     */
    void setArrivalCargo(double cargo) { arrival_cargo_ = cargo; }
    
    /**
     * @brief 获取出港货量（吨）
     * @return 出港货量（吨）
     */
    double getDepartureCargo() const { return departure_cargo_; }
    
    /**
     * @brief 设置出港货量（吨）
     * @param cargo 出港货量（吨）
     */
    void setDepartureCargo(double cargo) { departure_cargo_ = cargo; }
    
    /**
     * @brief 获取报时（准确时间）
     * @return 报时（从2020年1月1日0点0分0秒开始的秒数），0表示未报时
     */
    long getReportTime() const { return report_time_; }
    
    /**
     * @brief 设置报时（准确时间）
     * @param time 报时（从2020年1月1日0点0分0秒开始的秒数），0表示未报时
     */
    void setReportTime(long time) { report_time_ = time; }
    
    /**
     * @brief 检查是否已报时
     * @return true表示已报时，false表示未报时
     */
    bool hasReported() const { return report_time_ > 0; }
    
    /**
     * @brief 获取机位
     * @return 机位编号（1-24），0表示未分配机位
     */
    int getStand() const { return stand_; }
    
    /**
     * @brief 设置机位
     * @param stand 机位编号（1-24），0表示未分配机位
     */
    void setStand(int stand) { stand_ = stand; }

private:
    int flight_type_;          ///< 航班类型（0-5）
    long arrival_time_;         ///< 进港时间（从2020年1月1日0点0分0秒开始的秒数）
    long departure_time_;       ///< 出港时间（从2020年1月1日0点0分0秒开始的秒数）
    long vip_travel_time_;      ///< 贵宾通勤时间（秒，默认8分钟=480秒）
    bool is_remote_stand_;         ///< 是否远机位
    double arrival_cargo_;         ///< 进港货量（吨）
    double departure_cargo_;       ///< 出港货量（吨）
    long report_time_;          ///< 报时（准确时间，从2020年1月1日0点0分0秒开始的秒数），0表示未报时
    int stand_;                ///< 机位编号（1-24），0表示未分配机位
};

}  // namespace zhuangxie_class

#endif  // ZHUANGXIE_CLASS_FLIGHT_H

