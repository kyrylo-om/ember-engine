module ember.platform;

import ember.common;
import std;

namespace ember::platform
{

namespace
{

std::size_t key_index(Key key)
{
    return static_cast<std::size_t>(key);
}

std::size_t button_index(MouseButton button)
{
    return static_cast<std::size_t>(button);
}

// Adds a value to a list unless it is already there, so binding twice changes nothing.
template <typename T>
void add_unique(std::vector<T>& values, T value)
{
    if (!std::ranges::contains(values, value))
    {
        values.push_back(value);
    }
}

} // namespace

bool Input::is_key_down(Key key) const
{
    return m_keys[key_index(key)];
}

bool Input::is_key_pressed(Key key) const
{
    return m_keys[key_index(key)] && !m_previous_keys[key_index(key)];
}

bool Input::is_key_released(Key key) const
{
    return !m_keys[key_index(key)] && m_previous_keys[key_index(key)];
}

bool Input::is_mouse_down(MouseButton button) const
{
    return m_mouse[button_index(button)];
}

bool Input::is_mouse_pressed(MouseButton button) const
{
    return m_mouse[button_index(button)] && !m_previous_mouse[button_index(button)];
}

bool Input::is_mouse_released(MouseButton button) const
{
    return !m_mouse[button_index(button)] && m_previous_mouse[button_index(button)];
}

Vec2 Input::mouse_position() const
{
    return m_mouse_position;
}

Vec2 Input::mouse_delta() const
{
    return m_mouse_position - m_previous_mouse_position;
}

float Input::mouse_wheel() const
{
    return m_mouse_wheel;
}

std::string_view Input::text() const
{
    return m_text;
}

void Input::bind(std::string_view action, Key key)
{
    // try_emplace instead of operator[], which cannot look up by std::string_view.
    add_unique(m_bindings.try_emplace(std::string{action}).first->second.keys, key);
}

void Input::bind(std::string_view action, MouseButton button)
{
    add_unique(m_bindings.try_emplace(std::string{action}).first->second.buttons, button);
}

void Input::unbind(std::string_view action)
{
    const auto found = m_bindings.find(action);
    if (found != m_bindings.end())
    {
        m_bindings.erase(found);
    }
}

bool Input::is_action_down(std::string_view action) const
{
    const auto found = m_bindings.find(action);
    if (found == m_bindings.end())
    {
        return false;
    }
    const Binding& binding = found->second;
    return std::ranges::any_of(binding.keys, [this](Key key) { return is_key_down(key); }) ||
           std::ranges::any_of(binding.buttons,
                               [this](MouseButton button) { return is_mouse_down(button); });
}

bool Input::is_action_pressed(std::string_view action) const
{
    return is_action_down(action) && !was_action_down(action);
}

bool Input::is_action_released(std::string_view action) const
{
    return !is_action_down(action) && was_action_down(action);
}

float Input::axis(std::string_view negative, std::string_view positive) const
{
    const float negative_part = is_action_down(negative) ? 1.0f : 0.0f;
    const float positive_part = is_action_down(positive) ? 1.0f : 0.0f;
    return positive_part - negative_part;
}

void Input::begin_frame()
{
    m_previous_keys = m_keys;
    m_previous_mouse = m_mouse;
    m_previous_mouse_position = m_mouse_position;
    m_mouse_wheel = 0.0f;
    m_text.clear();
}

void Input::set_key_down(Key key, bool is_down)
{
    m_keys[key_index(key)] = is_down;
}

void Input::set_mouse_down(MouseButton button, bool is_down)
{
    m_mouse[button_index(button)] = is_down;
}

void Input::set_mouse_position(Vec2 position)
{
    m_mouse_position = position;
}

void Input::add_mouse_wheel(float amount)
{
    m_mouse_wheel += amount;
}

void Input::add_text(std::string_view text)
{
    m_text += text;
}

bool Input::was_action_down(std::string_view action) const
{
    const auto found = m_bindings.find(action);
    if (found == m_bindings.end())
    {
        return false;
    }
    const Binding& binding = found->second;
    return std::ranges::any_of(binding.keys,
                               [this](Key key) { return m_previous_keys[key_index(key)]; }) ||
           std::ranges::any_of(binding.buttons, [this](MouseButton button)
                               { return m_previous_mouse[button_index(button)]; });
}

} // namespace ember::platform
