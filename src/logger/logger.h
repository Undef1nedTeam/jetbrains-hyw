#pragma once

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

// =============================================================================
// ConsoleColor - Win32 console color definitions
// =============================================================================

enum class ConsoleColor : WORD
{
    Black = 0,
    DarkBlue = FOREGROUND_BLUE,
    DarkGreen = FOREGROUND_GREEN,
    DarkCyan = FOREGROUND_GREEN | FOREGROUND_BLUE,
    DarkRed = FOREGROUND_RED,
    DarkMagenta = FOREGROUND_RED | FOREGROUND_BLUE,
    DarkYellow = FOREGROUND_RED | FOREGROUND_GREEN,
    DarkGray = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    Gray = FOREGROUND_INTENSITY,
    BrightBlue = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
    BrightGreen = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    BrightCyan = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
    BrightRed = FOREGROUND_INTENSITY | FOREGROUND_RED,
    BrightMagenta = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
    BrightYellow = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    BrightWhite = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
};

struct ConsoleTextSegment
{
    std::string text;
    ConsoleColor color = ConsoleColor::BrightWhite;
};

// =============================================================================
// ConsoleTextColor - Utility for setting/saving/restoring console colors
// =============================================================================

class ConsoleTextColor
{
public:
    ConsoleTextColor() = delete;

    static void Set(ConsoleColor color);
    static WORD Save();
    static void Restore(WORD attributes);

    struct ScopedColor
    {
        WORD savedAttributes;
        ScopedColor(ConsoleColor color);
        ~ScopedColor();
        ScopedColor(const ScopedColor&) = delete;
        ScopedColor& operator=(const ScopedColor&) = delete;
    };
};

// =============================================================================
// logger - Main logging interface with colored console output
// =============================================================================

class logger
{
public:
    // -----------------------------------------------------------------------
    // Initialization - call once at startup
    // -----------------------------------------------------------------------
    static void Init();

    // -----------------------------------------------------------------------
    // log line with auto-detected color based on content tags
    // -----------------------------------------------------------------------
    static void log_line(const std::string& line);
    static void log_line(const char* line);

    // -----------------------------------------------------------------------
    // Variadic template log (formats via ostringstream)
    // -----------------------------------------------------------------------
    template <typename... Args>
    static void log(const Args&... args)
    {
        std::ostringstream stream;
        (stream << ... << args);
        log_line(stream.str());
    }

    template <typename... Args>
    static void logf(const char* fmt, ...)
    {
        char buffer[8192]{};
        va_list args;
        va_start(args, fmt);
        _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, args);
        va_end(args);
        log_line(std::string(buffer));
    }

    // -----------------------------------------------------------------------
    // Printf-style formatted logging
    // -----------------------------------------------------------------------
    static std::string Format(const char* fmt, ...);

    // -----------------------------------------------------------------------
    // Colored output with explicit color
    // -----------------------------------------------------------------------
    static void WriteColoredLine(const std::string& message, ConsoleColor color);
    static void WriteColoredChar(char ch, ConsoleColor color);
    static void WriteColoredText(const std::string& text, ConsoleColor color);
    static void WriteSegments(const std::vector<ConsoleTextSegment>& segments, bool newline = false);
    static void WriteSegmentsLine(const std::vector<ConsoleTextSegment>& segments);
    static void WriteRainbowLine(const std::string& message);

    // -----------------------------------------------------------------------
    // Tagged output: colored bracket prefix + default-colored rest
    // -----------------------------------------------------------------------
    static void WriteTaggedLine(const std::string& tag, ConsoleColor tagColor, const std::string& message);

    // -----------------------------------------------------------------------
    // Plain console write (auto-detects color from tags)
    // -----------------------------------------------------------------------
    static void WriteLine(const std::string& message);
    static void WriteLine(const char* message);

    template <typename T>
    static void WriteLine(const T& value)
    {
        std::ostringstream stream;
        stream << value;
        WriteLine(stream.str());
    }

    // -----------------------------------------------------------------------
    // Minecraft-style color code parsing (&0-&f, &r to reset)
    // -----------------------------------------------------------------------
    static ConsoleColor ParseColorCode(char code);
    static std::vector<ConsoleTextSegment> ParseColorCodes(const std::string& message);
    static std::string StripColorCodes(const std::string& message);

    // -----------------------------------------------------------------------
    // Color detection based on message content
    // -----------------------------------------------------------------------
    static ConsoleColor GetColorForTag(const std::string& message);

    // -----------------------------------------------------------------------
    // File output paths
    // -----------------------------------------------------------------------
    static const std::string& DumpOutputDirectory();
    static std::string BuildDumpOutputPath(const std::string& fileName);
    static const std::string& ConsoleLogFilePath();
    static const std::string& TraceLogFilePath();

    // -----------------------------------------------------------------------
    // File-only logging (no console output)
    // -----------------------------------------------------------------------
    static void AppendLineToPath(const std::string& path, const std::string& message);
    static void AppendLineToFile(const std::string& message);

    // -----------------------------------------------------------------------
    // Directory setup
    // -----------------------------------------------------------------------
    static void EnsureDumpOutputDirectoryExists();

private:
    static bool CreateDedicatedConsole();

    static std::mutex s_logMutex;
    static bool s_initialized;
    static bool s_consoleAllocated;
};
