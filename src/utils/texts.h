#pragma once

#include <string>
#include <algorithm>

namespace utils
{
    class texts
    {
        public:
            static bool equals(const std::string& a, const std::string& b)
            {
                return a == b;
            }

            static bool equals_ignore_case(const std::string& a, const std::string& b)
            {
                if (a.size() != b.size())
                    return false;
                return std::equal(a.begin(), a.end(), b.begin(), [](char ca, char cb) {
                    return std::tolower(static_cast<unsigned char>(ca)) ==
                           std::tolower(static_cast<unsigned char>(cb));
                });
            }

            static bool contains(const std::string& str, const std::string& substr)
            {
                return str.find(substr) != std::string::npos;
            }

            static bool contains_ignore_case(const std::string& str, const std::string& substr)
            {
                if (substr.size() > str.size())
                    return false;
                auto it = std::search(str.begin(), str.end(), substr.begin(), substr.end(),
                    [](char cs, char ct) {
                        return std::tolower(static_cast<unsigned char>(cs)) ==
                               std::tolower(static_cast<unsigned char>(ct));
                    });
                return it != str.end();
            }
    };
}
