#pragma once
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

namespace utils
{
    class hex
    {
    public:
        static std::string hex32(unsigned value)
        {
            std::ostringstream s;
            s << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
            return s.str();
        }

        static std::string hex64(std::uint64_t value)
        {
            std::ostringstream s;
            s << "0x" << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << value;
            return s.str();
        }

        static std::string hex_ptr(const void* value)
        {
            return hex64(reinterpret_cast<std::uintptr_t>(value));
        }

        static std::string hex_ptr(std::uintptr_t value)
        {
            return hex64(value);
        }
    };
}
