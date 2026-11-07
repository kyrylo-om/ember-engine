export module ember.platform:input;

import ember.common;
import std;

export namespace ember::platform
{

/// Keys of the keyboard, named after their place on a US layout.
enum class Key
{
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Space,
    Enter,
    Escape,
    Backspace,
    Tab,
    Delete,
    Insert,
    Home,
    End,
    PageUp,
    PageDown,
    Left,
    Right,
    Up,
    Down,
    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,
    /// The key left of 1, usually marked with a backtick and a tilde.
    Grave,
    Minus,
    Equal,
    LeftBracket,
    RightBracket,
    Backslash,
    Semicolon,
    Apostrophe,
    Comma,
    Period,
    Slash
};

/// Number of values of `Key`.
inline constexpr std::size_t key_count = static_cast<std::size_t>(Key::Slash) + 1;

/// Buttons of the mouse.
enum class MouseButton
{
    Left,
    Right,
    Middle
};

/// Number of values of `MouseButton`.
inline constexpr std::size_t mouse_button_count = static_cast<std::size_t>(MouseButton::Middle) + 1;

/// State of the keyboard and the mouse, and actions bound to keys and buttons.
///
/// An action is a name chosen by the game, such as "jump", bound to any number of keys and
/// mouse buttons. It is down while any of them is down, so the game asks for actions
/// instead of keys and the bindings can change without changing the game code.
///
/// The state changes once per frame: the platform plugin calls `begin_frame()` and then
/// sets the state of every key and button in `Stage::PreUpdate`, so all systems of a frame
/// see the same state. Tests and replays can set the state the same way.
class Input
{
public:
    /// Returns true while a key is held down.
    bool is_key_down(Key key) const;

    /// Returns true if a key went down in this frame.
    bool is_key_pressed(Key key) const;

    /// Returns true if a key went up in this frame.
    bool is_key_released(Key key) const;

    /// Returns true while a mouse button is held down.
    bool is_mouse_down(MouseButton button) const;

    /// Returns true if a mouse button went down in this frame.
    bool is_mouse_pressed(MouseButton button) const;

    /// Returns true if a mouse button went up in this frame.
    bool is_mouse_released(MouseButton button) const;

    /// Returns the position of the mouse in pixels from the top-left corner of the window.
    Vec2 mouse_position() const;

    /// Returns how far the mouse moved in this frame, in pixels.
    Vec2 mouse_delta() const;

    /// Returns how far the mouse wheel turned in this frame; positive values mean away from
    /// the user.
    float mouse_wheel() const;

    /// Returns the text typed in this frame in UTF-8, with the keyboard layout and Shift
    /// applied. Used for text fields such as the debug console.
    std::string_view text() const;

    /// Binds a key to an action. A key can be bound to several actions.
    /// @param action Name of the action.
    /// @param key Key that triggers the action.
    void bind(std::string_view action, Key key);

    /// Binds a mouse button to an action. A button can be bound to several actions.
    /// @param action Name of the action.
    /// @param button Button that triggers the action.
    void bind(std::string_view action, MouseButton button);

    /// Removes all keys and buttons bound to an action.
    /// @param action Name of the action. Nothing happens if it has no bindings.
    void unbind(std::string_view action);

    /// Returns true while any key or button of an action is held down. False for an
    /// action without bindings.
    bool is_action_down(std::string_view action) const;

    /// Returns true if an action went from up to down in this frame.
    bool is_action_pressed(std::string_view action) const;

    /// Returns true if an action went from down to up in this frame.
    bool is_action_released(std::string_view action) const;

    /// Combines two opposite actions into one value, for example "left" and "right".
    /// @param negative Action that gives -1.
    /// @param positive Action that gives 1.
    /// @return -1, 0 or 1; 0 if both or none of the actions are down.
    float axis(std::string_view negative, std::string_view positive) const;

    /// Starts the state of a new frame: the current state becomes the previous one, and the
    /// mouse movement, wheel and text are cleared.
    void begin_frame();

    /// Sets whether a key is down in this frame.
    void set_key_down(Key key, bool is_down);

    /// Sets whether a mouse button is down in this frame.
    void set_mouse_down(MouseButton button, bool is_down);

    /// Sets the position of the mouse; the difference to the previous frame becomes
    /// `mouse_delta()`.
    void set_mouse_position(Vec2 position);

    /// Adds to the turn of the mouse wheel in this frame.
    void add_mouse_wheel(float amount);

    /// Adds text typed in this frame.
    /// @param text Text in UTF-8.
    void add_text(std::string_view text);

private:
    // Keys and buttons bound to one action.
    struct Binding
    {
        std::vector<Key> keys{};
        std::vector<MouseButton> buttons{};
    };

    bool was_action_down(std::string_view action) const;

    std::bitset<key_count> m_keys{};
    std::bitset<key_count> m_previous_keys{};
    std::bitset<mouse_button_count> m_mouse{};
    std::bitset<mouse_button_count> m_previous_mouse{};
    Vec2 m_mouse_position{};
    Vec2 m_previous_mouse_position{};
    float m_mouse_wheel = 0.0f;
    std::string m_text{};
    // std::less<> allows lookup by std::string_view without creating a std::string.
    std::map<std::string, Binding, std::less<>> m_bindings{};
};

} // namespace ember::platform
