// Yozakura ("cherry blossoms at night"): a small game that shows the engine at work. A girl
// runs and jumps under falling petals and catches spirit flames that drift down from the sky.
// Every spirit that reaches the ground costs a life.
//
// The pictures and sounds are made in code (see art.cppm and sound.cppm), so the game needs
// no asset files.
import yozakura.art;
import yozakura.sound;
import ember.common;
import ember.core;
import ember.ecs;
import ember.particles;
import ember.platform;
import std;

using ember::Color;
using ember::Handle;
using ember::Rect;
using ember::Vec2;
using ember::core::Application;
using ember::core::EventBus;
using ember::core::Seconds;
using ember::core::Stage;
using ember::core::Time;
using ember::ecs::Commands;
using ember::ecs::Emitter;
using ember::ecs::Entity;
using ember::ecs::Transform;
using ember::ecs::Velocity;
using ember::ecs::World;
using ember::platform::Audio;
using ember::platform::Graphics;
using ember::platform::Input;
using ember::platform::Key;
using ember::platform::Window;
using yozakura::Frame;
namespace platform = ember::platform;

namespace
{

constexpr float pi = std::numbers::pi_v<float>;
// Direction of emitters that emit downwards; y points down.
constexpr float down = pi / 2.0f;
constexpr float opaque = 255.0f;

// ---------------------------------------------------------------------------------------
// Layout of the scene, in world units. The scene is as large as the window when it opens,
// so one unit is one pixel until the window is resized.
// ---------------------------------------------------------------------------------------

constexpr int window_width = 960;
constexpr int window_height = 540;
constexpr Vec2 scene_size{static_cast<float>(window_width), static_cast<float>(window_height)};
constexpr float ground_y = 470.0f;

constexpr Color clear_color{10, 6, 30, 255};
constexpr Color text_color{255, 236, 246, 255};
constexpr Color dim_text_color{206, 176, 208, 255};
constexpr Color text_shadow{20, 6, 36, 200};
constexpr Color bar_color{0, 0, 0, 255};
constexpr Color spirit_blue{96, 224, 255, 255};
constexpr Color spirit_gold{255, 208, 96, 255};

// Returns a color with another alpha, from 0 (transparent) to 1 (opaque).
constexpr Color faded(Color color, float alpha)
{
    color.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * opaque);
    return color;
}

// The scene always has the same size in world units. When the window has another size, the
// scene is scaled to fit into it and centered.
platform::View scene_view(Vec2 window_size, Vec2 shake = {})
{
    return {.target = scene_size / 2.0f + shake,
            .offset = window_size / 2.0f,
            .zoom = std::min(window_size.x / scene_size.x, window_size.y / scene_size.y)};
}

// ---------------------------------------------------------------------------------------
// Components, resources and events of the game.
// ---------------------------------------------------------------------------------------

/// The girl controlled by the player.
struct Player
{
    bool is_facing_left = false;
    bool is_on_ground = true;
    bool is_running = false;
    /// Time until she leaves the next shadow behind her.
    Seconds until_afterimage{};
};

/// A shadow the girl leaves behind while she moves: her shape at that moment, fading out.
struct Afterimage
{
    Frame frame = Frame::Standing;
    bool is_facing_left = false;
};

/// A spirit flame that drifts down and can be caught.
struct Spirit
{
    /// Horizontal position around which the spirit sways.
    float center_x = 0.0f;
    /// Offset of the sway, so the spirits do not move in step.
    float phase = 0.0f;
    float fall_speed = 0.0f;
    /// A golden spirit is rarer and faster and is worth more points.
    bool is_golden = false;
};

/// Points that float up from the place where a spirit was caught.
struct FloatingScore
{
    int points = 0;
};

/// Destroys its entity when the time runs out.
struct Expiring
{
    Seconds remaining{};
    /// The time the entity started with; drawing uses the ratio to fade it out.
    Seconds lifetime{};
};

/// Published when the player touches a spirit.
struct SpiritCaught
{
    Vec2 position{};
    bool is_golden = false;
};

/// Published when a spirit reaches the ground.
struct SpiritLost
{
    Vec2 position{};
};

constexpr int max_lives = 3;

/// State of the whole game.
struct Game
{
    bool is_over = false;
    int score = 0;
    int best_score = 0;
    int lives = max_lives;
    Seconds until_next_spirit{1.0f};
    /// Time the screen still shakes after a spirit was lost.
    Seconds shake_remaining{};
    /// Offset of the view in this frame, from the shake.
    Vec2 shake_offset{};
    bool is_music_muted = false;
    std::mt19937 random{2027};
};

struct GameAssets
{
    Handle<ember::Texture> girl{};
    Handle<ember::Texture> girl_silhouette{};
    Handle<ember::Texture> glow{};
    Handle<ember::Texture> sky{};
    Handle<ember::Texture> moon{};
    Handle<ember::Texture> mountain{};
    Handle<ember::Texture> hills{};
    Handle<ember::Texture> cloud{};
    Handle<ember::Texture> shadow{};
    Handle<ember::Texture> heart{};
    Handle<ember::Texture> vignette{};
    Handle<ember::Sound> chime{};
    Handle<ember::Sound> golden_chime{};
    Handle<ember::Sound> miss{};
    Handle<ember::Music> music{};
};

// Screen pixels per sprite pixel.
constexpr float girl_scale = 4.0f;
constexpr Vec2 girl_size = yozakura::frame_size * girl_scale;

constexpr float run_speed = 320.0f;
constexpr float jump_speed = 680.0f;
constexpr float gravity = 1700.0f;
// Part of the upward speed that is kept when the jump key is released early, which makes a
// short press a low jump.
constexpr float short_jump_factor = 0.45f;
constexpr float player_half_width = girl_size.x / 2.0f;
constexpr float player_ground_y = ground_y - girl_size.y / 2.0f;

constexpr Seconds afterimage_interval{0.045f};
constexpr Seconds afterimage_lifetime{0.32f};

