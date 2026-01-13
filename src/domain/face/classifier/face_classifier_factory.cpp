module;
#include <memory>

module domain.face.classifier;

import :factory;
import :fair_face;

namespace domain::face::classifier {

std::unique_ptr<IFaceClassifier> create_classifier(ClassifierType type) {
    switch (type) {
    case ClassifierType::FairFace: return std::make_unique<FairFace>();
    }
    return nullptr;
}

} // namespace domain::face::classifier
