#pragma once
#include <codecvt>
#include <locale>
#include <string>

#include "console.h"

namespace utils
{
    class converts
    {
        public:
        static std::string to_string(LPCWSTR lpcwszStr)
        {
            // Determine the length of the converted string
            const int strLength
                = WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, -1,
                                      nullptr, 0, nullptr, nullptr);

            // Create a std::string with the determined length
            std::string str(strLength, 0);

            // Perform the conversion from LPCWSTR to std::string
            WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, -1, &str[0],
                                strLength, nullptr, nullptr);

            // Return the converted std::string
            return str;
        }

        static std::wstring s2ws(const std::string& str) {
            using convert_type = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_type, wchar_t> converter;
            return converter.from_bytes(str);
        }

        static std::string ws2s(const std::wstring& wstr) {
            using convert_type = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_type, wchar_t> converter;
            return converter.to_bytes(wstr);
        }
    };
}


