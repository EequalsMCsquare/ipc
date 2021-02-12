#pragma once 

#include <stdexcept>

#include "ec.hpp"

namespace ipc
{
    class IPCExcept : std::exception
    {
    private:
        char _M_What[256];

    public:
        IPCExcept(const IPCErrc ec);
        IPCExcept(const std::error_code ec);

        const char* what() const noexcept final override;
    };
}