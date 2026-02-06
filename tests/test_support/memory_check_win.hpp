#pragma once

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cstdint>

namespace test_support {

/**
 * @brief RAII Memory Leak Detector (Windows Debug only)
 */
class MemoryLeakChecker {
public:
    MemoryLeakChecker() {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        _CrtMemCheckpoint(&start_state_);
    }
    
    ~MemoryLeakChecker() {
        _CrtMemState end_state, diff;
        _CrtMemCheckpoint(&end_state);
        
        if (_CrtMemDifference(&diff, &start_state_, &end_state)) {
            _CrtMemDumpStatistics(&diff);
            // Note: Do not throw exceptions in destructor
        }
    }
    
    int64_t get_current_allocation_bytes() {
        _CrtMemState current;
        _CrtMemCheckpoint(&current);
        return static_cast<int64_t>(current.lTotalCount);
    }

private:
    _CrtMemState start_state_;
};

} // namespace test_support
#endif
