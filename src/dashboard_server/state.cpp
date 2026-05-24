#include <dashboard_server/state.hpp>

namespace dashboard {

long long nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

}  // namespace dashboard
