#include <catch_amalgamated.hpp>

import ember.common;
import ember.platform;
import std;

using ember::Color;
using ember::platform::decode_image;
using ember::platform::PlatformError;

namespace
{

// A BMP file of 2 x 1 pixels with 24 bits per pixel: a red pixel, then a green one.
constexpr auto bitmap = std::to_array<unsigned char>({
    // File header: "BM", file size 62, reserved, pixel data at offset 54.
    0x42,
    0x4D,
    62,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    54,
    0,
    0,
    0,
    // Info header: header size 40, width 2, height 1, 1 plane, 24 bits, no compression.
    40,
    0,
    0,
    0,
    2,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    1,
    0,
    24,
    0,
    0,
    0,
    0,
    0,
    // Size of the pixel data 8, resolution and palette fields unused.
    8,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // Pixels as blue, green, red; the row is padded to 4 bytes.
    0,
    0,
    255,
    0,
    255,
    0,
    0,
    0,
});

} // namespace

TEST_CASE("an image file in memory is decoded to its pixels")
{
    const auto image = decode_image(std::as_bytes(std::span{bitmap}), ".bmp");

    REQUIRE(image.has_value());
    CHECK(image->width == 2);
    CHECK(image->height == 1);
    REQUIRE(image->pixels.size() == 2);
    CHECK(image->pixels[0] == Color::red);
    CHECK(image->pixels[1] == Color::green);
}

TEST_CASE("data that is not an image is rejected")
{
    constexpr auto garbage = std::to_array<unsigned char>({1, 2, 3, 4, 5, 6, 7, 8});

    SECTION("corrupt data")
    {
        const auto image = decode_image(std::as_bytes(std::span{garbage}), ".png");
        REQUIRE_FALSE(image.has_value());
        CHECK(image.error() == PlatformError::InvalidData);
    }

    SECTION("an unknown format")
    {
        const auto image = decode_image(std::as_bytes(std::span{bitmap}), ".xyz");
        REQUIRE_FALSE(image.has_value());
        CHECK(image.error() == PlatformError::InvalidData);
    }

    SECTION("no data")
    {
        const auto image = decode_image({}, ".png");
        REQUIRE_FALSE(image.has_value());
        CHECK(image.error() == PlatformError::InvalidData);
    }
}
