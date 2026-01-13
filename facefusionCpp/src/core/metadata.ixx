/**
 ******************************************************************************
 * @file           : MetaData.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-8-13
 ******************************************************************************
 */

module;
#include <string>

export module metadata;

export namespace ffc::core::metadata {

const std::string name = "FaceFusionCpp";
const std::string description =
    "This project is a C++ implementation of the open-source project facefusion.";
const std::string version = "0.33.0";
const std::string license = "GPL-3.0 License";
const std::string author = "CodingRookie98 https://github.com/CodingRookie98";
const std::string url = "https://github.com/CodingRookie98/FaceFusionCpp";

} // namespace ffc::core::metadata
