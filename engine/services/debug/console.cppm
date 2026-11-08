export module ember.debug:console;

import ember.ecs;
import std;

export namespace ember::debug
{

/// Result of a console command: the text to show on success, or an error message.
using CommandResult = std::expected<std::string, std::string>;

/// Function that runs a console command.
///
/// It gets the world and the words that follow the command name, split at spaces.
using CommandHandler = std::move_only_function<CommandResult(
    ecs::World& world, std::span<const std::string_view> arguments)>;

/// Name and description of a console command, as listed by the `help` command.
struct CommandInfo
{
    /// Name typed to run the command.
    std::string_view name{};
    /// One line that says what the command does and which arguments it takes.
    std::string_view description{};
};

/// Runs text commands typed in the terminal or in the debug overlay.
///
/// A command line is a command name followed by arguments separated by spaces, for example
/// `inspect player`. Lines from the terminal arrive on a background thread, so they are
/// queued with `submit()` and run on the main thread by `run_pending()`.
class Console
{
public:
    /// Creates a console without commands that does not read the terminal.
    Console();

    /// Stops reading the terminal.
    ~Console();

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    /// Takes over the commands, the queued lines and the terminal reader.
    Console(Console&& other) noexcept;

    /// Stops reading the terminal, then takes over everything from `other`.
    Console& operator=(Console&& other) noexcept;

    /// Adds a command, replacing a command with the same name.
    /// @param name Name typed to run the command; a single word.
    /// @param description One line shown by the `help` command.
    /// @param handler Function that runs the command.
    void add_command(std::string_view name, std::string_view description, CommandHandler handler);

    /// Returns all commands sorted by name.
    std::vector<CommandInfo> commands() const;

    /// Runs a command line at once.
    /// @param world World passed to the command.
    /// @param line Command name and arguments.
    /// @return Result of the command; an error if the line names no known command. An empty
    /// line gives an empty text.
    [[nodiscard]] CommandResult execute(ecs::World& world, std::string_view line);

    /// Queues a command line to run in the next `run_pending()`.
    /// @param line Command name and arguments.
    /// @note Can be called from any thread.
    void submit(std::string line);

    /// Runs all queued lines in the order they were submitted and writes every result to
    /// the log: texts as info, errors as warnings. The debug plugin calls it in
    /// `Stage::PreUpdate`.
    /// @param world World passed to the commands.
    void run_pending(ecs::World& world);

    /// Starts reading lines from the standard input on a background thread and submitting
    /// them. Nothing happens if the terminal is already read.
    void start_terminal();

    /// Stops reading the standard input. A line that is typed but not finished is dropped.
    void stop_terminal();

private:
    // Holds the mutex and the reader thread, which need a stable address while the console
    // itself is moved into the world.
    struct State;
    std::unique_ptr<State> m_state;
};

} // namespace ember::debug
