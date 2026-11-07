export module ember.platform:image;

import :platform_error;
import ember.common;
import std;

export namespace ember::platform
{

/// Decoded image in memory, ready to become a texture.
struct Image
{
    /// Width in pixels.
    int width = 0;
    /// Height in pixels.
    int height = 0;
    /// Pixels row by row from the top-left corner; `width * height` elements.
    std::vector<Color> pixels{};
};

/// Decodes an image file that is already in memory, for example an embedded asset.
/// @param data Contents of the file.
/// @param format File extension that names the format, with the dot: ".png", ".bmp",
/// ".qoi" or ".gif" (first frame only).
/// @return The decoded image, or `PlatformError::InvalidData` if the data cannot be decoded.
/// @note Uses no graphics context, so it can be called from any thread, for example to
/// decode several assets in parallel.
[[nodiscard]] std::expected<Image, PlatformError> decode_image(std::span<const std::byte> data,
                                                               std::string_view format);

} // namespace ember::platform
