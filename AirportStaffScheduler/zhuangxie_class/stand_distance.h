/**
 * @file stand_distance.h
 * @brief 机位距离管理类
 * 
 * 管理机位之间的距离和路程时间
 */

#ifndef ZHUANGXIE_CLASS_STAND_DISTANCE_H
#define ZHUANGXIE_CLASS_STAND_DISTANCE_H

#include <map>

namespace zhuangxie_class {

using namespace std;

/**
 * @brief 机位距离管理类
 * 
 * 管理机位之间的距离和路程时间（秒）
 */
class StandDistance {
public:
    /**
     * @brief 获取单例实例
     * @return StandDistance单例引用
     */
    static StandDistance& getInstance();
    
    /**
     * @brief 获取两个机位之间的路程时间
     * @param stand1 起始机位（1-24）
     * @param stand2 目标机位（1-24）
     * @return 路程时间（秒），如果机位无效或未设置则返回默认值（5分钟=300秒）
     */
    long getTravelTime(int stand1, int stand2) const;
    
    /**
     * @brief 设置两个机位之间的路程时间
     * @param stand1 起始机位（1-24）
     * @param stand2 目标机位（1-24）
     * @param time 路程时间（秒）
     */
    void setTravelTime(int stand1, int stand2, long time);
    
    /**
     * @brief 初始化默认距离矩阵（可根据实际需求调整）
     */
    void initializeDefaultDistances();

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    StandDistance();
    
    /**
     * @brief 私有析构函数
     */
    ~StandDistance();
    
    /**
     * @brief 禁止拷贝构造
     */
    StandDistance(const StandDistance&) = delete;
    
    /**
     * @brief 禁止赋值操作
     */
    StandDistance& operator=(const StandDistance&) = delete;
    
    /**
     * @brief 生成机位对的键（保证stand1 <= stand2）
     * @param stand1 机位1
     * @param stand2 机位2
     * @return 机位对的键
     */
    int makeKey(int stand1, int stand2) const;
    
    map<int, long> distance_map_;  ///< 机位对到路程时间的映射
    static const long DEFAULT_TRAVEL_TIME = 5 * 60;  ///< 默认路程时间（5分钟=300秒）
};

}  // namespace zhuangxie_class

#endif  // ZHUANGXIE_CLASS_STAND_DISTANCE_H

