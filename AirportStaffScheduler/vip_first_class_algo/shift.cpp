/**
 * @file shift.cpp
 * @brief 班次类实现
 */

#include "shift.h"

namespace vip_first_class {

Shift::Shift()
    : shift_type_(static_cast<int32_t>(ShiftType::REST))
{
}

Shift::~Shift()
{
}

}  // namespace vip_first_class

