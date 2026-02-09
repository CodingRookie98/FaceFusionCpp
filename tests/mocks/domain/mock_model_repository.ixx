module;
#include <gmock/gmock.h>
#include <string>
#include <vector>

export module tests.mocks.domain.mock_model_repository;

import domain.ai.model_repository;

export namespace tests::mocks::domain {

class MockModelRepository : public ::domain::ai::model_repository::ModelRepository {
public:
    MOCK_METHOD(std::string, ensure_model, (const std::string&), (const, override));
    MOCK_METHOD(void, set_model_info_file_path, (const std::string&), (override));
    MOCK_METHOD(std::string, get_model_path, (const std::string&), (const, override));
    MOCK_METHOD(bool, has_model, (const std::string&), (const, override));
    MOCK_METHOD(bool, is_downloaded, (const std::string&), (const, override));
};

}
