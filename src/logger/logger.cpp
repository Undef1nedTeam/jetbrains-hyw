#include "logger.h"

// =============================================================================
// Static member definitions
// =============================================================================

std::mutex logger::s_logMutex;
bool logger::s_initialized = false;
bool logger::s_consoleAllocated = false;

namespace
{
	HANDLE g_loggerConsoleOut = nullptr;
	WORD g_loggerDefaultAttributes = static_cast<WORD>(ConsoleColor::BrightWhite);

	const char* AnsiColorCode(ConsoleColor color)
	{
		switch (color)
		{
		case ConsoleColor::Black:
			return "\x1b[30m";
		case ConsoleColor::DarkBlue:
			return "\x1b[34m";
		case ConsoleColor::DarkGreen:
			return "\x1b[32m";
		case ConsoleColor::DarkCyan:
			return "\x1b[36m";
		case ConsoleColor::DarkRed:
			return "\x1b[31m";
		case ConsoleColor::DarkMagenta:
			return "\x1b[35m";
		case ConsoleColor::DarkYellow:
			return "\x1b[33m";
		case ConsoleColor::DarkGray:
			return "\x1b[90m";
		case ConsoleColor::Gray:
			return "\x1b[37m";
		case ConsoleColor::BrightBlue:
			return "\x1b[94m";
		case ConsoleColor::BrightGreen:
			return "\x1b[92m";
		case ConsoleColor::BrightCyan:
			return "\x1b[96m";
		case ConsoleColor::BrightRed:
			return "\x1b[91m";
		case ConsoleColor::BrightMagenta:
			return "\x1b[95m";
		case ConsoleColor::BrightYellow:
			return "\x1b[93m";
		case ConsoleColor::BrightWhite:
		default:
			return "\x1b[97m";
		}
	}

	HANDLE LoggerConsoleOut()
	{
		if (g_loggerConsoleOut == nullptr || g_loggerConsoleOut == INVALID_HANDLE_VALUE)
		{
			g_loggerConsoleOut = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		}
		return g_loggerConsoleOut;
	}

	void EnableVirtualTerminalForLoggerConsole()
	{
		HANDLE output = LoggerConsoleOut();
		if (output == nullptr || output == INVALID_HANDLE_VALUE)
		{
			return;
		}

		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (GetConsoleScreenBufferInfo(output, &csbi))
		{
			g_loggerDefaultAttributes = csbi.wAttributes;
		}

		DWORD mode = 0;
		if (GetConsoleMode(output, &mode))
		{
			mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(output, mode);
		}
	}

	void WriteRawToLoggerConsole(const std::string& text)
	{
		HANDLE output = LoggerConsoleOut();
		if (output == nullptr || output == INVALID_HANDLE_VALUE || text.empty())
		{
			return;
		}

		DWORD written = 0;
		WriteConsoleA(output, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
	}

	void WriteAnsiColoredText(const std::string& text, ConsoleColor color)
	{
		// Emit ANSI escape sequences first. This works in CLion/Windows Terminal and
		// also works in modern conhost after logger::Init enables VT processing.
		WriteRawToLoggerConsole(std::string(AnsiColorCode(color)) + text + "\x1b[0m");
	}

	void WriteConsoleColoredTextNoLock(const std::string& text, ConsoleColor color)
	{
		HANDLE output = LoggerConsoleOut();
		if (output == nullptr || output == INVALID_HANDLE_VALUE)
		{
			return;
		}

		WORD savedAttr = g_loggerDefaultAttributes;
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (GetConsoleScreenBufferInfo(output, &csbi))
		{
			savedAttr = csbi.wAttributes;
		}

		SetConsoleTextAttribute(output, static_cast<WORD>(color));
		WriteAnsiColoredText(text, color);
		SetConsoleTextAttribute(output, savedAttr);
	}

	void WriteNewLineToLoggerConsole()
	{
		WriteRawToLoggerConsole("\r\n");
	}

	std::string FlattenSegments(const std::vector<ConsoleTextSegment>& segments)
	{
		std::string plain;
		for (const auto& segment : segments)
		{
			plain += segment.text;
		}
		return plain;
	}

	void AppendLineToDefaultFilesNoLock(const std::string& message)
	{
		logger::AppendLineToPath(logger::ConsoleLogFilePath(), message);
		logger::AppendLineToPath(logger::TraceLogFilePath(), message);
	}

	bool HasColorCodes(const std::string& message)
	{
		return message.find('&') != std::string::npos;
	}

	bool IsColorCodeChar(char c)
	{
		// 0-9, a-f, A-F, r, R are valid color code characters
		return (c >= '0' && c <= '9') ||
			(c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ||
			c == 'r' || c == 'R';
	}
}

// =============================================================================
// ConsoleTextColor implementation
// =============================================================================

void ConsoleTextColor::Set(ConsoleColor color)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut != INVALID_HANDLE_VALUE && hStdOut != nullptr)
	{
		SetConsoleTextAttribute(hStdOut, static_cast<WORD>(color));
	}
}

