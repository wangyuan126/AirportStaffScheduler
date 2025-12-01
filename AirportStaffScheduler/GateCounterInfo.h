#pragma once
#include <string>
#include <vector>

namespace AirportStaffScheduler {

    class GateCounterInfo {
    public:
        // 构造函数：仅必填字段（机位/柜台编号）
        GateCounterInfo(const std::string& gateOrCounterId)
            : gateOrCounterId_(gateOrCounterId)
            , adjacentGateOrCounterIds_()
            , isRemoteStand_(false)
            , area_("")
            , terminal_("")
        {}

        // Getters
        const std::string& getGateOrCounterId() const { return gateOrCounterId_; }
        const std::vector<std::string>& getAdjacentGateOrCounterIds() const { return adjacentGateOrCounterIds_; }
        bool isRemoteStand() const { return isRemoteStand_; }
        const std::string& getArea() const { return area_; }
        const std::string& getTerminal() const { return terminal_; }

        // Setters
        void setGateOrCounterId(const std::string& value) { gateOrCounterId_ = value; }
        void setAdjacentGateOrCounterIds(const std::vector<std::string>& value) { adjacentGateOrCounterIds_ = value; }
        void setRemoteStand(bool value) { isRemoteStand_ = value; }
        void setArea(const std::string& value) { area_ = value; }
        void setTerminal(const std::string& value) { terminal_ = value; }

    private:
        std::string gateOrCounterId_;                // 必填
        std::vector<std::string> adjacentGateOrCounterIds_; // 可空列表
        bool isRemoteStand_;                         // 可空布尔，默认 false
        std::string area_;                           // 可空
        std::string terminal_;                       // 可空
    };

} // namespace AirportStaffScheduler