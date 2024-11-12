/**
 ******************************************************************************
 * @file           : face.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

#include "face.h"

bool Face::isEmpty() const {
    if (m_bBox.isEmpty() || m_landmark5.empty()) {
        return true;
    }
    return false;
}