//
// Created by abyss on 2026/5/2.
//

#include "crash_logger.h"

#include <excpt.h>

#include "logger/logger.h"
#include "utils/hex.h"

volatile LONG g_installed = 0;

LONG WINAPI CrashLogger(PEXCEPTION_POINTERS exceptionInfo)
{
    if (exceptionInfo != nullptr && exceptionInfo->ExceptionRecord != nullptr)
    {
        logger::log("&c", "[crash]", "&7code=&c", utils::hex::hex32(exceptionInfo->ExceptionRecord->ExceptionCode),
            "&7 addr=&c", utils::hex::hex_ptr(exceptionInfo->ExceptionRecord->ExceptionAddress),
            "&7 rip=&c", exceptionInfo->ContextRecord != nullptr ? utils::hex::hex64(exceptionInfo->ContextRecord->Rip) : "null");
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void crash_logger::InstallCrashLogger()
{
    if (InterlockedCompareExchange(&g_installed, 1, 0) != 0)
    {
        return;
    }

    crash_logger_handle = AddVectoredExceptionHandler(1, CrashLogger);
    logger::log("[crash]", " crash logger &3", crash_logger_handle != nullptr ? "installed" : "failed");
}