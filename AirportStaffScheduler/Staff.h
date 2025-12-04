// include/Core/Staff.h
#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace AirportStaffScheduler {
    class Staff {
    public:
        // 构造函数（仅必填字段）
        Staff(
            const std::string& staffId,
            const std::string& name,
            const std::string& gender,
            const std::vector<std::string>& qualifications  // 接收 vector，内部转为 unordered_set
        )
            : staffId_(staffId)
            , name_(name)
            , gender_(gender)
        {
            // 转换资质 vector → unordered_set
            qualifications_.reserve(qualifications.size());
            for (const auto& q : qualifications) {
                qualifications_.insert(q);
            }
        }

        // ===== Getters =====
        const std::string& getStaffId() const { return staffId_; }
        const std::string& getName() const { return name_; }
        const std::string& getGender() const { return gender_; }
        const std::unordered_set<std::string>& getQualifications() const { return qualifications_; }
        const std::string& getTeamName() const { return teamName_; }
        const std::vector<std::string>& getRelatedStaffIds() const { return relatedStaffIds_; }
        

        // ===== Setters =====
        void setStaffId(const std::string& id) { staffId_ = id; }
        void setName(const std::string& name) { name_ = name; }
        void setGender(const std::string& gender) { gender_ = gender; }
        void setTeamName(const std::string& team) { teamName_ = team; }
        void setRelatedStaffIds(const std::vector<std::string>& ids) { relatedStaffIds_ = ids; }
        void setQualifications(const std::vector<std::string>& quals) {
            qualifications_.clear();
            qualifications_.reserve(quals.size()); // 避免多次 rehash
            for (const auto& q : quals) {
                qualifications_.insert(q);
            }
            // 注意：vector 中的重复资质会被自动去重
        }

        bool hasAllQualifications(const std::vector<std::string>& required) const {
            for (const auto& qual : required) {
                if (qualifications_.count(qual) == 0) {
                    return false; // 缺少某项资质
                }
            }
            return true; // 全部满足
        }

    private:
        // Member variables
        std::string staffId_;               // 必填
        std::string name_;                  // 必填
        std::string gender_;                // 必填 ("男" / "女")
        std::unordered_set<std::string> qualifications_;   // 必填语义，但可用空 vector 表示“无资质”
        std::string teamName_;              // 可空
        std::vector<std::string> relatedStaffIds_;  // 可空，存储关联的员工ID列表
    };

} // namespace AirportStaffScheduler