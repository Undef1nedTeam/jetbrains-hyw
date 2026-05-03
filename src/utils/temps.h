#pragma once

#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>

#include "converts.h"

class TempFile {
public:
    explicit TempFile(const std::wstring& path) : path_(path) {}

    ~TempFile() {
        if (!path_.empty()) {
            BOOL ok = DeleteFileW(path_.c_str());
            if (!ok) {
                std::wcout << L"[TempFileGuard] error=" << GetLastError() << std::endl;
            }
        }
    }

    std::string path() const
    {
        return utils::converts::ws2s(path_);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    bool WriteBytesToFile(const unsigned char* data, const unsigned int len) const
    {
        HANDLE hFile = CreateFileW(
            path_.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (hFile == INVALID_HANDLE_VALUE) {
            std::wcerr << L"CreateFile, error=" << GetLastError() << std::endl;
            return false;
        }

        DWORD written = 0;
        BOOL ok = WriteFile(hFile, data, (DWORD)len, &written, nullptr);
        CloseHandle(hFile);

        if (!ok || written != len) {
            std::wcerr << L"WriteFile" << std::endl;
            return false;
        }
        return true;
    }

    static std::wstring CreateTempFile() {
        wchar_t tempDir[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, tempDir);

        wchar_t tempFile[MAX_PATH] = {};
        UINT unique = GetTempFileNameW(tempDir, L"tmp", 0, tempFile);
        if (unique == 0) {
            std::wcerr << L"GetTempFileName, error=" << GetLastError() << std::endl;
            return L"";
        }
        return tempFile;
    }

private:
    std::wstring path_;
};
