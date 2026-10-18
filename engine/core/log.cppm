export module ember.core:log;

import std;

export namespace ember::core
{

/// Importance of a log message, from the least to the most important.
enum class LogLevel
{
    /// Detailed messages for following the program step by step.
    Trace,
    /// Messages that help to find bugs.
    Debug,
    /// Normal events: a plugin is built, a scene is loaded.
    Info,
    /// Something unexpected that the engine can recover from.
    Warning,
    /// A failure: an operation could not be completed.
    Error
};

/// Receives every log message that passes the level filter.
using LogSink = std::function<void(LogLevel level, std::string_view message)>;

/// Identifies a sink added with `add_log_sink()`.
enum class LogSinkId : std::uint32_t
{
};

/// Sets the lowest level of messages that are written. Messages below it are dropped.
/// The default level is `LogLevel::Info`.
/// @param level Lowest level to write.
void set_log_level(LogLevel level);

/// Returns the lowest level of messages that are written.
LogLevel log_level();

/// Adds a sink that receives all log messages, for example the debug console.
/// Messages are also printed to the standard error stream.
/// @param sink Function called for every message.
/// @return Id for removing the sink.
/// @note The sink may be called from any thread, but never from two threads at once.
LogSinkId add_log_sink(LogSink sink);

/// Removes a sink added with `add_log_sink()`. After the call returns, the sink is not
/// called anymore.
/// @param id Id returned by `add_log_sink()`.
void remove_log_sink(LogSinkId id);

/// Writes a message to all sinks if its level passes the filter.
/// @param level Level of the message.
/// @param message Text of the message.
/// @note Can be called from any thread.
void write_log(LogLevel level, std::string_view message);

/// Formats a message with `std::format` and writes it with `LogLevel::Trace`.
/// The message is not formatted if the level does not pass the filter.
/// @tparam Args Types of the format arguments.
/// @param format Format string.
/// @param args Values inserted into the format string.
template <typename... Args>
void log_trace(std::format_string<Args...> format, Args&&... args);

/// Formats a message with `std::format` and writes it with `LogLevel::Debug`.
/// The message is not formatted if the level does not pass the filter.
/// @tparam Args Types of the format arguments.
/// @param format Format string.
/// @param args Values inserted into the format string.
template <typename... Args>
void log_debug(std::format_string<Args...> format, Args&&... args);

/// Formats a message with `std::format` and writes it with `LogLevel::Info`.
/// The message is not formatted if the level does not pass the filter.
/// @tparam Args Types of the format arguments.
/// @param format Format string.
/// @param args Values inserted into the format string.
template <typename... Args>
void log_info(std::format_string<Args...> format, Args&&... args);

/// Formats a message with `std::format` and writes it with `LogLevel::Warning`.
/// The message is not formatted if the level does not pass the filter.
/// @tparam Args Types of the format arguments.
/// @param format Format string.
/// @param args Values inserted into the format string.
template <typename... Args>
void log_warning(std::format_string<Args...> format, Args&&... args);

/// Formats a message with `std::format` and writes it with `LogLevel::Error`.
/// The message is not formatted if the level does not pass the filter.
/// @tparam Args Types of the format arguments.
/// @param format Format string.
/// @param args Values inserted into the format string.
template <typename... Args>
void log_error(std::format_string<Args...> format, Args&&... args);

} // namespace ember::core