constexpr float spirit_radius = 11.0f;
constexpr float catch_distance = 54.0f;
constexpr float spawn_margin = 60.0f;
constexpr float spawn_y = -30.0f;
constexpr int golden_points = 5;
constexpr float music_volume = 0.5f;

// ---------------------------------------------------------------------------------------
// Emitters.
// ---------------------------------------------------------------------------------------

// Petals that fall slowly from above the window and drift with the wind.
Emitter petal_emitter()
{
    Emitter emitter{};
    emitter.emission_rate = 5.0f;
    emitter.lifetime = Seconds{10.0f};
    emitter.min_speed = 35.0f;
    emitter.max_speed = 75.0f;
    emitter.direction = down;
    emitter.spread = 0.7f;
    emitter.acceleration = {10.0f, 3.0f};
    emitter.start_color = {255, 196, 216, 240};
    emitter.end_color = {255, 140, 180, 0};
    emitter.start_size = 7.0f;
    emitter.end_size = 4.0f;
    return emitter;
}

// Petals that leave the crown of the tree in every direction and then fall.
Emitter tree_petal_emitter()
{
    Emitter emitter = petal_emitter();
    emitter.emission_rate = 9.0f;
    emitter.lifetime = Seconds{5.0f};
    emitter.min_speed = 20.0f;
    emitter.max_speed = 90.0f;
    emitter.spread = pi;
    emitter.acceleration = {-6.0f, 40.0f};
    return emitter;
}

// The flame that rises from a spirit.
Emitter flame_emitter(Color color)
{
    Emitter emitter{};
    emitter.emission_rate = 34.0f;
    emitter.lifetime = Seconds{0.55f};
    emitter.min_speed = 20.0f;
    emitter.max_speed = 70.0f;
    emitter.direction = -down;
    emitter.spread = 0.45f;
    emitter.start_color = color;
    emitter.end_color = faded(color, 0.0f);
    emitter.start_size = 8.0f;
    emitter.end_size = 2.0f;
    return emitter;
}

// One burst in all directions where a spirit was caught.
Emitter burst_emitter(Color color, std::uint32_t count)
{
    Emitter emitter{};
    emitter.is_emitting = false;
    emitter.burst_count = count;
    emitter.lifetime = Seconds{0.9f};
    emitter.min_speed = 80.0f;
    emitter.max_speed = 340.0f;
    emitter.spread = pi;
    emitter.acceleration = {0.0f, 420.0f};
    emitter.start_color = Color::white;
    emitter.end_color = faded(color, 0.0f);
    emitter.start_size = 7.0f;
    emitter.end_size = 1.0f;
    return emitter;
}

// Dark smoke that rises where a spirit reached the ground.
Emitter smoke_emitter()
{
    Emitter emitter{};
    emitter.is_emitting = false;
    emitter.burst_count = 26;
    emitter.lifetime = Seconds{0.9f};
    emitter.min_speed = 20.0f;
    emitter.max_speed = 110.0f;
    emitter.direction = -down;
    emitter.spread = 1.1f;
    emitter.acceleration = {0.0f, -60.0f};
    emitter.start_color = {120, 90, 170, 220};
    emitter.end_color = {40, 20, 70, 0};
    emitter.start_size = 6.0f;
    emitter.end_size = 12.0f;
    return emitter;
}

// ---------------------------------------------------------------------------------------
// Starting the game.
// ---------------------------------------------------------------------------------------

GameAssets load_assets(World& world, std::span<const std::byte> music)
{
    Graphics& graphics = world.resource<Graphics>();
    const auto texture = [&graphics](const platform::Image& image)
    {
        return graphics.create_texture(image).value_or({});
    };

    GameAssets assets{};
    const platform::Image girl = yozakura::girl_sheet();
    assets.girl = texture(girl);
    assets.girl_silhouette = texture(yozakura::silhouette_of(girl));
    assets.glow = texture(yozakura::glow_image());
    assets.sky = texture(yozakura::sky_image());
    assets.moon = texture(yozakura::moon_image());
    assets.mountain = texture(yozakura::mountain_image());
    assets.hills = texture(yozakura::hills_image());
    assets.cloud = texture(yozakura::cloud_image());
    assets.shadow = texture(yozakura::shadow_image());
    assets.heart = texture(yozakura::heart_image());
    assets.vignette = texture(yozakura::vignette_image());

    // Without an audio device the handles stay empty, and playing them does nothing.
    if (Audio* audio = world.try_resource<Audio>(); audio != nullptr)
    {
        assets.chime = audio->load_sound(yozakura::chime_wav(), ".wav").value_or({});
        assets.golden_chime = audio->load_sound(yozakura::golden_chime_wav(), ".wav").value_or({});
        assets.miss = audio->load_sound(yozakura::miss_wav(), ".wav").value_or({});
        assets.music = audio->load_music(music, ".wav").value_or({});
        audio->set_music_volume(assets.music, music_volume);
        audio->play_music(assets.music);
    }
    return assets;
}

void bind_keys(Input& input)
{
    input.bind("left", Key::A);
    input.bind("left", Key::Left);
    input.bind("right", Key::D);
    input.bind("right", Key::Right);
    input.bind("jump", Key::Space);
    input.bind("jump", Key::W);
    input.bind("jump", Key::Up);
    input.bind("restart", Key::Enter);
    input.bind("mute", Key::M);
    input.bind("quit", Key::Escape);
}

void create_scene(World& world)
{
    // Looks at the middle of the scene, like the view the scene is drawn with, so particles
    // are drawn at the same coordinates as everything else.
    const Entity camera = world.create();
    world.add<ember::ecs::Camera>(camera, {.position = scene_size / 2.0f});

    const Entity player = world.create();
    world.add<Transform>(player, {.position = {scene_size.x / 2.0f, player_ground_y}});
    world.add<Velocity>(player);
    world.add<Player>(player);

    constexpr Vec2 tree_crown{830.0f, 230.0f};
    const Entity tree_petals = world.create();
    world.add<Transform>(tree_petals, {.position = tree_crown});
    world.add<Emitter>(tree_petals, tree_petal_emitter());

    constexpr int petal_emitter_count = 7;
    constexpr float petal_emitter_y = -20.0f;
    for (int index = 0; index < petal_emitter_count; ++index)
    {
        const float x =
            scene_size.x * static_cast<float>(index) / static_cast<float>(petal_emitter_count - 1);
        const Entity petals = world.create();
        world.add<Transform>(petals, {.position = {x, petal_emitter_y}});
        world.add<Emitter>(petals, petal_emitter());
    }
}

