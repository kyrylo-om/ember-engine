export module ember.platform:audio;

import :platform_error;
import ember.common;
import std;

export namespace ember::platform
{

/// Sound effects and music.
///
/// A sound is decoded into memory as a whole when it is loaded, so it starts playing at
/// once; use it for short effects. Music is decoded bit by bit while it plays, so it uses
/// little memory; use it for long tracks. Owns all sounds and music and frees them when it
/// is destroyed. A handle to a freed resource is stale, and calls with it do nothing.
///
/// Volumes go from 0 (silent) to 1 (unchanged).
/// @note Must be used only from the main thread.
class Audio
{
public:
    /// Opens the default audio device.
    /// @return The audio service, or `PlatformError::AudioUnavailable` if no device can be
    /// opened or another `Audio` is already open.
    [[nodiscard]] static std::expected<Audio, PlatformError> open();

    /// Frees all sounds and music and closes the audio device.
    ~Audio();

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    /// Takes over the device, sounds and music; handles to them stay valid.
    Audio(Audio&& other) noexcept;

    /// Closes the device of this object, then takes over the device of `other`.
    Audio& operator=(Audio&& other) noexcept;

    /// Loads a sound effect from a sound file in memory.
    /// @param data Contents of the file; it is decoded at once and not used afterwards.
    /// @param format File extension with the dot: ".wav", ".ogg", ".mp3" or ".qoa".
    /// @return Handle to the sound, or `PlatformError::InvalidData` if the data cannot be
    /// decoded.
    [[nodiscard]] std::expected<Handle<Sound>, PlatformError>
    load_sound(std::span<const std::byte> data, std::string_view format);

    /// Frees a sound and stops it if it is playing. Its handles become stale.
    /// @param sound Sound to free. Nothing happens for a null or stale handle.
    void unload_sound(Handle<Sound> sound);

    /// Plays a sound from the start, or restarts it if it is already playing.
    /// @param sound Sound to play.
    /// @param volume Volume of this sound.
    void play_sound(Handle<Sound> sound, float volume = 1.0f);

    /// Stops a sound.
    /// @param sound Sound to stop.
    void stop_sound(Handle<Sound> sound);

    /// Loads music from a sound file in memory.
    /// @param data Contents of the file.
    /// @param format File extension with the dot: ".wav", ".ogg", ".mp3", ".qoa", ".xm" or
    /// ".mod".
    /// @return Handle to the music, or `PlatformError::InvalidData` if the data cannot be
    /// decoded.
    /// @note The music is decoded from `data` while it plays, so the data must stay alive
    /// and unchanged until the music is unloaded. Embedded assets live for the whole
    /// program.
    [[nodiscard]] std::expected<Handle<Music>, PlatformError>
    load_music(std::span<const std::byte> data, std::string_view format);

    /// Frees music and stops it if it is playing. Its handles become stale.
    /// @param music Music to free. Nothing happens for a null or stale handle.
    void unload_music(Handle<Music> music);

    /// Plays music from the start.
    /// @param music Music to play.
    /// @param is_looping Whether the music starts again when it ends.
    void play_music(Handle<Music> music, bool is_looping = true);

    /// Pauses music at its current position.
    /// @param music Music to pause.
    void pause_music(Handle<Music> music);

    /// Continues paused music from where it was paused.
    /// @param music Music to continue.
    void resume_music(Handle<Music> music);

    /// Stops music; it starts from the beginning when it is played again.
    /// @param music Music to stop.
    void stop_music(Handle<Music> music);

    /// Sets the volume of music.
    /// @param music Music whose volume is set.
    /// @param volume New volume.
    void set_music_volume(Handle<Music> music, float volume);

    /// Returns true while music is playing; false when it is paused, stopped or ended.
    bool is_music_playing(Handle<Music> music) const;

    /// Sets the volume of all sounds and music together.
    void set_master_volume(float volume);

    /// Decodes the next part of all playing music. The platform plugin calls it every frame
    /// in `Stage::PreUpdate`; music that is not updated stutters.
    void update();

private:
    // Created only by open().
    Audio();

    // The tables hold Raylib types, which must not appear in the interface, so they are
    // defined in the implementation. Null after a move, which means this object no longer
    // owns the device.
    struct Resources;
    std::unique_ptr<Resources> m_resources;
};

} // namespace ember::platform
