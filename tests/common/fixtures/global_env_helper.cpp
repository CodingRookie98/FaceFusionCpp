#include "common/fixtures/global_env_helper.h"

// Forward declaration of the global function
extern void LinkGlobalTestEnvironment();

namespace tests::common::fixtures::details {
void LinkGlobalEnvHelper() {
    LinkGlobalTestEnvironment();
}
} // namespace tests::common::fixtures::details