// ---------------------------------------------------------------------------------------
// Systems of the game logic.
// ---------------------------------------------------------------------------------------

// Chooses the frame of the sprite sheet for the state of the girl.
Frame animation_frame(const Player& player, float elapsed)
{
    constexpr float steps_per_second = 10.0f;
    constexpr std::array run_cycle{Frame::Striding, Frame::Standing, Frame::Passing,
                                   Frame::Standing};
    constexpr float blink_interval = 3.2f;
    constexpr float blink_duration = 0.14f;

    if (!player.is_on_ground)
    {
        return Frame::Jumping;
    }
    if (player.is_running)
    {
        const auto step = static_cast<std::size_t>(elapsed * steps_per_second);
        return run_cycle[step % run_cycle.size()];
    }
    return std::fmod(elapsed, blink_interval) < blink_duration ? Frame::Blinking : Frame::Standing;
}

void start_new_game(World& world)
{
    Game& game = world.resource<Game>();
    game.is_over = false;
    game.score = 0;
    game.lives = max_lives;
    game.until_next_spirit = Seconds{1.0f};
    for (auto [entity, spirit] : world.view<const Spirit>())
    {
        world.commands().destroy(entity);
    }
}

void handle_keys(World& world)
{
    const Input& input = world.resource<Input>();
    Game& game = world.resource<Game>();

    if (input.is_action_pressed("quit"))
    {
        world.resource<EventBus>().publish(ember::core::ExitRequested{});
    }
    if (input.is_action_pressed("mute"))
    {
        game.is_music_muted = !game.is_music_muted;
        if (Audio* audio = world.try_resource<Audio>(); audio != nullptr)
        {
            audio->set_music_volume(world.resource<GameAssets>().music,
                                    game.is_music_muted ? 0.0f : music_volume);
        }
    }
    if (game.is_over && input.is_action_pressed("restart"))
    {
        start_new_game(world);
    }

    for (auto [entity, player, velocity] : world.view<Player, Velocity>())
    {
        const float direction = input.axis("left", "right");
        velocity.value.x = direction * run_speed;
        player.is_running = direction != 0.0f;
        if (player.is_running)
        {
            player.is_facing_left = direction < 0.0f;
        }
        if (player.is_on_ground && input.is_action_pressed("jump"))
        {
            velocity.value.y = -jump_speed;
            player.is_on_ground = false;
        }
        if (input.is_action_released("jump") && velocity.value.y < 0.0f)
        {
            velocity.value.y *= short_jump_factor;
        }
    }
}

// Runs with a fixed time step, so a jump is equally high at any frame rate.
void move_player(World& world)
{
    const float dt = world.resource<Time>().fixed_delta.count();
    for (auto [entity, player, transform, velocity] : world.view<Player, Transform, Velocity>())
    {
        velocity.value.y += gravity * dt;
        transform.position += velocity.value * dt;

        if (transform.position.y >= player_ground_y)
        {
            transform.position.y = player_ground_y;
            velocity.value.y = 0.0f;
            player.is_on_ground = true;
        }
        transform.position.x =
            std::clamp(transform.position.x, player_half_width, scene_size.x - player_half_width);
    }
}

// While the girl runs or jumps, she leaves shadows of herself behind, a few every second.
void leave_afterimages(World& world)
{
    const Time& time = world.resource<Time>();
    const auto elapsed = static_cast<float>(time.elapsed.count());
    for (auto [entity, player, transform] : world.view<Player, const Transform>())
    {
        const bool is_moving = player.is_running || !player.is_on_ground;
        player.until_afterimage -= time.delta;
        if (!is_moving || player.until_afterimage > Seconds::zero())
        {
            continue;
        }
        player.until_afterimage = afterimage_interval;

        // The world is changed through commands, because this system is iterating over it;
        // the scheduler applies them after the stage.
        Commands& commands = world.commands();
        const Entity afterimage = commands.create();
        commands.add<Transform>(afterimage, transform);
        commands.add<Afterimage>(afterimage, {.frame = animation_frame(player, elapsed),
                                              .is_facing_left = player.is_facing_left});
        commands.add<Expiring>(afterimage,
                               {.remaining = afterimage_lifetime, .lifetime = afterimage_lifetime});
    }
}

void move_spirits(World& world)
{
    constexpr float sway_width = 40.0f;
    constexpr float sway_speed = 2.0f;

    const Time& time = world.resource<Time>();
    const auto elapsed = static_cast<float>(time.elapsed.count());
    for (auto [entity, spirit, transform] : world.view<const Spirit, Transform>())
    {
        transform.position.y += spirit.fall_speed * time.delta.count();
        transform.position.x =
            spirit.center_x + sway_width * std::sin(elapsed * sway_speed + spirit.phase);
        if (transform.position.y > ground_y)
        {
            world.resource<EventBus>().publish(SpiritLost{transform.position});
            world.commands().destroy(entity);
        }
    }
}

