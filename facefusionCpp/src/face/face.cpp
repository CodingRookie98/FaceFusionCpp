/**
 ******************************************************************************
 * @file           : face.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */
module;
#include <vector>

module face;

namespace ffc {
bool Face::is_empty() const {
    if (m_box.is_empty() || m_landmark5.empty()) {
        return true;
    }
    return false;
}
} // namespace ffc