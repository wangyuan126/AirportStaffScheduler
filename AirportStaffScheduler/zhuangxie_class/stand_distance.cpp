/**
 * @file stand_distance.cpp
 * @brief 机位距离管理类实现
 */

#include "stand_distance.h"
#include <algorithm>

namespace zhuangxie_class {

using namespace std;

StandDistance& StandDistance::getInstance() {
    static StandDistance instance;
    return instance;
}

StandDistance::StandDistance() {
    initializeDefaultDistances();
}

StandDistance::~StandDistance() {
}

int StandDistance::makeKey(int stand1, int stand2) const {
    // 保证stand1 <= stand2
    if (stand1 > stand2) {
        swap(stand1, stand2);
    }
    // 使用stand1 * 100 + stand2作为键
    return stand1 * 100 + stand2;
}

long StandDistance::getTravelTime(int stand1, int stand2) const {
    // 验证机位有效性
    if (stand1 <= 0 || stand1 > 24 || stand2 <= 0 || stand2 > 24) {
        return DEFAULT_TRAVEL_TIME;
    }
    
    // 相同机位路程时间为0
    if (stand1 == stand2) {
        return 0;
    }
    
    int key = makeKey(stand1, stand2);
    auto it = distance_map_.find(key);
    if (it != distance_map_.end()) {
        return it->second;
    }
    
    // 如果未设置，返回默认值
    return DEFAULT_TRAVEL_TIME;
}

void StandDistance::setTravelTime(int stand1, int stand2, long time) {
    // 验证机位有效性
    if (stand1 <= 0 || stand1 > 24 || stand2 <= 0 || stand2 > 24) {
        return;
    }
    
    int key = makeKey(stand1, stand2);
    distance_map_[key] = time;
}

void StandDistance::initializeDefaultDistances() {
    // 初始化默认距离矩阵
    // 相邻机位：3分钟（180秒）
    // 相近机位（相差1-2个）：5分钟（300秒）
    // 远机位（相差3-5个）：8分钟（480秒）
    // 最远机位（相差6个以上）：12分钟（720秒）
    
    for (int i = 1; i <= 24; ++i) {
        for (int j = i + 1; j <= 24; ++j) {
            int diff = j - i;
            long time;
            
            if (diff == 1) {
                time = 3 * 60;  // 相邻机位：3分钟
            } else if (diff <= 2) {
                time = 5 * 60;  // 相近机位：5分钟
            } else if (diff <= 5) {
                time = 8 * 60;  // 远机位：8分钟
            } else {
                time = 12 * 60; // 最远机位：12分钟
            }
            
            setTravelTime(i, j, time);
        }
    }
}

}  // namespace zhuangxie_class