// The more spirits are caught, the more often and the faster new ones fall.
void spawn_spirits(World& world)
{
    constexpr Seconds slowest_interval{1.5f};
    constexpr Seconds fastest_interval{0.45f};
    constexpr Seconds interval_step{0.03f};
    constexpr float slowest_fall = 90.0f;
    constexpr float fastest_fall = 230.0f;
    constexpr float fall_step = 3.0f;
    constexpr float golden_fall_factor = 1.5f;
    constexpr float golden_chance = 0.12f;
    // How much the time to the next spirit varies around its middle value.
    constexpr float interval_variation = 0.35f;

    Game& game = world.resource<Game>();
    game.until_next_spirit -= world.resource<Time>().delta;
    if (game.is_over || game.until_next_spirit > Seconds::zero())
    {
        return;
    }

    const auto score = static_cast<float>(game.score);
    const Seconds interval = std::max(fastest_interval, slowest_interval - interval_step * score);
    const float fall_speed = std::min(fastest_fall, slowest_fall + fall_step * score);

    std::uniform_real_distribution<float> variation{1.0f - interval_variation,
                                                    1.0f + interval_variation};
    std::uniform_real_distribution<float> position{spawn_margin, scene_size.x - spawn_margin};
    std::uniform_real_distribution<float> phase{0.0f, 2.0f * pi};
    std::bernoulli_distribution golden{golden_chance};

    game.until_next_spirit = interval * variation(game.random);
    const float x = position(game.random);
    const bool is_golden = golden(game.random);

    Commands& commands = world.commands();
    const Entity spirit = commands.create();
    commands.add<Transform>(spirit, {.position = {x, spawn_y}});
    commands.add<Spirit>(spirit,
                         {.center_x = x,
                          .phase = phase(game.random),
                          .fall_speed = is_golden ? fall_speed * golden_fall_factor : fall_speed,
                          .is_golden = is_golden});
    commands.add<Emitter>(spirit, flame_emitter(is_golden ? spirit_gold : spirit_blue));
}

void catch_spirits(World& world)
{
    for (auto [player_entity, player, player_transform] :
         world.view<const Player, const Transform>())
    {
        for (auto [entity, spirit, transform] : world.view<const Spirit, const Transform>())
        {
            if (ember::distance(player_transform.position, transform.position) < catch_distance)
            {
                world.resource<EventBus>().publish(
                    SpiritCaught{transform.position, spirit.is_golden});
                world.commands().destroy(entity);
            }
        }
    }
}

// Creates an entity that emits one burst of particles and then removes itself.
void spawn_burst(Commands& commands, Vec2 position, const Emitter& emitter)
{
    // The particles disappear with their emitter, so it lives a little longer than they.
    constexpr Seconds extra_time{0.2f};
    const Seconds lifetime = emitter.lifetime + extra_time;

    const Entity burst = commands.create();
    commands.add<Transform>(burst, {.position = position});
    commands.add<Emitter>(burst, emitter);
    commands.add<Expiring>(burst, {.remaining = lifetime, .lifetime = lifetime});
}

void play(World& world, Handle<ember::Sound> sound, float volume)
{
    if (Audio* audio = world.try_resource<Audio>(); audio != nullptr)
    {
        audio->play_sound(sound, volume);
    }
}

// Reacts to the spirits caught and lost in the previous frame. The systems that detect
// these moments do not need to know about score, lives, particles or sound; they only
// publish events.
void react_to_spirits(World& world)
{
    constexpr std::uint32_t burst_size = 60;
    constexpr std::uint32_t golden_burst_size = 140;
    constexpr Seconds score_lifetime{0.9f};
    constexpr Seconds shake_duration{0.3f};
    constexpr float chime_volume = 0.7f;
    constexpr float miss_volume = 0.8f;

    const EventBus& events = world.resource<EventBus>();
    const GameAssets& assets = world.resource<GameAssets>();
    Game& game = world.resource<Game>();
    Commands& commands = world.commands();

    for (const SpiritCaught& caught : events.read<SpiritCaught>())
    {
        if (game.is_over)
        {
            continue;
        }
        const int points = caught.is_golden ? golden_points : 1;
        game.score += points;
        game.best_score = std::max(game.best_score, game.score);

        spawn_burst(commands, caught.position,
                    caught.is_golden ? burst_emitter(spirit_gold, golden_burst_size)
                                     : burst_emitter(spirit_blue, burst_size));
        const Entity score = commands.create();
        commands.add<Transform>(score, {.position = caught.position});
        commands.add<FloatingScore>(score, {.points = points});
        commands.add<Expiring>(score, {.remaining = score_lifetime, .lifetime = score_lifetime});
        play(world, caught.is_golden ? assets.golden_chime : assets.chime, chime_volume);
    }

    for (const SpiritLost& lost : events.read<SpiritLost>())
    {
        spawn_burst(commands, lost.position, smoke_emitter());
        if (game.is_over)
        {
            continue;
        }
        --game.lives;
        game.shake_remaining = shake_duration;
        play(world, assets.miss, miss_volume);
        if (game.lives <= 0)
        {
            game.is_over = true;
        }
    }
}

void float_scores(World& world)
{
    constexpr float rise_speed = 60.0f;
    const float dt = world.resource<Time>().delta.count();
    for (auto [entity, score, transform] : world.view<const FloatingScore, Transform>())
    {
        transform.position.y -= rise_speed * dt;
    }
}

void remove_expired(World& world)
{
    const Seconds delta = world.resource<Time>().delta;
    for (auto [entity, expiring] : world.view<Expiring>())
    {
        expiring.remaining -= delta;
        if (expiring.remaining <= Seconds::zero())
        {
            world.commands().destroy(entity);
        }
    }
}

// Keeps the camera in step with the size of the window and with the shake of the screen, so
// the particles, which are drawn through the camera, move like the rest of the scene.
void update_camera(World& world)
{
    constexpr float shake_size = 7.0f;
    constexpr Seconds shake_duration{0.3f};

    Game& game = world.resource<Game>();
    game.shake_remaining =
        std::max(Seconds::zero(), game.shake_remaining - world.resource<Time>().delta);
    // The shake is strongest at its start and dies away.
    const float strength = shake_size * (game.shake_remaining / shake_duration);
    std::uniform_real_distribution<float> offset{-1.0f, 1.0f};
    game.shake_offset = Vec2{offset(game.random), offset(game.random)} * strength;

    const platform::View view = scene_view(world.resource<Window>().size(), game.shake_offset);
    for (auto [entity, camera] : world.view<ember::ecs::Camera>())
    {
        camera.position = view.target;
        camera.zoom = view.zoom;
    }
}

// ---------------------------------------------------------------------------------------
// Drawing.
// ---------------------------------------------------------------------------------------

