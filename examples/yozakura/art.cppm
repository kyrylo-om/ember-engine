// Pictures of the game, made in code: the animated sprite of the girl and the images of the
// scenery. The game turns them into textures when it starts, so it needs no image files.
export module yozakura.art;

import ember.common;
import ember.platform;
import std;

export namespace yozakura
{

/// Frames of the sprite sheet of the girl, in the order they are stored from left to right.
enum class Frame
{
    Standing,
    Blinking,
    Striding,
    Passing,
    Jumping
};

/// Size of one frame of the sprite sheet in pixels.
inline constexpr ember::Vec2 frame_size{20.0f, 29.0f};

/// Returns the sprite sheet of the girl: all frames next to each other.
ember::platform::Image girl_sheet();

/// Returns an image with the shape of another one in plain white, for drawing it in a
/// single color, such as a shadow.
/// @param image Image whose shape is copied.
ember::platform::Image silhouette_of(const ember::platform::Image& image);

/// Returns a white round glow that fades out towards its edge.
ember::platform::Image glow_image();

/// Returns the night sky: one column of pixels from the dark top to the glow at the horizon.
ember::platform::Image sky_image();

/// Returns the full moon with its craters.
ember::platform::Image moon_image();

/// Returns a snow-capped mountain.
ember::platform::Image mountain_image();

/// Returns a row of dark hills.
ember::platform::Image hills_image();

/// Returns a soft white cloud.
ember::platform::Image cloud_image();

/// Returns the soft dark ellipse drawn on the ground under the girl.
ember::platform::Image shadow_image();

/// Returns a small white heart, drawn once for every life.
ember::platform::Image heart_image();

/// Returns a frame that is transparent in the middle and dark at the edges of the screen.
ember::platform::Image vignette_image();

} // namespace yozakura

