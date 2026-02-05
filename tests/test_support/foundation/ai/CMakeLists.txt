add_library(tests_test_support_foundation_ai_mock_inference_session)
target_sources(tests_test_support_foundation_ai_mock_inference_session
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      mock_inference_session.ixx
)
target_link_libraries(tests_test_support_foundation_ai_mock_inference_session
  PUBLIC
    GTest::gmock
    foundation_ai
    ONNXRuntime::ONNXRuntime
)
