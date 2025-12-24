/**
 * @file flight.cpp
 * @brief 航班信息类实现
 */

#include "flight.h"

namespace zhuangxie_class {

using namespace std;

const long DEFAULT_VIP_TRAVEL_TIME = 8 * 60;  // 默认8分钟 = 480秒

Flight::Flight()
    : flight_type_(0)
    , arrival_time_(0)
    , departure_time_(0)
    , vip_travel_time_(DEFAULT_VIP_TRAVEL_TIME)
    , is_remote_stand_(false)
    , arrival_cargo_(0.0)
    , departure_cargo_(0.0)
    , report_time_(0)
    , stand_(0)
{
}

Flight::~Flight()
{
}

}  // namespace zhuangxie_class