// Draws a whole texture with its center at a point, stretched to a size.
void draw_image(Graphics& graphics, Handle<ember::Texture> texture, Vec2 center, Vec2 size,
                Color tint = Color::white)
{
    const Vec2 texture_size = graphics.texture_size(texture).value_or({1.0f, 1.0f});
    graphics.draw_texture(texture, {.position = center,
                                    .scale = {size.x / texture_size.x, size.y / texture_size.y},
                                    .tint = tint});
}

// A round glow, drawn under lights.
void draw_glow(Graphics& graphics, const GameAssets& assets, Vec2 center, float diameter,
               Color color)
{
    draw_image(graphics, assets.glow, center, {diameter, diameter}, color);
}

void draw_sky(Graphics& graphics, const GameAssets& assets, float elapsed)
{
    draw_image(graphics, assets.sky, {scene_size.x / 2.0f, ground_y / 2.0f},
               {scene_size.x, ground_y});

    // The same stars every frame: the generator starts from the same seed. Each star
    // twinkles at its own speed.
    constexpr int star_count = 90;
    constexpr float lowest_star = 0.62f;
    constexpr float dimmest = 0.25f;
    constexpr Color star_color{255, 240, 250, 255};
    std::minstd_rand random{7};
    std::uniform_real_distribution<float> x{0.0f, scene_size.x};
    std::uniform_real_distribution<float> y{0.0f, ground_y * lowest_star};
    std::uniform_real_distribution<float> size{1.0f, 3.0f};
    std::uniform_real_distribution<float> speed{0.6f, 2.4f};
    for (int star = 0; star < star_count; ++star)
    {
        const Vec2 position{x(random), y(random)};
        const float star_size = size(random);
        const float twinkle = (std::sin(elapsed * speed(random) + position.x) + 1.0f) / 2.0f;
        graphics.draw_rect({.position = position, .size = {star_size, star_size}},
                           faded(star_color, std::lerp(dimmest, 1.0f, twinkle)));
    }
}

void draw_moon(Graphics& graphics, const GameAssets& assets, float elapsed)
{
    constexpr Vec2 center{600.0f, 122.0f};
    constexpr float diameter = 150.0f;
    constexpr float halo_diameter = 520.0f;
    constexpr float inner_halo_diameter = 280.0f;
    constexpr Color halo_color{255, 130, 190, 70};
    constexpr Color inner_halo_color{255, 210, 230, 110};
    // The halo slowly grows and shrinks, as if the moon were breathing.
    constexpr float pulse_speed = 0.8f;
    constexpr float pulse_size = 18.0f;

    const float pulse = pulse_size * std::sin(elapsed * pulse_speed);
    draw_glow(graphics, assets, center, halo_diameter + pulse, halo_color);
    draw_glow(graphics, assets, center, inner_halo_diameter + pulse, inner_halo_color);
    draw_image(graphics, assets.moon, center, {diameter, diameter});
}

void draw_clouds(Graphics& graphics, const GameAssets& assets, float elapsed)
{
    struct Cloud
    {
        float start_x = 0.0f;
        float y = 0.0f;
        Vec2 size{};
        float speed = 0.0f;
        std::uint8_t alpha = 0;
    };
    constexpr std::array<Cloud, 4> clouds{
        Cloud{100.0f, 90.0f, {300.0f, 80.0f}, 9.0f, 70},
        Cloud{560.0f, 150.0f, {380.0f, 90.0f}, 14.0f, 90},
        Cloud{820.0f, 60.0f, {240.0f, 60.0f}, 6.0f, 60},
        Cloud{340.0f, 210.0f, {420.0f, 70.0f}, 18.0f, 50},
    };
    constexpr Color cloud_color{196, 150, 214, 255};

    for (const Cloud& cloud : clouds)
    {
        // A cloud that leaves the scene on the right comes back in on the left.
        const float path = scene_size.x + cloud.size.x;
        const float x =
            std::fmod(cloud.start_x + elapsed * cloud.speed, path) - cloud.size.x / 2.0f;
        Color tint = cloud_color;
        tint.a = cloud.alpha;
        draw_image(graphics, assets.cloud, {x, cloud.y}, cloud.size, tint);
    }
}

void draw_landscape(Graphics& graphics, const GameAssets& assets)
{
    constexpr Vec2 mountain_size{640.0f, 240.0f};
    constexpr float mountain_x = 330.0f;
    constexpr float hills_height = 128.0f;
    // Mist at the foot of the mountain, which makes it look far away.
    constexpr Vec2 mist_size{1100.0f, 190.0f};
    constexpr Color mist_color{232, 112, 150, 90};

    draw_image(graphics, assets.mountain, {mountain_x, ground_y - mountain_size.y / 2.0f},
               mountain_size);
    draw_image(graphics, assets.glow, {scene_size.x / 2.0f, ground_y}, mist_size, mist_color);
    draw_image(graphics, assets.hills, {scene_size.x / 2.0f, ground_y - hills_height / 2.0f},
               {scene_size.x, hills_height});
}

