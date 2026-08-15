#pragma once

#include "VarCtlController.h"

#include <cstddef>

namespace VarCtlRuntime
{
    bool Create();
    void Destroy();

    Controller* Data();
    size_t Count();
    Controller* At(size_t index);
}