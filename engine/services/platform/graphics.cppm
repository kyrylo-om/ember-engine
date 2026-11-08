export module ember.platform:graphics;

import :image;
import :platform_error;
import ember.common;
import std;

export namespace ember::platform
{

/// Maps world coordinates to the screen while drawing between `Graphics::begin_view()` and
/// `Graphics::end_view()`.
struct View
{
    /// Point of the world that appears at `offset` on the screen, in world units.
    Vec2 target{};
    /// Point of the screen where `target` appears, in pixels; usually the center of the
    /// window.
    Vec2 offset{};
    /// Scale: at 1, one world unit is one pixel; at 2, everything is drawn twice as large.
    float zoom = 1.0f;
    /// Rotation of the view around `target` in radians, clockwise.
    float rotation = 0.0f;
};

/// How a texture is placed on the screen by `Graphics::draw_texture()`.
struct TextureDraw
{
    /// Part of the texture to draw, in pixels from its top-left corner. A size of 0 draws
    /// the whole texture.
    Rect source{};
    /// Point where the center of the drawn image is placed.
    Vec2 position{};
    /// Scale on each axis; at 1, one pixel of the texture covers one unit.
    Vec2 scale{1.0f, 1.0f};
    /// Rotation around the center in radians, clockwise.
    float rotation = 0.0f;
    /// Color multiplied with the texture; white draws it unchanged.
    Color tint = Color::white;
    /// Whether the image is mirrored horizontally.
    bool is_flipped_x = false;
    /// Whether the image is mirrored vertically.
    bool is_flipped_y = false;
};

/// Textures, fonts and 2D drawing.
///
/// Owns all textures and fonts and frees them when it is destroyed. They are referred to by
/// handles; a handle to a freed resource is stale, and drawing with it draws nothing.
/// Coordinates are pixels from the top-left corner of the window, or world units between
/// `begin_view()` and `end_view()`.
///
/// Every frame is drawn between `begin_frame()` and `end_frame()`; the platform plugin
/// calls them in `Stage::PreRender` and `Stage::PostRender`.
/// @note Needs an open `Window` for its whole lifetime and must be used only from the main
/// thread.
class Graphics
{
public:
    /// Creates a graphics service without textures and fonts.
    Graphics();

    /// Frees all textures and fonts.
    ~Graphics();

    Graphics(const Graphics&) = delete;
    Graphics& operator=(const Graphics&) = delete;

    /// Takes over all textures and fonts; handles to them stay valid.
    Graphics(Graphics&& other) noexcept;

    /// Frees the textures and fonts of this object, then takes over those of `other`.
    Graphics& operator=(Graphics&& other) noexcept;

    /// Creates a texture from a decoded image.
    /// @param image Image to upload to the graphics card.
    /// @return Handle to the texture, or `PlatformError::InvalidData` if the image is empty
    /// or its pixel count does not match its size.
    [[nodiscard]] std::expected<Handle<Texture>, PlatformError> create_texture(const Image& image);

    /// Decodes an image file in memory and creates a texture from it.
    /// @param data Contents of the file.
    /// @param format File extension with the dot, as for `decode_image()`.
    /// @return Handle to the texture, or `PlatformError::InvalidData` if the data cannot be
    /// decoded.
    [[nodiscard]] std::expected<Handle<Texture>, PlatformError>
    load_texture(std::span<const std::byte> data, std::string_view format);

    /// Frees a texture. Its handles become stale.
    /// @param texture Texture to free. Nothing happens for a null or stale handle.
    void unload_texture(Handle<Texture> texture);

    /// Returns the size of a texture in pixels.
    /// @param texture Texture whose size is returned.
    /// @return Width and height, or `std::nullopt` for a null or stale handle.
    [[nodiscard]] std::optional<Vec2> texture_size(Handle<Texture> texture) const;

