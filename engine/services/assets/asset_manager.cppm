export module ember.assets:asset_manager;

import :asset_catalog;
import ember.common;
import ember.platform;
import std;

export namespace ember::assets
{

/// Reasons why an asset cannot be loaded.
enum class AssetError
{
    /// No file with this name is registered in the catalog.
    NotFound,
    /// The file is corrupt or its format is not supported.
    InvalidData,
    /// A texture or a font is asked for, but there is no `platform::Graphics`.
    GraphicsUnavailable,
    /// A sound or music is asked for, but there is no `platform::Audio`.
    AudioUnavailable
};

/// Loads the files of the catalog as textures, sounds, music and fonts, and caches them by
/// name.
///
/// The format of a file is taken from the extension of its name. Loading a name again returns
/// the cached handle without decoding the file again. The `platform::Graphics` and
/// `platform::Audio` services create and own the resources; the manager only remembers which
/// names it has loaded.
/// @note Uses the catalog and the services it is given, so it must be destroyed before them.
/// The assets plugin ensures this. Must be used only from the main thread, like
/// `platform::Graphics`.
class AssetManager
{
public:
    /// Creates a manager that has loaded nothing.
    /// @param catalog Catalog with the files.
    /// @param graphics Service that creates textures and fonts, or nullptr if there is none.
    /// @param audio Service that creates sounds and music, or nullptr if there is none.
    AssetManager(const AssetCatalog& catalog, platform::Graphics* graphics, platform::Audio* audio);
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) noexcept;
    AssetManager& operator=(AssetManager&&) noexcept;

    /// Loads a texture, or returns the cached one if it was loaded before.
    /// @param name Name of an image file in the catalog: ".png", ".bmp", ".qoi" or ".gif".
    /// @return Handle to the texture, or the reason why it cannot be loaded.
    [[nodiscard]] std::expected<Handle<Texture>, AssetError> load_texture(std::string_view name);

    /// Loads a sound effect, or returns the cached one if it was loaded before.
    /// @param name Name of a sound file in the catalog: ".wav", ".ogg", ".mp3" or ".qoa".
    /// @return Handle to the sound, or the reason why it cannot be loaded.
    [[nodiscard]] std::expected<Handle<Sound>, AssetError> load_sound(std::string_view name);

    /// Loads music, or returns the cached one if it was loaded before.
    /// @param name Name of a music file in the catalog: ".wav", ".ogg", ".mp3", ".qoa", ".xm"
    /// or ".mod".
    /// @return Handle to the music, or the reason why it cannot be loaded.
    [[nodiscard]] std::expected<Handle<Music>, AssetError> load_music(std::string_view name);

    /// Loads a font in a size, or returns the cached one if it was loaded in this size before.
    /// @param name Name of a font file in the catalog: ".ttf" or ".otf".
    /// @param size Height of the glyphs in pixels; every size is a separate font.
    /// @return Handle to the font, or the reason why it cannot be loaded.
    [[nodiscard]] std::expected<Handle<Font>, AssetError> load_font(std::string_view name,
                                                                    int size);

    /// Frees everything loaded under a name, a font in all its sizes. Its handles become
    /// stale, and loading the name again decodes the file again.
    /// @param name Name of the file. Nothing happens if nothing is loaded under it.
    void unload(std::string_view name);

    /// Frees everything the manager has loaded. Their handles become stale.
    void clear();

    /// Returns the number of loaded resources; a font counts once for every size.
    std::size_t loaded_count() const;

private:
    const AssetCatalog* m_catalog = nullptr;
    platform::Graphics* m_graphics = nullptr;
    platform::Audio* m_audio = nullptr;
    // std::less<> lets the maps look up a std::string_view without creating a std::string.
    std::map<std::string, Handle<Texture>, std::less<>> m_textures{};
    std::map<std::string, Handle<Sound>, std::less<>> m_sounds{};
    std::map<std::string, Handle<Music>, std::less<>> m_music{};
    // A font is loaded separately for every size.
    std::map<std::pair<std::string, int>, Handle<Font>> m_fonts{};
};

} // namespace ember::assets