// A torii, the gate of a shrine: two pillars, two beams and a dark roof.
void draw_gate(Graphics& graphics)
{
    constexpr float center_x = 210.0f;
    constexpr float pillar_width = 22.0f;
    constexpr float pillar_height = 230.0f;
    constexpr float pillar_distance = 150.0f;
    constexpr Vec2 roof{300.0f, 10.0f};
    constexpr Vec2 top_beam{270.0f, 22.0f};
    constexpr Vec2 lower_beam{200.0f, 16.0f};
    constexpr float lower_beam_drop = 56.0f;
    constexpr Vec2 tablet{26.0f, 34.0f};
    constexpr Vec2 foot{34.0f, 14.0f};
    constexpr float shadow_width = 7.0f;
    constexpr Color red{176, 30, 52, 255};
    constexpr Color red_shadow{104, 16, 40, 255};
    constexpr Color dark{30, 12, 34, 255};

    const float top = ground_y - pillar_height;
    const auto centered_rect = [](float x, float y, Vec2 size)
    {
        return Rect{.position = {x - size.x / 2.0f, y}, .size = size};
    };

    for (const float side : {-1.0f, 1.0f})
    {
        const float x = center_x + side * pillar_distance / 2.0f;
        graphics.draw_rect(centered_rect(x, top, {pillar_width, pillar_height}), red);
        graphics.draw_rect({.position = {x + pillar_width / 2.0f - shadow_width, top},
                            .size = {shadow_width, pillar_height}},
                           red_shadow);
        graphics.draw_rect(centered_rect(x, ground_y - foot.y, foot), dark);
    }
    graphics.draw_rect(centered_rect(center_x, top - top_beam.y, top_beam), red);
    graphics.draw_rect(centered_rect(center_x, top - top_beam.y - roof.y, roof), dark);
    graphics.draw_rect(centered_rect(center_x, top + lower_beam_drop, lower_beam), red);
    graphics.draw_rect(
        centered_rect(center_x, top + lower_beam_drop + lower_beam.y - 2.0f, {lower_beam.x, 4.0f}),
        red_shadow);
    graphics.draw_rect(
        centered_rect(center_x, top + lower_beam_drop / 2.0f - tablet.y / 2.0f, tablet), dark);
}

// Paper lanterns on posts, with a warm light that flickers.
void draw_lanterns(Graphics& graphics, const GameAssets& assets, float elapsed)
{
    constexpr std::array<float, 3> positions{420.0f, 540.0f, 660.0f};
    constexpr float lantern_y = 318.0f;
    constexpr Vec2 body{24.0f, 32.0f};
    constexpr Vec2 light{10.0f, 24.0f};
    constexpr Vec2 cap{16.0f, 5.0f};
    constexpr float post_width = 4.0f;
    constexpr float glow_diameter = 170.0f;
    constexpr float flicker_speed = 5.0f;
    constexpr float flicker_size = 14.0f;
    constexpr Color paper{255, 112, 64, 255};
    constexpr Color flame{255, 214, 150, 255};
    constexpr Color wood{34, 18, 40, 255};
    constexpr Color glow_color{255, 130, 70, 120};

    for (const float x : positions)
    {
        const Vec2 center{x, lantern_y};
        // Each lantern flickers at its own moment.
        const float flicker = flicker_size * std::sin(elapsed * flicker_speed + x);
        graphics.draw_rect({.position = {x - post_width / 2.0f, lantern_y},
                            .size = {post_width, ground_y - lantern_y}},
                           wood);
        draw_glow(graphics, assets, center, glow_diameter + flicker, glow_color);
        graphics.draw_rect({.position = center - body / 2.0f, .size = body}, paper);
        graphics.draw_rect({.position = center - light / 2.0f, .size = light}, flame);
        graphics.draw_rect(
            {.position = {x - cap.x / 2.0f, lantern_y - body.y / 2.0f - cap.y}, .size = cap}, wood);
        graphics.draw_rect({.position = {x - cap.x / 2.0f, lantern_y + body.y / 2.0f}, .size = cap},
                           wood);
    }
}

// A cherry tree in bloom: a trunk with branches and a crown of blossom made of many circles.
void draw_tree(Graphics& graphics)
{
    constexpr Vec2 root{835.0f, ground_y};
    constexpr Vec2 fork{828.0f, 330.0f};
    constexpr std::array<Vec2, 4> branch_ends{Vec2{740.0f, 250.0f}, Vec2{800.0f, 200.0f},
                                              Vec2{870.0f, 210.0f}, Vec2{925.0f, 270.0f}};
    constexpr float trunk_width = 26.0f;
    constexpr float branch_width = 10.0f;
    constexpr Color bark{58, 34, 48, 255};
    constexpr Color bark_shadow{38, 22, 36, 255};
    constexpr float shadow_offset = 7.0f;

    graphics.draw_line(root, fork, trunk_width, bark);
    graphics.draw_line(root + Vec2{shadow_offset, 0.0f}, fork + Vec2{shadow_offset, 0.0f},
                       trunk_width / 2.0f, bark_shadow);
    for (const Vec2 end : branch_ends)
    {
        graphics.draw_line(fork, end, branch_width, bark);
    }

    // The blossom is drawn in three layers from dark to light, which gives the crown depth.
    // The same generator seed gives the same crown every frame.
    struct BlossomLayer
    {
        Color color{};
        int count = 0;
        float min_radius = 0.0f;
        float max_radius = 0.0f;
    };
    constexpr std::array<BlossomLayer, 3> layers{
        BlossomLayer{{176, 70, 130, 255}, 26, 30.0f, 46.0f},
        BlossomLayer{{238, 130, 176, 255}, 30, 20.0f, 34.0f},
        BlossomLayer{{255, 198, 220, 255}, 26, 8.0f, 18.0f},
    };
    constexpr Vec2 crown_center{830.0f, 215.0f};
    constexpr Vec2 crown_half_size{125.0f, 80.0f};

    std::minstd_rand random{11};
    std::uniform_real_distribution<float> angle{0.0f, 2.0f * pi};
    std::uniform_real_distribution<float> unit{0.0f, 1.0f};
    for (const BlossomLayer& layer : layers)
    {
        std::uniform_real_distribution<float> radius{layer.min_radius, layer.max_radius};
        for (int blossom = 0; blossom < layer.count; ++blossom)
        {
            // The square root spreads the points evenly over the ellipse instead of
            // crowding them in the middle.
            const float distance = std::sqrt(unit(random));
            const float direction = angle(random);
            const Vec2 offset{std::cos(direction) * crown_half_size.x * distance,
                              std::sin(direction) * crown_half_size.y * distance};
            graphics.draw_circle(crown_center + offset, radius(random), layer.color);
        }
    }
}