    /// Loads a font from a font file in memory.
    /// @param data Contents of the file.
    /// @param format File extension with the dot: ".ttf" or ".otf".
    /// @param size Height of the glyphs in pixels. Text drawn at this size looks sharpest.
    /// @return Handle to the font, or `PlatformError::InvalidData` if the data cannot be
    /// decoded.
    [[nodiscard]] std::expected<Handle<Font>, PlatformError>
    load_font(std::span<const std::byte> data, std::string_view format, int size);

    /// Frees a font. Its handles become stale.
    /// @param font Font to free. Nothing happens for a null or stale handle.
    void unload_font(Handle<Font> font);

    /// Starts drawing a frame and fills the window with a color.
    /// @param clear_color Background color of the frame.
    void begin_frame(Color clear_color);

    /// Finishes the frame and shows it in the window.
    /// @note With `WindowSettings::is_vsync`, waits for the vertical refresh of the monitor.
    void end_frame();

    /// Starts drawing in world coordinates mapped to the screen by a view. Views do not
    /// nest: calling it again replaces the current view.
    /// @param view Mapping from the world to the screen.
    void begin_view(const View& view);

    /// Returns to drawing in screen coordinates.
    void end_view();

    /// Draws a texture or a part of it.
    /// @param texture Texture to draw. Nothing is drawn for a null or stale handle.
    /// @param draw Part of the texture, position, rotation, scale and color.
    void draw_texture(Handle<Texture> texture, const TextureDraw& draw);

    /// Draws a filled rectangle.
    /// @param rect Rectangle to fill.
    /// @param color Fill color.
    void draw_rect(Rect rect, Color color);

    /// Draws the outline of a rectangle.
    /// @param rect Rectangle whose outline is drawn; the outline lies inside it.
    /// @param thickness Width of the outline.
    /// @param color Outline color.
    void draw_rect_outline(Rect rect, float thickness, Color color);

    /// Draws a filled circle.
    /// @param center Center of the circle.
    /// @param radius Radius of the circle.
    /// @param color Fill color.
    void draw_circle(Vec2 center, float radius, Color color);

    /// Draws the outline of a circle with a width of one pixel.
    /// @param center Center of the circle.
    /// @param radius Radius of the circle.
    /// @param color Outline color.
    void draw_circle_outline(Vec2 center, float radius, Color color);

    /// Draws a straight line.
    /// @param from Start point.
    /// @param to End point.
    /// @param thickness Width of the line.
    /// @param color Line color.
    void draw_line(Vec2 from, Vec2 to, float thickness, Color color);

    /// Draws a line of text.
    /// @param text Text in UTF-8.
    /// @param position Top-left corner of the text.
    /// @param size Height of the text.
    /// @param color Text color.
    /// @param font Font to draw with; the null handle uses the built-in font.
    void draw_text(std::string_view text, Vec2 position, float size, Color color,
                   Handle<Font> font = {});

    /// Returns the size of a line of text as `draw_text()` would draw it.
    /// @param text Text in UTF-8.
    /// @param size Height of the text.
    /// @param font Font to measure with; the null handle uses the built-in font.
    /// @return Width and height of the text.
    [[nodiscard]] Vec2 measure_text(std::string_view text, float size,
                                    Handle<Font> font = {}) const;

private:
    // The tables hold Raylib types, which must not appear in the interface, so they are
    // defined in the implementation.
    struct Resources;
    std::unique_ptr<Resources> m_resources;
};

/// Converts a point on the screen to the world through a view.
/// @param view View that maps the world to the screen.
/// @param point Point on the screen in pixels.
/// @return Point in the world.
Vec2 screen_to_world(const View& view, Vec2 point);

/// Converts a point in the world to the screen through a view.
/// @param view View that maps the world to the screen.
/// @param point Point in the world.
/// @return Point on the screen in pixels.
Vec2 world_to_screen(const View& view, Vec2 point);

} // namespace ember::platform
