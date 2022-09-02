// CommandLine.cpp : 
//

#include "CommandLine.hpp"


#pragma region Functions

#pragma region Win32 Argument Parsing

// DOCS: Detailed rules on how arguments are parsed, escaped, and split.
// <https://docs.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=msvc-170>

static bool WIN32_IsWhiteSpaceChar(char c)
{
	return (c == '\t' || c == '\n' || c == '\v' || c == ' ');
}

static void WIN32_ParseProgramArgument(const std::string& str, IN OUT size_t& i, OUT std::string& arg)
{
	arg.clear();

	// First argument (for program name) is much cleaner to parse, and follows slightly different rules.
	bool quoted = false;

	const size_t length = str.length();

	for (; i < length; i++) {
		const char c = str[i];

		if (c == '\"') {
			// Toggle quoted state.
			quoted = !quoted;
		}
		else if (!quoted && CommandLine::IsWhiteSpaceChar(c)) {
			break; // Unquoted whitespace, end of argument.
		}
		else {
			arg.append(1, c);
		}
	}
}

static void WIN32_ParseNextArgument(const std::string& str, IN OUT size_t& i, OUT std::string& arg)
{
	arg.clear();

	// Consecutive backslashes, for handling with quotes.
	size_t numBackslashes = 0;
	// Trailing quoted argument behaves as if quotes are closed at the end, so no special handling is needed.
	bool quoted = false;

	const size_t length = str.length();

	for (; i < length; i++) {
		const bool isEnd = (i+1 == length);
		const char c     = str[i];

		if (c == '\\') {
			// Count number of backslashes, because we need to flush them differently depending on quote characters.
			numBackslashes++;
			// We don't know how to treat backslashes until we encounter a non-backslash character.
			// Backslash escapes are not affected by the quoted state.
			if (isEnd || str[i+1] != '\\') {
				if (!isEnd && str[i+1] == '\"') {
					// Repeated backslashes followed by a quote are escaped.
					arg.append(numBackslashes / 2, '\\');

					if ((numBackslashes % 2) == 1) {
						// If an odd number of backslashes exist, then the quote is escaped.
						arg.append(1, '\"');
						i++; // Consume and skip the escaped character.
					}
					else {
						// Otherwise don't consume the quote character, and handle it like normal next loop.
					}
				}
				else {
					// Backslashes followed by any other character are not escaped.
					// This also flushes backslashes at the end of the string.
					arg.append(numBackslashes, '\\');
				}
				numBackslashes = 0;
			}
		}
		else if (c == '\"') {
			// We don't need to count number of quotes, because no other character changes how they're escaped.
			if (!quoted) {
				// Begin quoted state.
				quoted = true;
			}
			else {
				// We don't know how to treat quotes while in the quoted state until the next character.
				if (!isEnd && str[i+1] == '\"') {
					// Repeated quotes are escaped while in the quoted state.
					arg.append(1, '\"');
					i++; // Consume and skip the escaped character.
				}
				else {
					// Unescaped quote. End quoted state.
					// Trailing quoted argument behaves as if quotes are closed at the end, so don't flush.
					quoted = false;
				}
			}
		}
		else if (!quoted && CommandLine::IsWhiteSpaceChar(c)) {
			break; // Unquoted whitespace, end of argument.
		}
		else {
			arg.append(1, c);
		}
	}
}

#pragma endregion


bool CommandLine::IsWhiteSpaceChar(char c)
{
#ifdef _WIN32
	return WIN32_IsWhiteSpaceChar(c);
#else
#error CommandLine::IsWhiteSpaceChar not defined for other operating systems.
#endif
}

void CommandLine::SkipWhiteSpace(const std::string& str, IN OUT size_t& i)
{
	const size_t length = str.length();

	while (i < length && IsWhiteSpaceChar(str[i])) i++;
}