namespace yozakura
{

using ember::Color;
using ember::Vec2;
using ember::platform::Image;

// ---------------------------------------------------------------------------------------
// The sprite of the girl. Every character is one pixel; the palette is below. The sprite is
// put together from a head, a body and legs, so the frames of the animation share the parts
// that do not move.
// ---------------------------------------------------------------------------------------

constexpr std::size_t sprite_width = 20;

constexpr std::array<std::string_view, 16> head_rows{
    "......kkkkkkkk......", //
    "....kkKKkkkkkkkk....", //
    "...kkKKkkkkkkkpkk...", //
    "..kkkKkkkkkkkppkkk..", //
    "..kkkkkkkkkkkppkkk..", //
    "..kkkkskkkkskkpkkk..", //
    ".kkkkssskksssskpkkk.", //
    ".kkksssssssssssskkk.", //
    "kkkksEEEssssEEEskkkk", //
    "kkkksEewssssEewskkkk", //
    "kkkkseeEsssseeEskkkk", //
    "kkkkSrrssssssrrSkkkk", //
    "kkkksssssrrssssskkkk", //
    "kkk..ssssssssss..kkk", //
    "kkk....ssssss....kkk", //
    "kpk....cxccxc....kpk", //
};

// Rows of the head that are replaced while the eyes are closed.
constexpr std::size_t first_eye_row = 8;
constexpr std::array<std::string_view, 3> closed_eye_rows{
    "kkkksssssssssssskkkk", //
    "kkkksEEEssssEEEskkkk", //
    "kkkksssssssssssskkkk", //
};

constexpr std::array<std::string_view, 7> body_rows{
    "kpk..dddddddddd..kpk", //
    "kkp..gdddwwdddg..pkk", //
    ".kp..sddwwwwdds..pk.", //
    ".pp..gdddwwdddg..pp.", //
    ".p...sdddwwddds...p.", //
    ".....xVvVvVvVvx.....", //
    "....vVvVvVvVvVvV....", //
};

using LegRows = std::array<std::string_view, 6>;

constexpr LegRows standing_legs{
    "......sss..sss......", //
    "......ggg..ggg......", //
    "......www..www......", //
    "......ggg..ggg......", //
    ".....bbbb..bbbb.....", //
    ".....bBbb..bBbb.....", //
};

constexpr LegRows striding_legs{
    "......sss..sss......", //
    ".....ggg....ggg.....", //
    "....www......www....", //
    "...ggg........ggg...", //
    "..bbbb........bbbb..", //
    "..bBbb........bBbb..", //
};

constexpr LegRows passing_legs{
    "......sss..sss......", //
    ".......ggggg........", //
    ".......wwwww........", //
    "......ggg.ggg.......", //
    ".....bbbb..bbbb.....", //
    ".....bBbb...........", //
};

constexpr LegRows jumping_legs{
    "......sss..sss......", //
    ".....ggg....ggg.....", //
    "....www......www....", //
    "....bbbb....bbbb....", //
    "....bBbb....bBbb....", //
    "....................", //
};

constexpr std::size_t frame_count = static_cast<std::size_t>(Frame::Jumping) + 1;
constexpr std::size_t sprite_height = head_rows.size() + body_rows.size() + standing_legs.size();

constexpr bool have_sprite_width(std::span<const std::string_view> rows)
{
    return std::ranges::all_of(rows,
                               [](std::string_view row) { return row.size() == sprite_width; });
}
static_assert(have_sprite_width(head_rows) && have_sprite_width(closed_eye_rows) &&
                  have_sprite_width(body_rows) && have_sprite_width(standing_legs) &&
                  have_sprite_width(striding_legs) && have_sprite_width(passing_legs) &&
                  have_sprite_width(jumping_legs),
              "every row of the sprite must be sprite_width pixels wide");

static_assert(frame_size ==
                  Vec2{static_cast<float>(sprite_width), static_cast<float>(sprite_height)},
              "frame_size must be the size of the sprite rows");

constexpr Color sprite_color(char symbol)
{
    switch (symbol)
    {
        case 'k': // hair
            return {26, 22, 38, 255};
        case 'K': // shine on the hair
            return {74, 62, 108, 255};
        case 'p': // dyed streak and tips of the twin tails
            return {255, 70, 170, 255};
        case 's': // skin
            return {255, 228, 214, 255};
        case 'S': // shadow on the skin
            return {242, 196, 190, 255};
        case 'E': // eyeliner and pupils
            return {44, 24, 80, 255};
        case 'e': // eyes
            return {176, 96, 255, 255};
        case 'r': // blush and lips
            return {255, 120, 150, 255};
        case 'c': // choker
            return {16, 14, 22, 255};
        case 'x': // studs of the choker and the belt
            return {214, 214, 228, 255};
        case 'd': // top
            return {36, 32, 50, 255};
        case 'w': // white details
            return {244, 244, 252, 255};
        case 'v': // skirt
            return {150, 86, 220, 255};
        case 'V': // darker stripes of the skirt
            return {88, 46, 150, 255};
        case 'g': // fishnet sleeves and striped stockings
            return {112, 106, 134, 255};
        case 'b': // boots
            return {14, 14, 20, 255};
        case 'B': // shine on the boots
            return {70, 70, 90, 255};
        default:
            return Color::transparent;
    }
}

// Returns the rows of one frame from top to bottom.
std::vector<std::string_view> frame_rows(Frame frame)
{
    std::vector<std::string_view> rows{std::from_range, head_rows};
    if (frame == Frame::Blinking)
    {
        std::ranges::copy(closed_eye_rows, rows.begin() + first_eye_row);
    }
    rows.append_range(body_rows);
    switch (frame)
    {
        case Frame::Standing:
        case Frame::Blinking:
            rows.append_range(standing_legs);
            break;
        case Frame::Striding:
            rows.append_range(striding_legs);
            break;
        case Frame::Passing:
            rows.append_range(passing_legs);
            break;
        case Frame::Jumping:
            rows.append_range(jumping_legs);
            break;
    }
    return rows;
}

// ---------------------------------------------------------------------------------------
// Helpers for images that are computed pixel by pixel.
// ---------------------------------------------------------------------------------------

constexpr float opaque = 255.0f;

Image blank_image(int width, int height)
{
    return {.width = width,
            .height = height,
            .pixels =
                std::vector<Color>(static_cast<std::size_t>(width * height), Color::transparent)};
}

Color& pixel(Image& image, int x, int y)
{
    return image.pixels[static_cast<std::size_t>(y * image.width + x)];
}

std::uint8_t to_channel(float value)
{
    return static_cast<std::uint8_t>(std::clamp(value, 0.0f, opaque));
}

// Returns a color with its alpha replaced by a value from 0 (transparent) to 1 (opaque).
Color faded(Color color, float alpha)
{
    color.a = to_channel(alpha * opaque);
    return color;
}

Color blend(Color from, Color to, float amount)
{
    const auto channel = [amount](std::uint8_t start, std::uint8_t end)
    {
        return to_channel(std::lerp(static_cast<float>(start), static_cast<float>(end), amount));
    };
    return {channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b),
            channel(from.a, to.a)};
}

// Returns the center of a pixel as a point from -1 to 1 on both axes, with 0 in the middle
// of the image.
Vec2 centered(const Image& image, int x, int y)
{
    constexpr float pixel_center = 0.5f;
    return {(static_cast<float>(x) + pixel_center) / static_cast<float>(image.width) * 2.0f - 1.0f,
            (static_cast<float>(y) + pixel_center) / static_cast<float>(image.height) * 2.0f -
                1.0f};
}

// How opaque a pixel at the edge of a round shape is: 1 inside, 0 outside and a soft step
// between, which hides the staircase of the pixels.
float edge_alpha(float distance, float radius, float softness)
{
    return std::clamp((radius - distance) / softness, 0.0f, 1.0f);
}

