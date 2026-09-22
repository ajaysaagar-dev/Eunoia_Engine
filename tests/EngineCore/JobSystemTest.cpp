#include <iostream>
#include <cassert>
#include "EngineCore/JobSystem.h"
#include "EngineCore/UUID.h"
#include "EngineCore/Time.h"
#include "EngineCore/Result.h"
#include "EngineCore/Log.h"

int main() {
    std::cout << "[RUNNING] EngineCore Tests...\n";

    // 1. Result tests
    auto okResult = EngineCore::Result<int>::Ok(42);
    assert(okResult.IsOk());
    assert(okResult.Value() == 42);

    auto errResult = EngineCore::Result<int>::Err(EngineCore::Error("Test Error", 1));
    assert(errResult.IsErr());
    assert(errResult.GetError().code == 1);
    assert(errResult.GetError().message == "Test Error");

    // 2. UUID tests
    EngineCore::UUID id1;
    EngineCore::UUID id2;
    assert(id1 != id2);
    assert(!id1.IsNull());
    assert(id1.ToString().length() == 36);

    // 3. Time tests
    auto& time = EngineCore::Time::Get();
    time.Tick();
    assert(time.TotalTime() >= 0.0f);

    // 4. JobSystem tests
    auto& jobSystem = EngineCore::JobSystem::Get();
    std::atomic<int> counter{0};
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 20; ++i) {
        futures.push_back(jobSystem.Enqueue([i, &counter]() {
            counter++;
            return i * 2;
        }));
    }

    for (int i = 0; i < 20; ++i) {
        int res = futures[i].get();
        assert(res == i * 2);
    }
    assert(counter == 20);

    std::cout << "[PASSED] All EngineCore Tests passed successfully!\n";
    return 0;
}
