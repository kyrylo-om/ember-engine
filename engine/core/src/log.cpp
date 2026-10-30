module ember.core;

import std;

namespace ember::core
{

namespace
{

struct SinkEntry
{
    LogSinkId id{};
    LogSink sink{};
};

// Atomic, so every log call can check the filter without taking the lock.
std::atomic<LogLevel> current_level{LogLevel::Info};

// Guards the sinks and the standard error stream, so sinks never run on two threads at once
// and lines written by different threads do not mix.
std::mutex sinks_mutex;
std::vector<SinkEntry> sinks;
// Ids start from 1, so a default-constructed LogSinkId never names a sink.
std::uint32_t last_sink_id = 0;

std::string_view level_name(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Trace:
            return "trace";
        case LogLevel::Debug:
            return "debug";
        case LogLevel::Info:
            return "info";
        case LogLevel::Warning:
            return "warning";
        case LogLevel::Error:
            return "error";
    }
    std::unreachable();
}

} // namespace

void set_log_level(LogLevel level)
{
    current_level.store(level, std::memory_order_relaxed);
}

LogLevel log_level()
{
    return current_level.load(std::memory_order_relaxed);
}

LogSinkId add_log_sink(LogSink sink)
{
    const std::scoped_lock lock{sinks_mutex};
    const auto id = static_cast<LogSinkId>(++last_sink_id);
    sinks.push_back({id, std::move(sink)});
    return id;
}

void remove_log_sink(LogSinkId id)
{
    const std::scoped_lock lock{sinks_mutex};
    std::erase_if(sinks, [id](const SinkEntry& entry) { return entry.id == id; });
}

void write_log(LogLevel level, std::string_view message)
{
    if (level < log_level())
    {
        return;
    }

    const std::scoped_lock lock{sinks_mutex};
    std::println(std::cerr, "[{}] {}", level_name(level), message);
    for (const SinkEntry& entry : sinks)
    {
        entry.sink(level, message);
    }
}

} // namespace ember::core
