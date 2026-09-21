#pragma once
#include <string>

class IBehaviourService {
public:
    virtual ~IBehaviourService() = default;
    virtual size_t GetRegisteredCount() const = 0;
};
