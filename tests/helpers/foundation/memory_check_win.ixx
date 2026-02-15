module;
#include <cstdint>
#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

export module tests.helpers.foundation.memory_check_win;

export namespace tests::helpers::foundation {

#ifdef _WIN32
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

        if (_CrtMemDifference(&diff, &start_state_, &end_state)) { _CrtMemDumpStatistics(&diff); }
    }

    int64_t get_current_allocation_bytes() {
        _CrtMemState current;
        _CrtMemCheckpoint(&current);
        return static_cast<int64_t>(current.lTotalCount);
    }

private:
    _CrtMemState start_state_;
};
#else
class MemoryLeakChecker {
public:
    MemoryLeakChecker() = default;
    int64_t get_current_allocation_bytes() { return 0; }
};
#endif

} // namespace tests::helpers::foundation