WORD ConsoleTextColor::Save()
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut != INVALID_HANDLE_VALUE && hStdOut != nullptr)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (GetConsoleScreenBufferInfo(hStdOut, &csbi))
		{
			return csbi.wAttributes;
		}
	}
	return static_cast<WORD>(ConsoleColor::BrightWhite);
}

void ConsoleTextColor::Restore(WORD attributes)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut != INVALID_HANDLE_VALUE && hStdOut != nullptr)
	{
		SetConsoleTextAttribute(hStdOut, attributes);
	}
}

ConsoleTextColor::ScopedColor::ScopedColor(ConsoleColor color)
	: savedAttributes(ConsoleTextColor::Save())
{
	ConsoleTextColor::Set(color);
}

ConsoleTextColor::ScopedColor::~ScopedColor()
{
	ConsoleTextColor::Restore(savedAttributes);
}

// =============================================================================
// logger::Init
// =============================================================================

bool logger::CreateDedicatedConsole()
{
	if (s_consoleAllocated)
	{
		return true;
	}

	// 优先附加到父进程的控制台（例如从 cmd/PowerShell 启动时的控制台）
	// 如果进程已有控制台，AttachConsole 会失败，此时直接使用当前控制台即可
	AttachConsole(ATTACH_PARENT_PROCESS);

	s_consoleAllocated = true;
	g_loggerConsoleOut = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	EnableVirtualTerminalForLoggerConsole();
	return true;
}

void logger::Init()
{
	if (s_initialized)
	{
		return;
	}
	s_initialized = true;
	CreateDedicatedConsole();

	EnsureDumpOutputDirectoryExists();
}

// =============================================================================
// logger::GetColorForTag - auto-detect color from message content
// =============================================================================

