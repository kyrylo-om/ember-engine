module ember.platform;

import :raylib_support;
import ember.common;
import ember.core;
import ember.ecs;
import raylib;
import std;

namespace ember::platform
{

namespace
{

using RaylibKey = raylib::KeyboardKey;

// Raylib key of every Key, in the order of the enumerators of Key.
constexpr std::array raylib_keys{
    RaylibKey::KEY_A,
    RaylibKey::KEY_B,
    RaylibKey::KEY_C,
    RaylibKey::KEY_D,
    RaylibKey::KEY_E,
    RaylibKey::KEY_F,
    RaylibKey::KEY_G,
    RaylibKey::KEY_H,
    RaylibKey::KEY_I,
    RaylibKey::KEY_J,
    RaylibKey::KEY_K,
    RaylibKey::KEY_L,
    RaylibKey::KEY_M,
    RaylibKey::KEY_N,
    RaylibKey::KEY_O,
    RaylibKey::KEY_P,
    RaylibKey::KEY_Q,
    RaylibKey::KEY_R,
    RaylibKey::KEY_S,
    RaylibKey::KEY_T,
    RaylibKey::KEY_U,
    RaylibKey::KEY_V,
    RaylibKey::KEY_W,
    RaylibKey::KEY_X,
    RaylibKey::KEY_Y,
    RaylibKey::KEY_Z,
    RaylibKey::KEY_ZERO,
    RaylibKey::KEY_ONE,
    RaylibKey::KEY_TWO,
    RaylibKey::KEY_THREE,
    RaylibKey::KEY_FOUR,
    RaylibKey::KEY_FIVE,
    RaylibKey::KEY_SIX,
    RaylibKey::KEY_SEVEN,
    RaylibKey::KEY_EIGHT,
    RaylibKey::KEY_NINE,
    RaylibKey::KEY_F1,
    RaylibKey::KEY_F2,
    RaylibKey::KEY_F3,
    RaylibKey::KEY_F4,
    RaylibKey::KEY_F5,
    RaylibKey::KEY_F6,
    RaylibKey::KEY_F7,
    RaylibKey::KEY_F8,
    RaylibKey::KEY_F9,
    RaylibKey::KEY_F10,
    RaylibKey::KEY_F11,
    RaylibKey::KEY_F12,
    RaylibKey::KEY_SPACE,
    RaylibKey::KEY_ENTER,
    RaylibKey::KEY_ESCAPE,
    RaylibKey::KEY_BACKSPACE,
    RaylibKey::KEY_TAB,
    RaylibKey::KEY_DELETE,
    RaylibKey::KEY_INSERT,
    RaylibKey::KEY_HOME,
    RaylibKey::KEY_END,
    RaylibKey::KEY_PAGE_UP,
    RaylibKey::KEY_PAGE_DOWN,
    RaylibKey::KEY_LEFT,
    RaylibKey::KEY_RIGHT,
    RaylibKey::KEY_UP,
    RaylibKey::KEY_DOWN,
    RaylibKey::KEY_LEFT_SHIFT,
    RaylibKey::KEY_RIGHT_SHIFT,
    RaylibKey::KEY_LEFT_CONTROL,
    RaylibKey::KEY_RIGHT_CONTROL,
    RaylibKey::KEY_LEFT_ALT,
    RaylibKey::KEY_RIGHT_ALT,
    RaylibKey::KEY_GRAVE,
    RaylibKey::KEY_MINUS,
    RaylibKey::KEY_EQUAL,
    RaylibKey::KEY_LEFT_BRACKET,
    RaylibKey::KEY_RIGHT_BRACKET,
    RaylibKey::KEY_BACKSLASH,
    RaylibKey::KEY_SEMICOLON,
    RaylibKey::KEY_APOSTROPHE,
    RaylibKey::KEY_COMMA,
    RaylibKey::KEY_PERIOD,
    RaylibKey::KEY_SLASH,
};
static_assert(raylib_keys.size() == key_count, "every Key needs a Raylib key");

// Raylib button of every MouseButton, in the order of the enumerators of MouseButton.
constexpr std::array raylib_buttons{
    raylib::MouseButton::MOUSE_BUTTON_LEFT,
    raylib::MouseButton::MOUSE_BUTTON_RIGHT,
    raylib::MouseButton::MOUSE_BUTTON_MIDDLE,
};
static_assert(raylib_buttons.size() == mouse_button_count,
              "every MouseButton needs a Raylib button");

// Appends a Unicode character to text in UTF-8.
void append_utf8(std::string& text, std::uint32_t character)
{
    // Largest characters that fit into one, two and three bytes.
    constexpr std::uint32_t one_byte_max = 0x7F;
    constexpr std::uint32_t two_bytes_max = 0x7FF;
    constexpr std::uint32_t three_bytes_max = 0xFFFF;
    // Every byte after the first starts with the bits 10 and carries six bits.
    constexpr std::uint32_t continuation_marker = 0x80;
    constexpr std::uint32_t continuation_bits = 0x3F;
    constexpr int continuation_shift = 6;
    // The first byte of a longer character starts with as many 1 bits as it has bytes.
    constexpr std::uint32_t two_bytes_marker = 0xC0;
    constexpr std::uint32_t three_bytes_marker = 0xE0;
    constexpr std::uint32_t four_bytes_marker = 0xF0;

    const auto continuation = [character](int index)
    {
        return static_cast<char>(continuation_marker |
                                 ((character >> (continuation_shift * index)) & continuation_bits));
    };

    if (character <= one_byte_max)
    {
        text.push_back(static_cast<char>(character));
    }
    else if (character <= two_bytes_max)
    {
        text.push_back(static_cast<char>(two_bytes_marker | (character >> continuation_shift)));
        text.push_back(continuation(0));
    }
    else if (character <= three_bytes_max)
    {
        text.push_back(
            static_cast<char>(three_bytes_marker | (character >> (continuation_shift * 2))));
        text.push_back(continuation(1));
        text.push_back(continuation(0));
    }
    else
    {
        text.push_back(
            static_cast<char>(four_bytes_marker | (character >> (continuation_shift * 3))));
        text.push_back(continuation(2));
        text.push_back(continuation(1));
        text.push_back(continuation(0));
    }
}

// Copies the state of the keyboard and the mouse from Raylib into the Input resource.
void read_input(ecs::World& world)
{
    Input& input = world.resource<Input>();
    input.begin_frame();

    for (std::size_t index = 0; index < key_count; ++index)
    {
        input.set_key_down(static_cast<Key>(index),
                           raylib::Keyboard::IsKeyDown(raylib_keys[index]));
    }
    for (std::size_t index = 0; index < mouse_button_count; ++index)
    {
        input.set_mouse_down(static_cast<MouseButton>(index),
                             raylib::Mouse::IsButtonDown(raylib_buttons[index]));
    }
    input.set_mouse_position(to_vec2(raylib::Mouse::GetPosition()));
    input.add_mouse_wheel(raylib::Mouse::GetWheelMove());

    // Raylib queues the typed characters and returns 0 when the queue is empty.
    std::string text;
    for (int character = raylib::Keyboard::GetCharPressed(); character > 0;
         character = raylib::Keyboard::GetCharPressed())
    {
        append_utf8(text, static_cast<std::uint32_t>(character));
    }
    input.add_text(text);
}

void update_music(ecs::World& world)
{
    // There is no Audio resource when audio is disabled or no device could be opened.
    if (Audio* audio = world.try_resource<Audio>(); audio != nullptr)
    {
        audio->update();
    }
}

void publish_exit_request(ecs::World& world)
{
    if (world.resource<Window>().should_close())
    {
        world.resource<core::EventBus>().publish(core::ExitRequested{});
    }
}

} // namespace

void PlatformPlugin::build(core::Application& app) const
{
    auto window = Window::open(settings.window);
    if (!window.has_value())
    {
        core::log_error("The window could not be opened; the application exits");
        app.request_exit();
        return;
    }

    ecs::World& world = app.world();
    // The world destroys resources in reverse order of insertion, so the window outlives
    // the graphics service, which needs its graphics context to free textures.
    world.insert_resource(std::move(*window));
    world.insert_resource(Graphics{});
    world.insert_resource(Input{});

    if (settings.is_audio_enabled)
    {
        if (auto audio = Audio::open(); audio.has_value())
        {
            world.insert_resource(std::move(*audio));
        }
        else
        {
            core::log_warning("No audio device could be opened; the game runs without sound");
        }
    }

    app.add_system(core::Stage::PreUpdate, "read_input", read_input);
    app.add_system(core::Stage::PreUpdate, "update_music", update_music);
    app.add_system(core::Stage::PreUpdate, "publish_exit_request", publish_exit_request);
    app.add_system(core::Stage::PreRender, "begin_frame",
                   [clear_color = settings.clear_color](ecs::World& current)
                   { current.resource<Graphics>().begin_frame(clear_color); });
    app.add_system(core::Stage::PostRender, "end_frame",
                   [](ecs::World& current) { current.resource<Graphics>().end_frame(); });
}

} // namespace ember::platform
