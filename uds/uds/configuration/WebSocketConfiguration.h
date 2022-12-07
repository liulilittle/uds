#pragma once

#include <uds/stdafx.h>

namespace uds {
    namespace configuration {
        struct WebSocketConfiguration {
            std::string                         Host;
            std::string                         Path;
        };
    }
}