ConsoleColor logger::GetColorForTag(const std::string& message)
{
	// Order matters: check more specific tags first

	// Diagnostic / crash - dim gray
	if (message.find("[diag]") != std::string::npos ||
		message.find("[crash]") != std::string::npos)
	{
		return ConsoleColor::DarkGray;
	}

	// Startup banner - bright white
	if (message.find("[startup]") != std::string::npos ||
		message.find("bjddumper starting") != std::string::npos ||
		message.find("bjddumper ready") != std::string::npos)
	{
		return ConsoleColor::BrightWhite;
	}

	// Memory mapping / remap operations - cyan
	if (message.find("[remap]") != std::string::npos ||
		message.find("[NtCreateSection]") != std::string::npos ||
		message.find("[NtMapViewOfSection]") != std::string::npos ||
		message.find("[NtUnmapViewOfSection]") != std::string::npos ||
		message.find("[NtProtectVirtualMemory]") != std::string::npos)
	{
		return ConsoleColor::BrightCyan;
	}

	// Library loading - yellow
	if (message.find("[loadlib]") != std::string::npos)
	{
		return ConsoleColor::BrightYellow;
	}

	// Target hook installation - green
	if (message.find("[target-hook]") != std::string::npos)
	{
		return ConsoleColor::BrightGreen;
	}

	// Parameter / data extraction - gray (subtle, data-focused)
	if (message.find("[param]") != std::string::npos)
	{
		return ConsoleColor::Gray;
	}

	// Packet construction - magenta
	if (message.find("[construct]") != std::string::npos)
	{
		return ConsoleColor::BrightMagenta;
	}

	// WinHTTP API operations - blue
	if (message.find("[api-winhttp]") != std::string::npos ||
		message.find("[winhttp]") != std::string::npos)
	{
		return ConsoleColor::BrightBlue;
	}

	// BCrypt / crypto operations - red
	if (message.find("[api-bcrypt]") != std::string::npos ||
		message.find("[bcrypt]") != std::string::npos)
	{
		return ConsoleColor::BrightRed;
	}

	// Java / JNI operations - green
	if (message.find("[java]") != std::string::npos ||
		message.find("[jni]") != std::string::npos)
	{
		return ConsoleColor::BrightGreen;
	}

	// Lazy importer hooks - dark cyan
	if (message.find("[lazy]") != std::string::npos)
	{
		return ConsoleColor::DarkCyan;
	}

	// RSA / crypto in Java - dark magenta
	if (message.find("[RSA") != std::string::npos ||
		message.find("[RSA_DATA]") != std::string::npos ||
		message.find("[MD5]") != std::string::npos ||
		message.find("[DIGEST]") != std::string::npos ||
		message.find("[STR]") != std::string::npos)
	{
		return ConsoleColor::DarkMagenta;
	}

	// Socket / network in Java - dark yellow
	if (message.find("[SOCKET]") != std::string::npos ||
		message.find("[PRINTLN]") != std::string::npos ||
		message.find("[READLINE") != std::string::npos ||
		message.find("[READLINE_RET]") != std::string::npos)
	{
		return ConsoleColor::DarkYellow;
	}

	// Hook installation status - dark yellow
	if (message.find(" hook ") != std::string::npos ||
		message.find("Hook") != std::string::npos ||
		message.find("hooks installed") != std::string::npos ||
		message.find("hooks already") != std::string::npos)
	{
		return ConsoleColor::DarkYellow;
	}

	// Error / failure - bright red
	if (message.find("failed") != std::string::npos ||
		message.find("error") != std::string::npos ||
		message.find("missing") != std::string::npos ||
		message.find("timeout") != std::string::npos ||
		message.find("unreadable") != std::string::npos)
	{
		return ConsoleColor::BrightRed;
	}

	// Default color
	return ConsoleColor::BrightWhite;
}

// =============================================================================
// logger::log_line - core logging function with color
// =============================================================================

void logger::log_line(const std::string& line)
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	if (HasColorCodes(line))
	{
		auto segments = ParseColorCodes(line);
		for (const auto& segment : segments)
		{
			WriteConsoleColoredTextNoLock(segment.text, segment.color);
		}
		WriteNewLineToLoggerConsole();
		AppendLineToDefaultFilesNoLock(StripColorCodes(line));
	}
	else
	{
		ConsoleColor color = GetColorForTag(line);
		WriteConsoleColoredTextNoLock(line, color);
		WriteNewLineToLoggerConsole();
		AppendLineToDefaultFilesNoLock(line);
	}
}

void logger::log_line(const char* line)
{
	if (line == nullptr)
	{
		log_line(std::string("(null)"));
	}
	else
	{
		log_line(std::string(line));
	}
}

// =============================================================================
// logger::Format - printf-style formatting
// =============================================================================

std::string logger::Format(const char* fmt, ...)
{
	if (fmt == nullptr)
	{
		return {};
	}

	char buffer[8192]{};
	va_list args;
	va_start(args, fmt);
	_vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, args);
	va_end(args);
	return std::string(buffer);
}

// =============================================================================
// logger::WriteColoredLine - explicit color output
// =============================================================================

void logger::WriteColoredLine(const std::string& message, ConsoleColor color)
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	WriteConsoleColoredTextNoLock(message, color);
	WriteNewLineToLoggerConsole();
	AppendLineToDefaultFilesNoLock(message);
}

