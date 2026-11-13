module ember.platform;

import :raylib_support;
import :resource_table;
import ember.common;
import raylib;
import std;

namespace ember::platform
{

namespace
{

// Number of straight lines that approximate the outline of a circle.
constexpr int circle_outline_segments = 48;

raylib::Camera2D to_camera(const View& view)
{
    // A view turned clockwise shows the world turned counterclockwise, and Raylib's camera
    // rotation turns the world, so the sign is flipped.
    return {to_raylib(view.offset), to_raylib(view.target), -to_degrees(view.rotation), view.zoom};
}

// Space between the letters that Raylib uses for its own text: one pixel at the size the
// font was loaded with, scaled with the text.
float letter_spacing(const raylib::Font& font, float size)
{
    return size / static_cast<float>(font.baseSize);
}

std::uint32_t read_big_endian(std::span<const std::byte> data, std::size_t offset,
                              std::size_t byte_count)
{
    std::uint32_t value = 0;
    for (const std::byte byte : data.subspan(offset, byte_count))
    {
        value = (value << 8) | std::to_integer<std::uint32_t>(byte);
    }
    return value;
}

// Checks that the data starts like a TrueType or OpenType font and that all tables it lists
// lie inside the data. Raylib's font decoder does not check this itself and reads outside
// the data of a corrupt file.
bool has_valid_font_tables(std::span<const std::byte> data)
{
    constexpr std::size_t header_size = 12;
    constexpr std::size_t table_record_size = 16;
    constexpr std::size_t table_count_offset = 4;
    constexpr std::size_t record_offset_field = 8;
    constexpr std::size_t record_length_field = 12;
    constexpr std::uint32_t true_type_version = 0x00010000;
    constexpr std::uint32_t open_type_tag = 0x4F54544F;       // "OTTO"
    constexpr std::uint32_t apple_true_type_tag = 0x74727565; // "true"

    if (data.size() < header_size)
    {
        return false;
    }
    const std::uint32_t version = read_big_endian(data, 0, sizeof(std::uint32_t));
    if (version != true_type_version && version != open_type_tag && version != apple_true_type_tag)
    {
        return false;
    }

    const std::size_t table_count =
        read_big_endian(data, table_count_offset, sizeof(std::uint16_t));
    if (table_count == 0 || header_size + table_count * table_record_size > data.size())
    {
        return false;
    }
    for (std::size_t table = 0; table < table_count; ++table)
    {
        const std::size_t record = header_size + table * table_record_size;
        const std::uint64_t offset =
            read_big_endian(data, record + record_offset_field, sizeof(std::uint32_t));
        const std::uint64_t length =
            read_big_endian(data, record + record_length_field, sizeof(std::uint32_t));
        if (offset + length > data.size())
        {
            return false;
        }
    }
    return true;
}

} // namespace

struct Graphics::Resources
{
    ResourceTable<Texture, raylib::Texture> textures{};
    ResourceTable<Font, raylib::Font> fonts{};
    // Raylib's built-in font, drawn with when no font is given.
    raylib::Font default_font{};
    // Whether drawing is between begin_view() and end_view().
    bool is_view_active = false;
};

Graphics::Graphics() : m_resources{std::make_unique<Resources>()}
{
}

Graphics::~Graphics() = default;
Graphics::Graphics(Graphics&& other) noexcept = default;
Graphics& Graphics::operator=(Graphics&& other) noexcept = default;

std::expected<Handle<Texture>, PlatformError> Graphics::create_texture(const Image& image)
{
    const bool has_size = image.width > 0 && image.height > 0;
    if (!has_size || image.pixels.size() != static_cast<std::size_t>(image.width) *
                                                static_cast<std::size_t>(image.height))
    {
        return std::unexpected{PlatformError::InvalidData};
    }

    // Raylib only reads the pixels while it uploads them, but its image type has no const
    // pointer.
    constexpr int mipmap_count = 1;
    raylib::Image source{const_cast<Color*>(image.pixels.data()), image.width, image.height,
                         mipmap_count, raylib::PixelFormat::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    std::expected<Handle<Texture>, PlatformError> result{
        std::unexpected{PlatformError::InvalidData}};
    try
    {
        result = m_resources->textures.add(raylib::Texture{source});
    }
    catch (const raylib::RaylibException&)
    {
    }
    // The pixels belong to the Image; without this the Raylib image would free them.
    source.data = nullptr;
    return result;
}

std::expected<Handle<Texture>, PlatformError>
Graphics::load_texture(std::span<const std::byte> data, std::string_view format)
{
    return decode_image(data, format)
        .and_then([this](const Image& image) { return create_texture(image); });
}

void Graphics::unload_texture(Handle<Texture> texture)
{
    m_resources->textures.remove(texture);
}

std::optional<Vec2> Graphics::texture_size(Handle<Texture> texture) const
{
    const raylib::Texture* found = m_resources->textures.find(texture);
    if (found == nullptr)
    {
        return std::nullopt;
    }
    return Vec2{static_cast<float>(found->width), static_cast<float>(found->height)};
}

std::expected<Handle<Font>, PlatformError> Graphics::load_font(std::span<const std::byte> data,
                                                               std::string_view format, int size)
{
    if (!has_valid_font_tables(data) || !std::in_range<int>(data.size()) || size <= 0)
    {
        return std::unexpected{PlatformError::InvalidData};
    }

    try
    {
        // No list of characters: Raylib then loads the printable ASCII characters.
        raylib::Font font{std::string{format},
                          reinterpret_cast<const unsigned char*>(data.data()),
                          static_cast<int>(data.size()),
                          size,
                          nullptr,
                          0};
        // For data it cannot decode, Raylib returns its built-in font instead of failing.
        if (font.texture.id == m_resources->default_font.texture.id)
        {
            return std::unexpected{PlatformError::InvalidData};
        }
        return m_resources->fonts.add(std::move(font));
    }
    catch (const raylib::RaylibException&)
    {
        return std::unexpected{PlatformError::InvalidData};
    }
}

void Graphics::unload_font(Handle<Font> font)
{
    m_resources->fonts.remove(font);
}

void Graphics::begin_frame(Color clear_color)
{
    raylib_window().BeginDrawing();
    raylib_window().ClearBackground(to_raylib(clear_color));
}

void Graphics::end_frame()
{
    raylib_window().EndDrawing();
}

void Graphics::begin_view(const View& view)
{
    // Raylib does not nest 2D modes either; ending the current one first keeps its matrix
    // stack balanced when a view replaces another.
    if (m_resources->is_view_active)
    {
        end_view();
    }
    to_camera(view).BeginMode();
    m_resources->is_view_active = true;
}

void Graphics::end_view()
{
    if (m_resources->is_view_active)
    {
        raylib::Camera2D{}.EndMode();
        m_resources->is_view_active = false;
    }
}

void Graphics::draw_texture(Handle<Texture> texture, const TextureDraw& draw)
{
    const raylib::Texture* found = m_resources->textures.find(texture);
    if (found == nullptr)
    {
        return;
    }

    Rect source = draw.source;
    if (source.size == Vec2{})
    {
        source.size = {static_cast<float>(found->width), static_cast<float>(found->height)};
    }
    const Vec2 size{source.size.x * std::abs(draw.scale.x), source.size.y * std::abs(draw.scale.y)};

    // Raylib mirrors the image when the source rectangle has a negative size. A negative
    // scale mirrors too, so it cancels a flip.
    raylib::Rectangle source_rect = to_raylib(source);
    if (draw.is_flipped_x != (draw.scale.x < 0.0f))
    {
        source_rect.width = -source_rect.width;
    }
    if (draw.is_flipped_y != (draw.scale.y < 0.0f))
    {
        source_rect.height = -source_rect.height;
    }

    // The origin is the point of the destination that is placed at its position and rotated
    // around: the center.
    const raylib::Rectangle destination{draw.position.x, draw.position.y, size.x, size.y};
    found->Draw(source_rect, destination, to_raylib(size / 2.0f), to_degrees(draw.rotation),
                to_raylib(draw.tint));
}

void Graphics::draw_rect(Rect rect, Color color)
{
    to_raylib(rect).Draw(to_raylib(color));
}

void Graphics::draw_rect_outline(Rect rect, float thickness, Color color)
{
    to_raylib(rect).DrawLines(to_raylib(color), thickness);
}

void Graphics::draw_circle(Vec2 center, float radius, Color color)
{
    to_raylib(center).DrawCircle(radius, to_raylib(color));
}

void Graphics::draw_circle_outline(Vec2 center, float radius, Color color)
{
    const float step =
        2.0f * std::numbers::pi_v<float> / static_cast<float>(circle_outline_segments);
    Vec2 from = center + Vec2{radius, 0.0f};
    for (int segment = 1; segment <= circle_outline_segments; ++segment)
    {
        const float angle = step * static_cast<float>(segment);
        const Vec2 to = center + Vec2{std::cos(angle), std::sin(angle)} * radius;
        to_raylib(from).DrawLine(to_raylib(to), to_raylib(color));
        from = to;
    }
}

void Graphics::draw_line(Vec2 from, Vec2 to, float thickness, Color color)
{
    to_raylib(from).DrawLine(to_raylib(to), thickness, to_raylib(color));
}

void Graphics::draw_text(std::string_view text, Vec2 position, float size, Color color,
                         Handle<Font> font)
{
    const raylib::Font* found = m_resources->fonts.find(font);
    const raylib::Font& used = found != nullptr ? *found : m_resources->default_font;
    used.DrawText(std::string{text}, to_raylib(position), size, letter_spacing(used, size),
                  to_raylib(color));
}

Vec2 Graphics::measure_text(std::string_view text, float size, Handle<Font> font) const
{
    const raylib::Font* found = m_resources->fonts.find(font);
    const raylib::Font& used = found != nullptr ? *found : m_resources->default_font;
    return to_vec2(used.MeasureText(std::string{text}, size, letter_spacing(used, size)));
}

Vec2 screen_to_world(const View& view, Vec2 point)
{
    return to_vec2(to_camera(view).GetScreenToWorld(to_raylib(point)));
}

Vec2 world_to_screen(const View& view, Vec2 point)
{
    return to_vec2(to_camera(view).GetWorldToScreen(to_raylib(point)));
}

} // namespace ember::platform
