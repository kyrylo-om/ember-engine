#include <catch_amalgamated.hpp>

import ember.common;
import ember.platform;
import std;

using ember::Handle;
using ember::Music;
using ember::Sound;
using ember::platform::Audio;
using ember::platform::PlatformError;

namespace
{

void append_bytes(std::vector<std::byte>& bytes, std::string_view text)
{
    bytes.append_range(std::as_bytes(std::span{text}));
}

// Appends an integer with the lowest byte first, as WAV files store numbers.
template <std::unsigned_integral T>
void append_number(std::vector<std::byte>& bytes, T value)
{
    for (std::size_t byte = 0; byte < sizeof(T); ++byte)
    {
        bytes.push_back(static_cast<std::byte>((value >> (8 * byte)) & 0xFF));
    }
}

// A WAV file with a tenth of a second of silence: one channel, 16 bits, 8000 Hz.
std::vector<std::byte> silent_wav()
{
    constexpr std::uint32_t sample_rate = 8000;
    constexpr std::uint16_t channel_count = 1;
    constexpr std::uint16_t bytes_per_sample = 2;
    constexpr std::uint32_t data_size = sample_rate / 10 * bytes_per_sample;
    constexpr std::uint32_t format_chunk_size = 16;
    constexpr std::uint16_t pcm_format = 1;
    // Everything after the size field itself: "WAVE", two chunk headers and their contents.
    constexpr std::uint32_t riff_size = 4 + 8 + format_chunk_size + 8 + data_size;

    std::vector<std::byte> bytes;
    append_bytes(bytes, "RIFF");
    append_number(bytes, riff_size);
    append_bytes(bytes, "WAVE");
    append_bytes(bytes, "fmt ");
    append_number(bytes, format_chunk_size);
    append_number(bytes, pcm_format);
    append_number(bytes, channel_count);
    append_number(bytes, sample_rate);
    append_number(bytes, std::uint32_t{sample_rate * channel_count * bytes_per_sample});
    append_number(bytes, static_cast<std::uint16_t>(channel_count * bytes_per_sample));
    append_number(bytes, static_cast<std::uint16_t>(bytes_per_sample * 8));
    append_bytes(bytes, "data");
    append_number(bytes, data_size);
    bytes.resize(bytes.size() + data_size);
    return bytes;
}

} // namespace

// Needs an audio device, which is opened once for all checks. On a machine without one the
// test is skipped. Only silence is played.
TEST_CASE("sounds and music are loaded, found by handles and played")
{
    auto audio = Audio::open();
    if (!audio.has_value())
    {
        SKIP("no audio device can be opened on this machine");
    }
    const std::vector<std::byte> wav = silent_wav();
    const std::vector<std::byte> garbage(64, std::byte{7});

    // Only one audio service can be open at a time.
    const auto second = Audio::open();
    REQUIRE_FALSE(second.has_value());
    CHECK(second.error() == PlatformError::AudioUnavailable);

    // A sound is loaded and played; a stale or null handle does nothing.
    const auto sound = audio->load_sound(wav, ".wav");
    REQUIRE(sound.has_value());
    CHECK_NOTHROW(audio->play_sound(*sound, 0.0f));
    CHECK_NOTHROW(audio->stop_sound(*sound));
    audio->unload_sound(*sound);
    CHECK_NOTHROW(audio->play_sound(*sound));
    CHECK_NOTHROW(audio->play_sound(Handle<Sound>{}));
    const auto reloaded_sound = audio->load_sound(wav, ".wav");
    REQUIRE(reloaded_sound.has_value());
    CHECK(*reloaded_sound != *sound);

    // Data that is not a sound is rejected.
    const auto broken_sound = audio->load_sound(garbage, ".wav");
    REQUIRE_FALSE(broken_sound.has_value());
    CHECK(broken_sound.error() == PlatformError::InvalidData);
    CHECK_FALSE(audio->load_sound({}, ".wav").has_value());
    CHECK_FALSE(audio->load_sound(wav, ".xyz").has_value());
    CHECK_FALSE(audio->load_music(garbage, ".ogg").has_value());
    CHECK_FALSE(audio->load_music({}, ".wav").has_value());

    // Music plays until it is paused or stopped.
    const auto music = audio->load_music(wav, ".wav");
    REQUIRE(music.has_value());
    CHECK_FALSE(audio->is_music_playing(*music));
    audio->set_music_volume(*music, 0.0f);
    audio->play_music(*music);
    audio->update();
    CHECK(audio->is_music_playing(*music));
    audio->pause_music(*music);
    CHECK_FALSE(audio->is_music_playing(*music));
    audio->resume_music(*music);
    CHECK(audio->is_music_playing(*music));
    audio->stop_music(*music);
    CHECK_FALSE(audio->is_music_playing(*music));

    // Unloaded music is not playing, and its handle does nothing.
    audio->play_music(*music);
    audio->unload_music(*music);
    CHECK_FALSE(audio->is_music_playing(*music));
    CHECK_NOTHROW(audio->play_music(*music));
    CHECK_FALSE(audio->is_music_playing(Handle<Music>{}));
    CHECK_NOTHROW(audio->update());
    CHECK_NOTHROW(audio->set_master_volume(0.5f));

    // A moved audio service keeps the device and the handles; the device is closed with it.
    {
        Audio moved = std::move(*audio);
        CHECK_NOTHROW(moved.play_sound(*reloaded_sound, 0.0f));
        CHECK_FALSE(Audio::open().has_value());
    }
    CHECK(Audio::open().has_value());
}