void CommandLine::ParseProgramArgument(const std::string& str, IN OUT size_t& index, OUT std::string& arg)
{
#ifdef _WIN32
	WIN32_ParseProgramArgument(str, index, arg);
#else
#error CommandLine::ParseProgramArgument not defined for other operating systems.
#endif
}


void CommandLine::ParseNextArgument(const std::string& str, IN OUT size_t& index, OUT std::string& arg)
{
#ifdef _WIN32
	WIN32_ParseNextArgument(str, index, arg);
#else
#error CommandLine::ParseNextArgument not defined for other operating systems.
#endif
}


void CommandLine::SplitArguments(const std::string& str, IN OUT std::vector<std::string>& arguments, bool hasProgramName, bool skipProgramName)
{
	// DOCS: Detailed rules on how arguments are parsed, escaped, and split.
	// <https://docs.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=msvc-170>

	// 4Q\\\\"""hello world"

	size_t i = 0;

	const size_t length = str.length();

	// Parse program name argument:
	// Program name arguments may have special parsing rules depending on the OS.
	if (hasProgramName) {
		std::string arg;
		ParseProgramArgument(str, i, arg);
		if (!skipProgramName) {
			arguments.push_back(arg);
		}
	}

	// Skip whitespace before next argument:
	// Do this before the loop in order to check (i < length) so that we don't parse an empty argument.
	SkipWhiteSpace(str, i);

	// Parse remaining arguments:
	while (i < length) {
		std::string arg;
		ParseNextArgument(str, i, arg);
		arguments.push_back(arg);

		// Skip whitespace before next argument:
		SkipWhiteSpace(str, i);
	}
}


bool CommandLine::ArgumentEquals(const std::string& arg, const std::string& name)
{
	return (::_stricmp(arg.c_str(), name.c_str()) == 0);
}

bool CommandLine::ArgumentStartsWith(const std::string& arg, const std::string& name)
{
	return (::_strnicmp(arg.c_str(), name.c_str(), name.length()) == 0);
}

size_t CommandLine::IndexOfOption(const std::vector<std::string>& arguments, const std::string& name)
{
	for (size_t i = 0; i < arguments.size(); i++) {
		if (ArgumentEquals(arguments[i], name)) {
			return i;
		}
	}
	return CommandLine::npos;
}

size_t CommandLine::IndexOfOptionStartsWith(const std::vector<std::string>& arguments, const std::string& name)
{
	for (size_t i = 0; i < arguments.size(); i++) {
		if (ArgumentStartsWith(arguments[i], name)) {
			return i;
		}
	}
	return CommandLine::npos;
}

bool CommandLine::FindOption(const std::vector<std::string>& arguments, const std::string& name)
{
	return (IndexOfOption(arguments, name) != CommandLine::npos);
}

bool CommandLine::FindOptionStartsWith(const std::vector<std::string>& arguments, const std::string& name)
{
	return (IndexOfOptionStartsWith(arguments, name) != CommandLine::npos);
}

bool CommandLine::FindParameter(const std::vector<std::string>& arguments, const std::string& name, OUT std::string& parameter)
{
	parameter.clear();

	size_t i = IndexOfOption(arguments, name);
	if (i != CommandLine::npos) {
		i++;
		if (i < arguments.size()) {
			parameter = arguments[i];
			return true; // Found the parameter.
		}
		else {
			return false; // Missing the parameter.
		}
	}
	return false;
}

bool CommandLine::FindParameters(const std::vector<std::string>& arguments, const std::string& name, OUT std::vector<std::string>& parameters, size_t count)
{
	parameters.clear();

	size_t i = IndexOfOption(arguments, name);
	if (i != CommandLine::npos) {
		i++;
		for (size_t j = 0; j < count; i++, j++) {
			if (i < arguments.size()) {
				parameters.push_back(arguments[i]);
			}
			else {
				//parameters.push_back("");
				return false; // Missing some parameters.
			}
		}
		return true; // Found all parameters.
	}
	return false;
}

#pragma endregion
