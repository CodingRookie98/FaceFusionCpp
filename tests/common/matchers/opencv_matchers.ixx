module;
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <string>
#include <ostream>

export module tests.common.matchers.opencv_matchers;

namespace tests::common::matchers {

    // Helper for printing Mats
    // Note: We cannot put this in std namespace or global easily with modules
    // Users might need to use PrintTo explicitly or rely on ADL if careful
    void PrintTo(const cv::Mat& mat, std::ostream* os) {
        *os << "cv::Mat(" << mat.rows << "x" << mat.cols << ", type=" << mat.type() << ")";
    }

    class MatEqMatcher {
    public:
        explicit MatEqMatcher(const cv::Mat& expected) : expected_(expected) {}

        // Required for gmock to recognize this as a matcher
        using is_gtest_matcher = void;

        bool MatchAndExplain(const cv::Mat& actual, std::ostream* listener) const {
            std::cout << "Debug: MatchAndExplain start" << std::endl;
            if (actual.empty() && expected_.empty()) return true;
            if (actual.empty() || expected_.empty()) {
                *listener << "one is empty and the other is not";
                return false;
            }
            if (actual.size() != expected_.size()) {
                *listener << "sizes differ: " << actual.size() << " vs " << expected_.size();
                return false;
            }
            if (actual.type() != expected_.type()) {
                *listener << "types differ: " << actual.type() << " vs " << expected_.type();
                return false;
            }

            cv::Mat diff;
            std::cout << "Debug: Calling cv::compare" << std::endl;
            cv::compare(actual, expected_, diff, cv::CMP_NE);
            std::cout << "Debug: Calling cv::countNonZero" << std::endl;
            int non_zero = cv::countNonZero(diff);
            std::cout << "Debug: Non-zero count: " << non_zero << std::endl;
            
            if (non_zero > 0) {
                std::cout << "Debug: Writing to listener" << std::endl;
                if (listener) {
                    *listener << "pixels differ: " << non_zero;
                } else {
                    std::cout << "Debug: Listener is null!" << std::endl;
                }
                std::cout << "Debug: Returning false" << std::endl;
                return false;
            }
            std::cout << "Debug: Returning true" << std::endl;
            return true;
        }

        void DescribeTo(std::ostream* os) const {
            *os << "is equal to ";
            PrintTo(expected_, os);
        }

        void DescribeNegationTo(std::ostream* os) const {
            *os << "is not equal to ";
            PrintTo(expected_, os);
        }

    private:
        cv::Mat expected_;
    };

    export ::testing::Matcher<const cv::Mat&> MatEq(const cv::Mat& expected) {
        return MatEqMatcher(expected);
    }
}