// ---------------------------------------------------------------------------------------
// The images.
// ---------------------------------------------------------------------------------------

Image girl_sheet()
{
    Image image =
        blank_image(static_cast<int>(sprite_width * frame_count), static_cast<int>(sprite_height));
    for (std::size_t frame = 0; frame < frame_count; ++frame)
    {
        const std::vector<std::string_view> rows = frame_rows(static_cast<Frame>(frame));
        for (std::size_t y = 0; y < sprite_height; ++y)
        {
            for (std::size_t x = 0; x < sprite_width; ++x)
            {
                image.pixels[y * sprite_width * frame_count + frame * sprite_width + x] =
                    sprite_color(rows[y][x]);
            }
        }
    }
    return image;
}

Image silhouette_of(const Image& image)
{
    Image silhouette = image;
    for (Color& color : silhouette.pixels)
    {
        color = faded(Color::white, static_cast<float>(color.a) / opaque);
    }
    return silhouette;
}

Image glow_image()
{
    constexpr int size = 128;
    Image image = blank_image(size, size);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            // Squared, so the glow is bright in the middle and has a long faint edge.
            const float brightness = std::max(0.0f, 1.0f - centered(image, x, y).length());
            pixel(image, x, y) = faded(Color::white, brightness * brightness);
        }
    }
    return image;
}

Image sky_image()
{
    struct Stop
    {
        // Height in the sky from 0 (top) to 1 (horizon).
        float height = 0.0f;
        Color color{};
    };
    constexpr std::array<Stop, 4> stops{
        Stop{0.0f, {10, 6, 30, 255}},
        Stop{0.45f, {44, 18, 82, 255}},
        Stop{0.8f, {128, 42, 116, 255}},
        Stop{1.0f, {232, 112, 132, 255}},
    };
    constexpr int height = 256;

    Image image = blank_image(1, height);
    for (int y = 0; y < height; ++y)
    {
        const float position = static_cast<float>(y) / static_cast<float>(height - 1);
        std::size_t next = 1;
        while (next + 1 < stops.size() && stops[next].height < position)
        {
            ++next;
        }
        const Stop& from = stops[next - 1];
        const Stop& to = stops[next];
        pixel(image, 0, y) =
            blend(from.color, to.color, (position - from.height) / (to.height - from.height));
    }
    return image;
}

Image moon_image()
{
    struct Crater
    {
        Vec2 center{};
        float radius = 0.0f;
    };
    constexpr std::array<Crater, 6> craters{
        Crater{{-0.35f, -0.3f}, 0.2f}, Crater{{0.3f, -0.45f}, 0.12f},
        Crater{{0.4f, 0.15f}, 0.24f},  Crater{{-0.2f, 0.4f}, 0.16f},
        Crater{{-0.55f, 0.15f}, 0.1f}, Crater{{0.05f, -0.05f}, 0.09f},
    };
    constexpr int size = 96;
    constexpr float radius = 0.96f;
    constexpr float softness = 0.04f;
    constexpr Color surface{255, 232, 238, 255};
    constexpr Color crater_color{240, 198, 216, 255};
    // The lower left side is a little darker, which makes the disc look round.
    constexpr Color shade{236, 190, 214, 255};
    constexpr Vec2 light_direction{0.6f, -0.6f};
    constexpr float shade_strength = 0.35f;

    Image image = blank_image(size, size);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const Vec2 point = centered(image, x, y);
            const float darkness =
                std::clamp(-ember::dot(point, light_direction) * shade_strength, 0.0f, 1.0f);
            Color color = blend(surface, shade, darkness);
            for (const Crater& crater : craters)
            {
                color =
                    blend(color, crater_color,
                          edge_alpha((point - crater.center).length(), crater.radius, softness));
            }
            pixel(image, x, y) = faded(color, edge_alpha(point.length(), radius, softness));
        }
    }
    return image;
}

