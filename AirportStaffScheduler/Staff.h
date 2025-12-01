// include/Core/Staff.h
#pragma once

#include <string>
#include <vector>

namespace AirportStaffScheduler {

    class Staff {
    public:
        // 构造函数（仅必填字段）
        Staff(
            const std::string& staffId,
            const std::string& name,
            const std::string& gender
        )
            : staffId_(staffId)
            , name_(name)
            , gender_(gender)
            , qualifications_()
            , teamName_("")
            , relatedStaffIds_()
        {}

        // ===== Getters =====
        const std::string& getStaffId() const { return staffId_; }
        const std::string& getName() const { return name_; }
        const std::string& getGender() const { return gender_; }
        const std::vector<std::string>& getQualifications() const { return qualifications_; }
        const std::string& getTeamName() const { return teamName_; }
        const std::vector<std::string>& getRelatedStaffIds() const { return relatedStaffIds_; }

        // ===== Setters =====
        void setStaffId(const std::string& id) { staffId_ = id; }
        void setName(const std::string& name) { name_ = name; }
        void setGender(const std::string& gender) { gender_ = gender; }
        void setQualifications(const std::vector<std::string>& quals) { qualifications_ = quals; }
        void setTeamName(const std::string& team) { teamName_ = team; }
        void setRelatedStaffIds(const std::vector<std::string>& ids) { relatedStaffIds_ = ids; }


    private:
        // Member variables
        std::string staffId_;               // 必填
        std::string name_;                  // 必填
        std::string gender_;                // 必填 ("男" / "女")
        std::vector<std::string> qualifications_;   // 必填语义，但可用空 vector 表示“无资质”
        std::string teamName_;              // 可空
        std::vector<std::string> relatedStaffIds_;  // 可空，存储关联的员工ID列表
    };

} // namespace AirportStaffScheduler