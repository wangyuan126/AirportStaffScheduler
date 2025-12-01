#pragma once
#include <string>

namespace AirportStaffScheduler {

    class VehicleInfo {
    public:
        // 构造函数：仅包含“是否可空 = 否”的字段
        VehicleInfo(
            const std::string& licensePlate,
            const std::string& vehicleType,
            bool available  // Y -> true, N -> false
        )
            : licensePlate_(licensePlate)
            , vehicleType_(vehicleType)
            , available_(available)
            , assignedStaffId_("")
            , assignedStaffName_("")
        {}

        // Getters
        const std::string& getLicensePlate() const { return licensePlate_; }
        const std::string& getVehicleType() const { return vehicleType_; }
        bool isAvailable() const { return available_; }
        const std::string& getAssignedStaffId() const { return assignedStaffId_; }
        const std::string& getAssignedStaffName() const { return assignedStaffName_; }

        // Setters
        void setLicensePlate(const std::string& value) { licensePlate_ = value; }
        void setVehicleType(const std::string& value) { vehicleType_ = value; }
        void setAvailable(bool value) { available_ = value; }
        void setAssignedStaffId(const std::string& value) { assignedStaffId_ = value; }
        void setAssignedStaffName(const std::string& value) { assignedStaffName_ = value; }

    private:
        std::string licensePlate_;        // 车牌号，必填
        std::string vehicleType_;         // 车辆类型，必填
        bool available_;                  // 车辆状态：Y=true, N=false，必填
        std::string assignedStaffId_;     // 绑定人员编号，可空
        std::string assignedStaffName_;   // 绑定人员姓名，可空
    };
}