//
// Created by abyss on 2026/5/2.
//

#ifndef JETBRAINS_DEV_CONSOLE_H
#define JETBRAINS_DEV_CONSOLE_H

#include <cstdio>
#include <windows.h>

// for test
namespace console
{
    inline void create_console(void)
    {
#ifndef PUBLISH
        FreeConsole();
        if (!AllocConsole())
        {
            char buffer[1024] = {0};
            sprintf_s(buffer, "Failed to AllocConsole( ), GetLastError( ) = %d", GetLastError());
            MessageBoxA(HWND_DESKTOP, buffer, "Error", MB_OK);

            return;
        }

        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);


        *(__acrt_iob_func(1)) = *fp;
        setvbuf(stdout, NULL, _IONBF, 0);
#else


#endif // !PUBLIC
    }

    inline void close_console(void)
    {
#ifndef PUBLISH
        FILE* fp = (__acrt_iob_func(1));
        if (fp != nullptr)
        {
            fclose(fp);
        }

        if (!FreeConsole())
        {
            char buffer[1024] = {0};
            sprintf_s(buffer, "Failed to FreeConsole(), GetLastError() = %d", GetLastError());
            MessageBoxA(HWND_DESKTOP, buffer, "Error", MB_OK);
            return;
        }
#endif // !PUBLISH
    }
};

#endif //JETBRAINS_DEV_CONSOLE_H