void draw_ground(Graphics& graphics)
{
    constexpr Color ground_color{16, 9, 28, 255};
    constexpr Color edge_color{92, 40, 96, 255};
    constexpr float edge_height = 3.0f;
    constexpr Color fallen_petal{255, 170, 200, 255};
    constexpr int fallen_petal_count = 70;

    graphics.draw_rect(
        {.position = {0.0f, ground_y}, .size = {scene_size.x, scene_size.y - ground_y}},
        ground_color);
    graphics.draw_rect({.position = {0.0f, ground_y}, .size = {scene_size.x, edge_height}},
                       edge_color);

    // Petals that have already fallen lie on the ground; the ones further down are fainter.
    std::minstd_rand random{23};
    std::uniform_real_distribution<float> x{0.0f, scene_size.x};
    std::uniform_real_distribution<float> depth{0.0f, 1.0f};
    std::uniform_real_distribution<float> size{2.0f, 5.0f};
    for (int petal = 0; petal < fallen_petal_count; ++petal)
    {
        const float petal_depth = depth(random);
        const float petal_size = size(random);
        graphics.draw_rect({.position = {x(random), ground_y + edge_height +
                                                        petal_depth * (scene_size.y - ground_y)},
                            .size = {petal_size, petal_size / 2.0f}},
                           faded(fallen_petal, 1.0f - petal_depth));
    }
}

void draw_spirits(Graphics& graphics, const GameAssets& assets, const World& world, float elapsed)
{
    constexpr float glow_diameter = 96.0f;
    constexpr float pulse_speed = 6.0f;
    constexpr float pulse_size = 14.0f;
    constexpr float glow_alpha = 0.55f;

    for (auto [entity, spirit, transform] : world.view<Spirit, Transform>())
    {
        const Color color = spirit.is_golden ? spirit_gold : spirit_blue;
        const float pulse = pulse_size * std::sin(elapsed * pulse_speed + spirit.phase);
        draw_glow(graphics, assets, transform.position, glow_diameter + pulse,
                  faded(color, glow_alpha));
        graphics.draw_circle(transform.position, spirit_radius, color);
        graphics.draw_circle(transform.position, spirit_radius / 2.0f, Color::white);
    }
}

// One frame of the sprite sheet of the girl, or of its silhouette.
platform::TextureDraw girl_draw(Frame frame, Vec2 position, bool is_facing_left, Color tint)
{
    return {.source = {.position = {yozakura::frame_size.x * static_cast<float>(frame), 0.0f},
                       .size = yozakura::frame_size},
            .position = position,
            .scale = {girl_scale, girl_scale},
            .tint = tint,
            .is_flipped_x = is_facing_left};
}

void draw_girl(Graphics& graphics, const GameAssets& assets, const World& world, float elapsed)
{
    constexpr Color shadow_color{70, 26, 130, 255};
    constexpr float strongest_shadow = 0.5f;
    for (auto [entity, afterimage, transform, expiring] :
         world.view<Afterimage, Transform, Expiring>())
    {
        const float alpha = strongest_shadow * (expiring.remaining / expiring.lifetime);
        graphics.draw_texture(assets.girl_silhouette,
                              girl_draw(afterimage.frame, transform.position,
                                        afterimage.is_facing_left, faded(shadow_color, alpha)));
    }

    // While she stands, the girl moves up and down a little, as if breathing.
    constexpr float breath_speed = 3.0f;
    constexpr float breath_height = 2.0f;
    // The shadow on the ground shrinks and fades as she jumps higher.
    constexpr Vec2 ground_shadow_size{84.0f, 18.0f};
    constexpr float ground_shadow_offset = 2.0f;
    constexpr float ground_shadow_alpha = 0.6f;
    constexpr float highest_jump = 260.0f;
    constexpr float smallest_shadow = 0.45f;

    for (auto [entity, player, transform] : world.view<Player, Transform>())
    {
        const float height =
            std::clamp((player_ground_y - transform.position.y) / highest_jump, 0.0f, 1.0f);
        const float shadow_scale = std::lerp(1.0f, smallest_shadow, height);
        draw_image(graphics, assets.shadow, {transform.position.x, ground_y + ground_shadow_offset},
                   ground_shadow_size * shadow_scale,
                   faded(Color::white, ground_shadow_alpha * shadow_scale));

        const bool is_idle = player.is_on_ground && !player.is_running;
        const float breath = is_idle ? breath_height * std::sin(elapsed * breath_speed) : 0.0f;
        graphics.draw_texture(assets.girl, girl_draw(animation_frame(player, elapsed),
                                                     transform.position + Vec2{0.0f, breath},
                                                     player.is_facing_left, Color::white));
    }
}

// Drawn before the particles, so petals and flames are in front of the scene.
void draw_scene(World& world)
{
    Graphics& graphics = world.resource<Graphics>();
    const GameAssets& assets = world.resource<GameAssets>();
    const Game& game = world.resource<Game>();
    const auto elapsed = static_cast<float>(world.resource<Time>().elapsed.count());

    graphics.begin_view(scene_view(world.resource<Window>().size(), game.shake_offset));
    draw_sky(graphics, assets, elapsed);
    draw_moon(graphics, assets, elapsed);
    draw_clouds(graphics, assets, elapsed);
    draw_landscape(graphics, assets);
    draw_gate(graphics);
    draw_lanterns(graphics, assets, elapsed);
    draw_tree(graphics);
    draw_ground(graphics);
    draw_spirits(graphics, assets, world, elapsed);
    draw_girl(graphics, assets, world, elapsed);
    graphics.end_view();
}

// Text with a dark copy behind it, which keeps it readable on any background.
void draw_label(Graphics& graphics, std::string_view text, Vec2 position, float size, Color color)
{
    constexpr Vec2 shadow_offset{2.0f, 2.0f};
    Color shadow = text_shadow;
    shadow.a = static_cast<std::uint8_t>(static_cast<float>(shadow.a) *
                                         static_cast<float>(color.a) / opaque);
    graphics.draw_text(text, position + shadow_offset, size, shadow);
    graphics.draw_text(text, position, size, color);
}

void draw_centered_label(Graphics& graphics, std::string_view text, float y, float size,
                         Color color)
{
    const float width = graphics.measure_text(text, size).x;
    draw_label(graphics, text, {(scene_size.x - width) / 2.0f, y}, size, color);
}