void logger::WriteColoredChar(char ch, ConsoleColor color)
{
	std::string text(1, ch);
	WriteColoredText(text, color);
}

void logger::WriteColoredText(const std::string& text, ConsoleColor color)
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	WriteConsoleColoredTextNoLock(text, color);
	AppendLineToDefaultFilesNoLock(text);
}

void logger::WriteSegments(const std::vector<ConsoleTextSegment>& segments, bool newline)
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	for (const auto& segment : segments)
	{
		WriteConsoleColoredTextNoLock(segment.text, segment.color);
	}
	if (newline)
	{
		WriteNewLineToLoggerConsole();
	}
	AppendLineToDefaultFilesNoLock(FlattenSegments(segments));
}

void logger::WriteSegmentsLine(const std::vector<ConsoleTextSegment>& segments)
{
	WriteSegments(segments, true);
}

void logger::WriteRainbowLine(const std::string& message)
{
	static constexpr ConsoleColor colors[] = {
		ConsoleColor::BrightRed,
		ConsoleColor::BrightYellow,
		ConsoleColor::BrightGreen,
		ConsoleColor::BrightCyan,
		ConsoleColor::BrightBlue,
		ConsoleColor::BrightMagenta,
	};

	std::vector<ConsoleTextSegment> segments;
	segments.reserve(message.size());
	for (size_t i = 0; i < message.size(); ++i)
	{
		segments.push_back({ std::string(1, message[i]), colors[i % std::size(colors)] });
	}
	WriteSegmentsLine(segments);
}

// =============================================================================
// logger::WriteTaggedLine - colored tag prefix + default rest
// =============================================================================

void logger::WriteTaggedLine(const std::string& tag, ConsoleColor tagColor, const std::string& message)
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	WriteConsoleColoredTextNoLock("[" + tag + "] ", tagColor);
	WriteRawToLoggerConsole(message);
	WriteNewLineToLoggerConsole();
	AppendLineToDefaultFilesNoLock("[" + tag + "] " + message);
}

// =============================================================================
// logger::WriteLine - auto-detect color from message tags
// =============================================================================

void logger::WriteLine(const std::string& message)
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	if (HasColorCodes(message))
	{
		auto segments = ParseColorCodes(message);
		for (const auto& segment : segments)
		{
			WriteConsoleColoredTextNoLock(segment.text, segment.color);
		}
		WriteNewLineToLoggerConsole();
		AppendLineToDefaultFilesNoLock(StripColorCodes(message));
	}
	else
	{
		ConsoleColor color = GetColorForTag(message);
		WriteConsoleColoredTextNoLock(message, color);
		WriteNewLineToLoggerConsole();
		AppendLineToDefaultFilesNoLock(message);
	}
}

void logger::WriteLine(const char* message)
{
	WriteLine(std::string(message != nullptr ? message : "(null)"));
}

// =============================================================================
// logger::DumpOutputDirectory / paths
// =============================================================================

const std::string& logger::DumpOutputDirectory()
{
	static const std::string path = R"(D:\idk\dump)";
	return path;
}

std::string logger::BuildDumpOutputPath(const std::string& fileName)
{
	EnsureDumpOutputDirectoryExists();
	if (fileName.empty())
	{
		return DumpOutputDirectory() + "\\output.log";
	}
	return DumpOutputDirectory() + "\\" + fileName;
}

const std::string& logger::ConsoleLogFilePath()
{
	static const std::string path = BuildDumpOutputPath("console_log.log");
	return path;
}

const std::string& logger::TraceLogFilePath()
{
	static const std::string path = BuildDumpOutputPath("trace.log");
	return path;
}

// =============================================================================
// logger::AppendLineToPath / AppendLineToFile
// =============================================================================

