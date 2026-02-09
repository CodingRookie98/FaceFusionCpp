module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module domain.face.classifier;
import :fair_face;
import domain.face;
import domain.face.helper;
import foundation.ai.inference_session_registry;

namespace domain::face::classifier {

FairFace::FairFace() = default;

void FairFace::load_model(const std::string& model_path,
                          const foundation::ai::inference_session::Options& options) {
    m_session =
        foundation::ai::inference_session::InferenceSessionRegistry::get_instance()->get_session(
            model_path, options);
    auto input_dims = get_input_node_dims();
    m_input_width = static_cast<int>(input_dims[0][2]);
    m_input_height = static_cast<int>(input_dims[0][3]);
    m_size = cv::Size(m_input_width, m_input_height);
}

ClassificationResult FairFace::classify(const cv::Mat& image,
                                        const domain::face::types::Landmarks& face_landmark_5) {
    auto [inputData, inputShape] = prepare_input(image, face_landmark_5);
    if (inputData.empty()) return {};

    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> inputTensor;
    inputTensor.emplace_back(Ort::Value::CreateTensor<float>(
        memory_info, inputData.data(), inputData.size(), inputShape.data(), inputShape.size()));

    std::vector<Ort::Value> outputTensor = run(inputTensor);

    return process_output(outputTensor);
}

std::pair<std::vector<float>, std::vector<int64_t>> FairFace::prepare_input(
    const cv::Mat& image, const domain::face::types::Landmarks& face_landmark_5) const {
    cv::Mat inputImage;
    std::tie(inputImage, std::ignore) = helper::warp_face_by_face_landmarks_5(
        image, face_landmark_5, helper::get_warp_template(m_warp_template_type), m_size);

    std::vector<cv::Mat> inputChannels(3);
    cv::split(inputImage, inputChannels);
    for (int c = 0; c < 3; c++) {
        inputChannels[c].convertTo(inputChannels[c], CV_32FC1,
                                   1 / (255.0 * m_standard_deviation.at(c)),
                                   -m_mean.at(c) / m_standard_deviation.at(c));
    }

    const int imageArea = inputImage.cols * inputImage.rows;
    std::vector<float> inputData(imageArea * 3);
    size_t singleChannelSize = imageArea * sizeof(float);
    memcpy(inputData.data(), reinterpret_cast<float*>(inputChannels[2].data),
           singleChannelSize); // R
    memcpy(inputData.data() + imageArea, reinterpret_cast<float*>(inputChannels[1].data),
           singleChannelSize); // G
    memcpy(inputData.data() + 2 * imageArea, reinterpret_cast<float*>(inputChannels[0].data),
           singleChannelSize); // B

    std::vector<int64_t> inputShape{1, 3, m_input_height, m_input_width};
    return {std::move(inputData), std::move(inputShape)};
}

ClassificationResult FairFace::process_output(const std::vector<Ort::Value>& outputTensor) const {
    int64_t raceId = outputTensor[0].GetTensorData<int64_t>()[0];
    int64_t genderId = outputTensor[1].GetTensorData<int64_t>()[0];
    int64_t ageId = outputTensor[2].GetTensorData<int64_t>()[0];

    ClassificationResult result{};
    result.age = categorizeAge(ageId);
    result.gender = categorizeGender(genderId);
    result.race = categorizeRace(raceId);
    return result;
}

domain::face::AgeRange FairFace::categorizeAge(int64_t age_id) {
    domain::face::AgeRange age{};

    if (age_id == 0) {
        age.min = 0;
        age.max = 2;
    } else if (age_id == 1) {
        age.min = 3;
        age.max = 9;
    } else if (age_id == 2) {
        age.min = 10;
        age.max = 19;
    } else if (age_id == 3) {
        age.min = 20;
        age.max = 29;
    } else if (age_id == 4) {
        age.min = 30;
        age.max = 39;
    } else if (age_id == 5) {
        age.min = 40;
        age.max = 49;
    } else if (age_id == 6) {
        age.min = 50;
        age.max = 59;
    } else if (age_id == 7) {
        age.min = 60;
        age.max = 69;
    } else {
        age.min = 70;
        age.max = 100;
    }

    return age;
}

domain::face::Gender FairFace::categorizeGender(int64_t genderId) {
    if (genderId == 0) { return domain::face::Gender::Male; }
    return domain::face::Gender::Female;
}

domain::face::Race FairFace::categorizeRace(int64_t raceId) {
    if (raceId == 1) { return domain::face::Race::Black; }
    if (raceId == 2) { return domain::face::Race::Latino; }
    if (raceId == 3 || raceId == 4) { return domain::face::Race::Asian; }
    if (raceId == 5) { return domain::face::Race::Indian; }
    if (raceId == 6) { return domain::face::Race::Arabic; }
    return domain::face::Race::White;
}

} // namespace domain::face::classifier