void draw_game_over(Graphics& graphics, const Game& game)
{
    constexpr Color veil{12, 4, 26, 170};
    constexpr float title_y = 170.0f;
    constexpr float title_size = 70.0f;
    constexpr float score_y = 262.0f;
    constexpr float score_size = 30.0f;
    constexpr float hint_y = 320.0f;
    constexpr float hint_size = 22.0f;

    graphics.draw_rect({.position = {}, .size = scene_size}, veil);
    draw_centered_label(graphics, "GAME OVER", title_y, title_size, text_color);
    draw_centered_label(graphics,
                        std::format("SPIRITS  {}      BEST  {}", game.score, game.best_score),
                        score_y, score_size, spirit_gold);
    draw_centered_label(graphics, "ENTER  play again", hint_y, hint_size, dim_text_color);
}

// Bars over the area beside or above and below the scene, when the window has other
// proportions than the scene. Without them, petals would be seen outside the scene.
void draw_bars(Graphics& graphics, Vec2 window_size, float zoom)
{
    const Vec2 margin = (window_size - scene_size * zoom) / 2.0f;
    if (margin.x > 0.0f)
    {
        graphics.draw_rect({.position = {}, .size = {margin.x, window_size.y}}, bar_color);
        graphics.draw_rect(
            {.position = {window_size.x - margin.x, 0.0f}, .size = {margin.x, window_size.y}},
            bar_color);
    }
    if (margin.y > 0.0f)
    {
        graphics.draw_rect({.position = {}, .size = {window_size.x, margin.y}}, bar_color);
        graphics.draw_rect(
            {.position = {0.0f, window_size.y - margin.y}, .size = {window_size.x, margin.y}},
            bar_color);
    }
}

// Drawn after the particles, so the text stays readable. It does not shake with the scene.
void draw_hud(World& world)
{
    constexpr Vec2 score_position{24.0f, 18.0f};
    constexpr float score_size = 30.0f;
    constexpr Vec2 best_position{24.0f, 54.0f};
    constexpr float best_size = 20.0f;
    constexpr Vec2 hint_position{24.0f, 502.0f};
    constexpr float hint_size = 20.0f;
    constexpr Vec2 heart_size{35.0f, 30.0f};
    constexpr float heart_spacing = 44.0f;
    constexpr Vec2 last_heart{914.0f, 36.0f};
    constexpr Color full_heart{255, 92, 150, 255};
    constexpr Color empty_heart{74, 50, 92, 255};
    constexpr float floating_score_size = 26.0f;

    Graphics& graphics = world.resource<Graphics>();
    const GameAssets& assets = world.resource<GameAssets>();
    const Game& game = world.resource<Game>();
    const Vec2 window_size = world.resource<Window>().size();
    const platform::View view = scene_view(window_size);
    const World& const_world = world;

    graphics.begin_view(view);
    draw_image(graphics, assets.vignette, scene_size / 2.0f, scene_size);

    for (auto [entity, score, transform, expiring] :
         const_world.view<FloatingScore, Transform, Expiring>())
    {
        draw_label(graphics, std::format("+{}", score.points), transform.position,
                   floating_score_size,
                   faded(score.points > 1 ? spirit_gold : text_color,
                         expiring.remaining / expiring.lifetime));
    }

    draw_label(graphics, std::format("SPIRITS  {}", game.score), score_position, score_size,
               text_color);
    draw_label(graphics, std::format("BEST  {}", game.best_score), best_position, best_size,
               dim_text_color);
    for (int life = 0; life < max_lives; ++life)
    {
        // Lives are lost from the left, so the hearts on the right are the ones still full.
        const bool is_full = life < game.lives;
        const float x = last_heart.x - heart_spacing * static_cast<float>(life);
        draw_image(graphics, assets.heart, {x, last_heart.y}, heart_size,
                   is_full ? full_heart : empty_heart);
    }
    draw_label(graphics, "A / D  move     SPACE  jump     M  music     ESC  quit", hint_position,
               hint_size, dim_text_color);

    if (game.is_over)
    {
        draw_game_over(graphics, game);
    }
    graphics.end_view();

    draw_bars(graphics, window_size, view.zoom);
}

} // namespace

int main()
{
    // Music is decoded from these bytes while it plays, so they are created before the
    // application and outlive its audio service.
    const std::vector<std::byte> music = yozakura::music_wav();

    Application app;
    app.add_plugin(platform::PlatformPlugin{.settings = {.window = {.title = "Yozakura",
                                                                    .width = window_width,
                                                                    .height = window_height,
                                                                    .is_resizable = true},
                                                         .clear_color = clear_color}});
    // Without a window there is nothing to play on; the plugin has already logged the reason.
    if (!app.is_running())
    {
        return 1;
    }

    app.add_system(Stage::Startup, "start_game",
                   [&music](World& world)
                   {
                       bind_keys(world.resource<Input>());
                       world.insert_resource(load_assets(world, music));
                       world.insert_resource(Game{});
                       create_scene(world);
                   });
    app.add_system(Stage::PreUpdate, "handle_keys", handle_keys);
    app.add_system(Stage::FixedUpdate, "move_player", move_player);
    app.add_system(Stage::Update, "leave_afterimages", leave_afterimages);
    app.add_system(Stage::Update, "move_spirits", move_spirits);
    app.add_system(Stage::Update, "spawn_spirits", spawn_spirits);
    app.add_system(Stage::Update, "catch_spirits", catch_spirits);
    app.add_system(Stage::Update, "react_to_spirits", react_to_spirits);
    app.add_system(Stage::Update, "float_scores", float_scores);
    app.add_system(Stage::Update, "remove_expired", remove_expired);
    app.add_system(Stage::PostUpdate, "update_camera", update_camera);

    // The order of the systems in Stage::Render is the drawing order: scene, particles, text.
    app.add_system(Stage::Render, "draw_scene", draw_scene);
    app.add_plugin(ember::particles::ParticlePlugin{});
    app.add_system(Stage::Render, "draw_hud", draw_hud);

    app.run();
    return 0;
}
