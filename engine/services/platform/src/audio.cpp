module ember.platform;

import :resource_table;
import ember.common;
import raylib;
import std;

namespace ember::platform
{

namespace
{

constexpr float silent = 0.0f;
constexpr float full_volume = 1.0f;

float clamp_volume(float volume)
{
    return std::clamp(volume, silent, full_volume);
}

} // namespace

struct Audio::Resources
{
    // Declared first, so it is destroyed last: the device is closed after the sounds and
    // music that play on it are freed. The device is opened by Audio::open() before this
    // object is created; the constructor argument only stops Raylib from opening it again.
    raylib::AudioDevice device{true};
    ResourceTable<Sound, raylib::Sound> sounds{};
    ResourceTable<Music, raylib::Music> music{};
};

std::expected<Audio, PlatformError> Audio::open()
{
    if (raylib::AudioDevice::IsReady())
    {
        return std::unexpected{PlatformError::AudioUnavailable};
    }

    try
    {
        raylib::AudioDevice::Init();
    }
    catch (const raylib::RaylibException&)
    {
        return std::unexpected{PlatformError::AudioUnavailable};
    }
    return Audio{};
}

Audio::Audio() : m_resources{std::make_unique<Resources>()}
{
}

Audio::~Audio() = default;
Audio::Audio(Audio&& other) noexcept = default;
Audio& Audio::operator=(Audio&& other) noexcept = default;

std::expected<Handle<Sound>, PlatformError> Audio::load_sound(std::span<const std::byte> data,
                                                              std::string_view format)
{
    if (data.empty() || !std::in_range<int>(data.size()))
    {
        return std::unexpected{PlatformError::InvalidData};
    }

    try
    {
        const raylib::Wave wave{std::string{format},
                                reinterpret_cast<const unsigned char*>(data.data()),
                                static_cast<int>(data.size())};
        return m_resources->sounds.add(raylib::Sound{wave});
    }
    catch (const raylib::RaylibException&)
    {
        return std::unexpected{PlatformError::InvalidData};
    }
}

void Audio::unload_sound(Handle<Sound> sound)
{
    m_resources->sounds.remove(sound);
}

void Audio::play_sound(Handle<Sound> sound, float volume)
{
    raylib::Sound* found = m_resources->sounds.find(sound);
    if (found != nullptr)
    {
        found->SetVolume(clamp_volume(volume));
        found->Play();
    }
}

void Audio::stop_sound(Handle<Sound> sound)
{
    raylib::Sound* found = m_resources->sounds.find(sound);
    if (found != nullptr)
    {
        found->Stop();
    }
}

std::expected<Handle<Music>, PlatformError> Audio::load_music(std::span<const std::byte> data,
                                                              std::string_view format)
{
    if (data.empty() || !std::in_range<int>(data.size()))
    {
        return std::unexpected{PlatformError::InvalidData};
    }

    try
    {
        // Raylib only reads the data while it decodes, but takes a pointer without const.
        auto* bytes =
            const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(data.data()));
        return m_resources->music.add(
            raylib::Music{std::string{format}, bytes, static_cast<int>(data.size())});
    }
    catch (const raylib::RaylibException&)
    {
        return std::unexpected{PlatformError::InvalidData};
    }
}

void Audio::unload_music(Handle<Music> music)
{
    m_resources->music.remove(music);
}

void Audio::play_music(Handle<Music> music, bool is_looping)
{
    raylib::Music* found = m_resources->music.find(music);
    if (found != nullptr)
    {
        // Stopping rewinds the music, so playing always starts from the beginning.
        found->Stop();
        found->looping = is_looping;
        found->Play();
    }
}

void Audio::pause_music(Handle<Music> music)
{
    raylib::Music* found = m_resources->music.find(music);
    if (found != nullptr)
    {
        found->Pause();
    }
}

void Audio::resume_music(Handle<Music> music)
{
    raylib::Music* found = m_resources->music.find(music);
    if (found != nullptr)
    {
        found->Resume();
    }
}

void Audio::stop_music(Handle<Music> music)
{
    raylib::Music* found = m_resources->music.find(music);
    if (found != nullptr)
    {
        found->Stop();
    }
}

void Audio::set_music_volume(Handle<Music> music, float volume)
{
    raylib::Music* found = m_resources->music.find(music);
    if (found != nullptr)
    {
        found->SetVolume(clamp_volume(volume));
    }
}

bool Audio::is_music_playing(Handle<Music> music) const
{
    const raylib::Music* found = m_resources->music.find(music);
    return found != nullptr && found->IsPlaying();
}

void Audio::set_master_volume(float volume)
{
    m_resources->device.SetVolume(clamp_volume(volume));
}

void Audio::update()
{
    m_resources->music.for_each([](raylib::Music& music) { music.Update(); });
}

} // namespace ember::platform
