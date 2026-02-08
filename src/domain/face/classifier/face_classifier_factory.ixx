module;
#include <memory>

export module domain.face.classifier:factory;

import :api;

export namespace domain::face::classifier {

enum class ClassifierType : std::uint8_t {
    FairFace,
};

std::unique_ptr<IFaceClassifier> create_classifier(ClassifierType type);

} // namespace domain::face::classifier