Image mountain_image()
{
    constexpr int width = 320;
    constexpr int height = 120;
    constexpr Color rock{50, 30, 88, 255};
    constexpr Color rock_shadow{36, 20, 68, 255};
    constexpr Color snow{226, 208, 238, 255};
    constexpr Color snow_shadow{176, 150, 208, 255};
    // The slopes are steeper near the top, like on a volcano.
    constexpr float slope_power = 1.7f;
    // The top is cut off flat, where the crater is.
    constexpr float crater_height = 0.94f;
    constexpr float snow_line = 0.62f;
    constexpr float snow_zigzag_size = 0.06f;
    constexpr float snow_zigzag_speed = 0.35f;
    // The moon lights the mountain from the right, so it gets darker towards the left. The
    // shade changes gradually over this part of the width; a sharp line would look like an
    // edge of the mountain.
    constexpr float shade_start = -0.5f;
    constexpr float shade_width = 1.0f;

    Image image = blank_image(width, height);
    for (int x = 0; x < width; ++x)
    {
        const float from_center = std::abs(centered(image, x, 0).x);
        const float top = std::min(std::pow(1.0f - from_center, slope_power), crater_height);
        const float shade =
            std::clamp((shade_start - centered(image, x, 0).x) / shade_width + 1.0f, 0.0f, 1.0f);
        for (int y = 0; y < height; ++y)
        {
            // Height of the pixel from 0 (foot of the mountain) to 1 (top of the image).
            const float level = 1.0f - static_cast<float>(y) / static_cast<float>(height);
            if (level > top)
            {
                continue;
            }
            const float snow_edge =
                snow_line + snow_zigzag_size * std::sin(static_cast<float>(x) * snow_zigzag_speed);
            if (level > snow_edge)
            {
                pixel(image, x, y) = blend(snow, snow_shadow, shade);
            }
            else
            {
                pixel(image, x, y) = blend(rock, rock_shadow, shade);
            }
        }
    }
    return image;
}

Image hills_image()
{
    constexpr int width = 480;
    constexpr int height = 64;
    constexpr Color color{24, 12, 44, 255};
    constexpr float base_height = 28.0f;
    // Two waves of different length give hills of different sizes.
    constexpr float long_wave_height = 14.0f;
    constexpr float long_wave_speed = 0.021f;
    constexpr float short_wave_height = 8.0f;
    constexpr float short_wave_speed = 0.057f;

    Image image = blank_image(width, height);
    for (int x = 0; x < width; ++x)
    {
        const auto position = static_cast<float>(x);
        const float hill_height = base_height +
                                  long_wave_height * std::sin(position * long_wave_speed) +
                                  short_wave_height * std::sin(position * short_wave_speed + 1.0f);
        for (int y = height - static_cast<int>(hill_height); y < height; ++y)
        {
            pixel(image, x, y) = color;
        }
    }
    return image;
}

Image cloud_image()
{
    struct Puff
    {
        Vec2 center{};
        float radius = 0.0f;
    };
    // In units where the image is 2 wide and 2 high, like the result of centered().
    constexpr std::array<Puff, 5> puffs{
        Puff{{-0.55f, 0.25f}, 0.4f}, Puff{{-0.2f, -0.1f}, 0.55f}, Puff{{0.2f, 0.1f}, 0.5f},
        Puff{{0.55f, 0.3f}, 0.38f},  Puff{{0.0f, 0.45f}, 0.5f},
    };
    constexpr int width = 128;
    constexpr int height = 48;
    // The image is wider than high, so distances are stretched to keep the puffs round.
    constexpr float aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float softness = 0.35f;

    Image image = blank_image(width, height);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const Vec2 point = centered(image, x, y);
            float alpha = 0.0f;
            for (const Puff& puff : puffs)
            {
                const Vec2 offset{(point.x - puff.center.x) * aspect, point.y - puff.center.y};
                alpha = std::max(
                    alpha, edge_alpha(offset.length(), puff.radius * aspect / 2.0f, softness));
            }
            pixel(image, x, y) = faded(Color::white, alpha);
        }
    }
    return image;
}

Image shadow_image()
{
    constexpr int width = 48;
    constexpr int height = 12;
    Image image = blank_image(width, height);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            pixel(image, x, y) =
                faded(Color::black, std::max(0.0f, 1.0f - centered(image, x, y).length()));
        }
    }
    return image;
}

Image heart_image()
{
    constexpr std::array<std::string_view, 6> rows{
        ".ww.ww.", //
        "wwwwwww", //
        "wwwwwww", //
        ".wwwww.", //
        "..www..", //
        "...w...", //
    };
    Image image = blank_image(static_cast<int>(rows.front().size()), static_cast<int>(rows.size()));
    for (std::size_t y = 0; y < rows.size(); ++y)
    {
        for (std::size_t x = 0; x < rows[y].size(); ++x)
        {
            if (rows[y][x] == 'w')
            {
                pixel(image, static_cast<int>(x), static_cast<int>(y)) = Color::white;
            }
        }
    }
    return image;
}

Image vignette_image()
{
    constexpr int width = 240;
    constexpr int height = 135;
    constexpr Color color{8, 2, 22, 255};
    // Distance from the middle where the darkening starts, and how fast it grows.
    constexpr float clear_radius = 0.62f;
    constexpr float falloff = 0.85f;
    constexpr float strength = 0.7f;

    Image image = blank_image(width, height);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const float darkness =
                std::clamp((centered(image, x, y).length() - clear_radius) / falloff, 0.0f, 1.0f);
            pixel(image, x, y) = faded(color, darkness * darkness * strength);
        }
    }
    return image;
}

} // namespace yozakura
