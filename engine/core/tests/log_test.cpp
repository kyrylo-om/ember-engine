#include <catch_amalgamated.hpp>

import ember.core;
import std;

using ember::core::add_log_sink;
using ember::core::log_debug;
using ember::core::log_error;
using ember::core::log_info;
using ember::core::log_level;
using ember::core::log_trace;
using ember::core::log_warning;
using ember::core::LogLevel;
using ember::core::LogSinkId;
using ember::core::remove_log_sink;
using ember::core::set_log_level;
using ember::core::write_log;

namespace
{

struct Message
{
    LogLevel level = LogLevel::Info;
    std::string text{};
};

// Collects the messages that reach its sink. The log is global, so the destructor restores
// the default state for the next test.
class CapturedLog
{
public:
    CapturedLog()
        : m_id{add_log_sink([this](LogLevel level, std::string_view text)
                            { m_messages.push_back({level, std::string{text}}); })}
    {
    }

    ~CapturedLog()
    {
        remove_log_sink(m_id);
        set_log_level(LogLevel::Info);
    }

    CapturedLog(const CapturedLog&) = delete;
    CapturedLog& operator=(const CapturedLog&) = delete;
    CapturedLog(CapturedLog&&) = delete;
    CapturedLog& operator=(CapturedLog&&) = delete;

    const std::vector<Message>& messages() const
    {
        return m_messages;
    }

private:
    // Declared before m_id, so it exists before the sink that writes to it is added.
    std::vector<Message> m_messages{};
    LogSinkId m_id{};
};

// Formats as the number of times it has been formatted, to show whether formatting happened.
struct FormatCounter
{
    int* count = nullptr;
};

} // namespace

template <>
struct std::formatter<FormatCounter> : std::formatter<int>
{
    auto format(const FormatCounter& counter, std::format_context& context) const
    {
        return std::formatter<int>::format(++*counter.count, context);
    }
};

TEST_CASE("messages below the log level are dropped")
{
    const CapturedLog log;
    set_log_level(LogLevel::Warning);

    write_log(LogLevel::Info, "dropped");
    write_log(LogLevel::Warning, "kept");
    write_log(LogLevel::Error, "also kept");

    REQUIRE(log.messages().size() == 2);
    CHECK(log.messages()[0].text == "kept");
    CHECK(log.messages()[1].level == LogLevel::Error);
}

TEST_CASE("the log level can be read back")
{
    const CapturedLog log;
    set_log_level(LogLevel::Debug);

    CHECK(log_level() == LogLevel::Debug);
}

TEST_CASE("log functions format the message and write it with their level")
{
    const CapturedLog log;
    set_log_level(LogLevel::Trace);

    log_trace("trace");
    log_debug("debug");
    log_info("info");
    log_warning("warning");
    log_error("{} + {} = {}", 1, 2, 3);

    REQUIRE(log.messages().size() == 5);
    CHECK(log.messages()[0].level == LogLevel::Trace);
    CHECK(log.messages()[1].level == LogLevel::Debug);
    CHECK(log.messages()[2].level == LogLevel::Info);
    CHECK(log.messages()[3].level == LogLevel::Warning);
    CHECK(log.messages()[4].level == LogLevel::Error);
    CHECK(log.messages()[4].text == "1 + 2 = 3");
}

TEST_CASE("messages below the log level are not formatted")
{
    const CapturedLog log;
    int format_count = 0;

    log_debug("{}", FormatCounter{&format_count});
    CHECK(format_count == 0);

    log_info("{}", FormatCounter{&format_count});
    CHECK(format_count == 1);
    CHECK(log.messages().back().text == "1");
}

TEST_CASE("every sink receives every message")
{
    const CapturedLog first;
    const CapturedLog second;

    write_log(LogLevel::Info, "message");

    CHECK(first.messages().size() == 1);
    CHECK(second.messages().size() == 1);
}

TEST_CASE("a removed sink receives no more messages")
{
    int call_count = 0;
    const LogSinkId id = add_log_sink([&call_count](LogLevel, std::string_view) { ++call_count; });

    write_log(LogLevel::Info, "before removing");
    remove_log_sink(id);
    write_log(LogLevel::Info, "after removing");

    CHECK(call_count == 1);
}

TEST_CASE("sinks are never called from two threads at once")
{
    constexpr int thread_count = 4;
    constexpr int messages_per_thread = 100;

    std::atomic<bool> is_inside_sink{false};
    std::atomic<bool> has_overlap{false};
    // Not atomic on purpose: only the lock of the log protects it.
    int call_count = 0;
    const LogSinkId id = add_log_sink(
        [&](LogLevel, std::string_view)
        {
            if (is_inside_sink.exchange(true))
            {
                has_overlap = true;
            }
            ++call_count;
            is_inside_sink = false;
        });

    {
        std::vector<std::jthread> threads;
        for (int thread = 0; thread < thread_count; ++thread)
        {
            threads.emplace_back(
                []
                {
                    for (int message = 0; message < messages_per_thread; ++message)
                    {
                        write_log(LogLevel::Info, "message from a thread");
                    }
                });
        }
    }
    remove_log_sink(id);

    CHECK_FALSE(has_overlap);
    CHECK(call_count == thread_count * messages_per_thread);
}
