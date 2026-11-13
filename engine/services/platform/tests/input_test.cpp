#include <catch_amalgamated.hpp>

import ember.common;
import ember.platform;
import std;

using ember::Vec2;
using ember::platform::Input;
using ember::platform::Key;
using ember::platform::MouseButton;

TEST_CASE("a new input has nothing down")
{
    const Input input;

    CHECK_FALSE(input.is_key_down(Key::A));
    CHECK_FALSE(input.is_mouse_down(MouseButton::Left));
    CHECK(input.mouse_position() == Vec2{});
    CHECK(input.mouse_delta() == Vec2{});
    CHECK(input.mouse_wheel() == 0.0f);
    CHECK(input.text().empty());
}

TEST_CASE("a key is pressed in the frame it goes down and released in the frame it goes up")
{
    Input input;

    input.begin_frame();
    input.set_key_down(Key::Space, true);
    CHECK(input.is_key_down(Key::Space));
    CHECK(input.is_key_pressed(Key::Space));
    CHECK_FALSE(input.is_key_released(Key::Space));

    input.begin_frame();
    CHECK(input.is_key_down(Key::Space));
    CHECK_FALSE(input.is_key_pressed(Key::Space));

    input.begin_frame();
    input.set_key_down(Key::Space, false);
    CHECK_FALSE(input.is_key_down(Key::Space));
    CHECK(input.is_key_released(Key::Space));

    input.begin_frame();
    CHECK_FALSE(input.is_key_released(Key::Space));
}

TEST_CASE("keys do not affect each other")
{
    Input input;
    input.set_key_down(Key::A, true);
    input.set_key_down(Key::Slash, true);

    CHECK(input.is_key_down(Key::A));
    CHECK(input.is_key_down(Key::Slash));
    CHECK_FALSE(input.is_key_down(Key::B));
}

TEST_CASE("a mouse button is pressed and released like a key")
{
    Input input;

    input.begin_frame();
    input.set_mouse_down(MouseButton::Right, true);
    CHECK(input.is_mouse_down(MouseButton::Right));
    CHECK(input.is_mouse_pressed(MouseButton::Right));
    CHECK_FALSE(input.is_mouse_down(MouseButton::Left));

    input.begin_frame();
    CHECK_FALSE(input.is_mouse_pressed(MouseButton::Right));

    input.begin_frame();
    input.set_mouse_down(MouseButton::Right, false);
    CHECK(input.is_mouse_released(MouseButton::Right));
}

TEST_CASE("mouse delta is the movement since the previous frame")
{
    Input input;

    input.begin_frame();
    input.set_mouse_position({10.0f, 20.0f});
    CHECK(input.mouse_position() == Vec2{10.0f, 20.0f});
    CHECK(input.mouse_delta() == Vec2{10.0f, 20.0f});

    input.begin_frame();
    input.set_mouse_position({13.0f, 15.0f});
    CHECK(input.mouse_delta() == Vec2{3.0f, -5.0f});

    input.begin_frame();
    CHECK(input.mouse_position() == Vec2{13.0f, 15.0f});
    CHECK(input.mouse_delta() == Vec2{});
}

TEST_CASE("mouse wheel and text are collected during a frame and cleared by the next")
{
    Input input;

    input.begin_frame();
    input.add_mouse_wheel(1.0f);
    input.add_mouse_wheel(0.5f);
    input.add_text("ab");
    input.add_text("c");
    CHECK(input.mouse_wheel() == 1.5f);
    CHECK(input.text() == "abc");

    input.begin_frame();
    CHECK(input.mouse_wheel() == 0.0f);
    CHECK(input.text().empty());
}

TEST_CASE("an action is down while any of its keys or buttons is down")
{
    Input input;
    input.bind("jump", Key::Space);
    input.bind("jump", Key::W);
    input.bind("jump", MouseButton::Left);

    CHECK_FALSE(input.is_action_down("jump"));

    input.set_key_down(Key::W, true);
    CHECK(input.is_action_down("jump"));

    input.set_key_down(Key::W, false);
    input.set_mouse_down(MouseButton::Left, true);
    CHECK(input.is_action_down("jump"));
}

TEST_CASE("an action is pressed and released once, even when its keys overlap")
{
    Input input;
    input.bind("jump", Key::Space);
    input.bind("jump", Key::W);

    input.begin_frame();
    input.set_key_down(Key::Space, true);
    CHECK(input.is_action_pressed("jump"));

    // A second key of an action that is already down is not a new press.
    input.begin_frame();
    input.set_key_down(Key::W, true);
    CHECK_FALSE(input.is_action_pressed("jump"));

    // The action stays down while one of its keys is down.
    input.begin_frame();
    input.set_key_down(Key::Space, false);
    CHECK(input.is_action_down("jump"));
    CHECK_FALSE(input.is_action_released("jump"));

    input.begin_frame();
    input.set_key_down(Key::W, false);
    CHECK(input.is_action_released("jump"));
}

TEST_CASE("an action without bindings is never down")
{
    Input input;
    input.set_key_down(Key::Space, true);

    CHECK_FALSE(input.is_action_down("jump"));
    CHECK_FALSE(input.is_action_pressed("jump"));
    CHECK_FALSE(input.is_action_released("jump"));
}

TEST_CASE("a key can be bound to several actions")
{
    Input input;
    input.bind("jump", Key::Space);
    input.bind("confirm", Key::Space);
    input.set_key_down(Key::Space, true);

    CHECK(input.is_action_down("jump"));
    CHECK(input.is_action_down("confirm"));
}

TEST_CASE("unbinding removes all keys and buttons of an action")
{
    Input input;
    input.bind("fire", Key::F);
    input.bind("fire", MouseButton::Left);
    input.set_key_down(Key::F, true);
    input.set_mouse_down(MouseButton::Left, true);

    input.unbind("fire");
    CHECK_FALSE(input.is_action_down("fire"));

    CHECK_NOTHROW(input.unbind("unknown"));
}

TEST_CASE("an axis combines two opposite actions")
{
    Input input;
    input.bind("left", Key::Left);
    input.bind("right", Key::Right);

    CHECK(input.axis("left", "right") == 0.0f);

    input.set_key_down(Key::Right, true);
    CHECK(input.axis("left", "right") == 1.0f);

    input.set_key_down(Key::Left, true);
    CHECK(input.axis("left", "right") == 0.0f);

    input.set_key_down(Key::Right, false);
    CHECK(input.axis("left", "right") == -1.0f);
}
