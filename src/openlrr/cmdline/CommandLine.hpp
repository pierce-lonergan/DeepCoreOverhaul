// CommandLine.hpp : Utility functions for parsing and processing command line arguments.
//

#pragma once

#include "../common.h"


namespace CommandLine
{

/**
 * @brief Result returned by IndexOf functions when the item is not found.
 */
static constexpr const size_t npos = static_cast<size_t>(-1);

#pragma region Functions

/**
 * @brief Returns if the character is classified as whitespace by the command line.
 */
bool IsWhiteSpaceChar(char c);

/**
 * @brief Skips continuous whitespace characters in a command line string.
 * @param str The entire command line arguments string.
 * @param index The input index before the whitespace, and output index after the whitespace.
 */
void SkipWhiteSpace(const std::string& str, IN OUT size_t& index);

/**
 * @brief Parses the first argument in a command line string that refers to the program path.
 * @param str The entire command line arguments string.
 * @param index The input index before the argument, and output index after the argument.
 * @param arg The output argument string.
 */
void ParseProgramArgument(const std::string& str, IN OUT size_t& index, OUT std::string& arg);

/**
 * @brief Parses the next argument in a command line string that does not refer to a program path.
 * @param str The entire command line arguments string.
 * @param index The input index before the argument, and output index after the argument.
 * @param arg The output argument string.
 */
void ParseNextArgument(const std::string& str, IN OUT size_t& index, OUT std::string& arg);

/**
 * @brief Splits and unescapes a continuous string of all command line arguments into individual arguments.
 * @param str The input command line arguments string.
 * @param arguments The collection where parsed arguments are appended to (arguments IS NOT cleared).
 * @param hasProgramName When true, the first argument is the program name, which follows different parsing rules.
 * @param skipProgramName When true and hasProgramName is true, the first argument will not be added to the list.
 */
void SplitArguments(const std::string& str, IN OUT std::vector<std::string>& arguments, bool hasProgramName, bool skipProgramName = false);


/**
 * @brief Checks if arg and name are equal (case-insensitive).
 */
bool ArgumentEquals(const std::string& arg, const std::string& name);

/**
 * @brief Checks if arg starts with name (case-insensitive).
 */
bool ArgumentStartsWith(const std::string& arg, const std::string& name);

/**
 * @brief Searches the arguments list for an argument matching name.
 * @param arguments The list of split command line arguments.
 * @param name The name an argument needs to match.
 * @return The index of the argument, or npos if not found.
 */
size_t IndexOfOption(const std::vector<std::string>& arguments, const std::string& name);

/**
 * @brief Searches the arguments list for an argument starting with name.
 * @param arguments The list of split command line arguments.
 * @param name The name an argument needs to start with.
 * @return The index of the argument, or npos if not found.
 */
size_t IndexOfOptionStartsWith(const std::vector<std::string>& arguments, const std::string& name);

/**
 * @brief Searches the arguments list for an argument matching name.
 * @param arguments The list of split command line arguments.
 * @param name The name an argument needs to match.
 * @return True if the argument was found.
 */
bool FindOption(const std::vector<std::string>& arguments, const std::string& name);

/**
 * @brief Searches the arguments list for an argument starting with name.
 * @param arguments The list of split command line arguments.
 * @param name The name an argument needs to start with.
 * @return True if the argument was found.
 */
bool FindOptionStartsWith(const std::vector<std::string>& arguments, const std::string& name);

/**
 * @brief Searches the arguments list for an argument matching name, and stores the following argument.
 * @param arguments The list of split command line arguments.
 * @param name The name an argument needs to match.
 * @param parameter The output argument found after name.
 * @return True if the argument followed by one parameter was found.
 */
bool FindParameter(const std::vector<std::string>& arguments, const std::string& name, OUT std::string& parameter);

/**
 * @brief Searches the arguments list for an argument matching name, and stores the following arguments.
 * @param arguments The list of split command line arguments.
 * @param name The name an argument needs to match.
 * @param parameters The output arguments found after name.
 * @param count The number of parameters to store after name.
 * @return True if the argument followed by count parameters was found.
 */
bool FindParameters(const std::vector<std::string>& arguments, const std::string& name, OUT std::vector<std::string>& parameters, size_t numParameters);

#pragma endregion

}