void logger::AppendLineToPath(const std::string& path, const std::string& message)
{
	FILE* file = nullptr;
	fopen_s(&file, path.c_str(), "ab");
	if (file == nullptr)
	{
		return;
	}

	if (!message.empty())
	{
		fwrite(message.data(), 1, message.size(), file);
	}
	fwrite("\r\n", 1, 2, file);
	fclose(file);
}

void logger::AppendLineToFile(const std::string& message)
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	AppendLineToDefaultFilesNoLock(message);
}

// =============================================================================
// logger::EnsureDumpOutputDirectoryExists
// =============================================================================

void logger::EnsureDumpOutputDirectoryExists()
{
	static std::once_flag initFlag;
	std::call_once(initFlag, []() {
		CreateDirectoryA(R"(D:\idk\dump)", nullptr);
		CreateDirectoryA(DumpOutputDirectory().c_str(), nullptr);
	});
}

// =============================================================================
// logger::ParseColorCode - Minecraft-style color code to ConsoleColor
// =============================================================================

ConsoleColor logger::ParseColorCode(char code)
{
	// Minecraft-style color codes:
	// &0=Black  &1=DarkBlue  &2=DarkGreen  &3=DarkCyan
	// &4=DarkRed  &5=DarkMagenta  &6=DarkYellow  &7=Gray
	// &8=DarkGray  &9=BrightBlue  &a=BrightGreen  &b=BrightCyan
	// &c=BrightRed  &d=BrightMagenta  &e=BrightYellow  &f=BrightWhite
	// &r=Reset (same as BrightWhite / default)
	switch (code)
	{
	case '0': return ConsoleColor::Black;
	case '1': return ConsoleColor::DarkBlue;
	case '2': return ConsoleColor::DarkGreen;
	case '3': return ConsoleColor::DarkCyan;
	case '4': return ConsoleColor::DarkRed;
	case '5': return ConsoleColor::DarkMagenta;
	case '6': return ConsoleColor::DarkYellow;
	case '7': return ConsoleColor::Gray;
	case '8': return ConsoleColor::DarkGray;
	case '9': return ConsoleColor::BrightBlue;
	case 'a': case 'A': return ConsoleColor::BrightGreen;
	case 'b': case 'B': return ConsoleColor::BrightCyan;
	case 'c': case 'C': return ConsoleColor::BrightRed;
	case 'd': case 'D': return ConsoleColor::BrightMagenta;
	case 'e': case 'E': return ConsoleColor::BrightYellow;
	case 'f': case 'F': return ConsoleColor::BrightWhite;
	case 'r': case 'R': return ConsoleColor::BrightWhite;  // reset = default
	default:  return ConsoleColor::BrightWhite;
	}
}

// =============================================================================
// logger::ParseColorCodes - parse &0-&f, &r codes into segments
// =============================================================================

std::vector<ConsoleTextSegment> logger::ParseColorCodes(const std::string& message)
{
	std::vector<ConsoleTextSegment> segments;
	ConsoleColor currentColor = ConsoleColor::BrightWhite;
	std::string currentText;

	for (size_t i = 0; i < message.size(); ++i)
	{
		if (message[i] == '&' && i + 1 < message.size() && IsColorCodeChar(message[i + 1]))
		{
			// Flush current text as a segment
			if (!currentText.empty())
			{
				segments.push_back({ currentText, currentColor });
				currentText.clear();
			}
			currentColor = ParseColorCode(message[i + 1]);
			++i;  // skip the color code character
		}
		else
		{
			currentText += message[i];
		}
	}

	// Flush remaining text
	if (!currentText.empty())
	{
		segments.push_back({ currentText, currentColor });
	}

	return segments;
}

// =============================================================================
// logger::StripColorCodes - remove &0-&f, &r codes from string
// =============================================================================

std::string logger::StripColorCodes(const std::string& message)
{
	std::string result;
	result.reserve(message.size());

	for (size_t i = 0; i < message.size(); ++i)
	{
		if (message[i] == '&' && i + 1 < message.size() && IsColorCodeChar(message[i + 1]))
		{
			++i;  // skip the color code character
		}
		else
		{
			result += message[i];
		}
	}

	return result;
}
