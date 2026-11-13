module ember.platform;

import ember.common;
import raylib;
import std;

namespace ember::platform
{

std::expected<Image, PlatformError> decode_image(std::span<const std::byte> data,
                                                 std::string_view format)
{
    if (data.empty() || !std::in_range<int>(data.size()))
    {
        return std::unexpected{PlatformError::InvalidData};
    }

    try
    {
        raylib::Image decoded{std::string{format},
                              reinterpret_cast<const unsigned char*>(data.data()),
                              static_cast<int>(data.size())};
        // Raylib keeps the pixel format of the file. With four 8-bit channels per pixel its
        // pixels have the same layout as Color and can be copied as they are.
        decoded.Format(raylib::PixelFormat::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        const std::size_t pixel_count =
            static_cast<std::size_t>(decoded.width) * static_cast<std::size_t>(decoded.height);
        const auto* pixels = static_cast<const Color*>(decoded.data);
        return Image{.width = decoded.width,
                     .height = decoded.height,
                     .pixels = std::vector<Color>(pixels, pixels + pixel_count)};
    }
    catch (const raylib::RaylibException&)
    {
        return std::unexpected{PlatformError::InvalidData};
    }
}

} // namespace ember::platform
