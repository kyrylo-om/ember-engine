module ember.assets;

import ember.common;
import ember.platform;
import std;

namespace ember::assets
{

namespace
{

// Returns the extension of a file name with its dot, as the platform services expect the
// format, or an empty view if the name has no dot.
std::string_view extension_of(std::string_view name)
{
    const std::size_t dot = name.rfind('.');
    return dot == std::string_view::npos ? std::string_view{} : name.substr(dot);
}

AssetError to_asset_error(platform::PlatformError error)
{
    switch (error)
    {
        case platform::PlatformError::WindowUnavailable:
            return AssetError::GraphicsUnavailable;
        case platform::PlatformError::AudioUnavailable:
            return AssetError::AudioUnavailable;
        case platform::PlatformError::InvalidData:
            return AssetError::InvalidData;
    }
    std::unreachable();
}

// Textures, sounds and music are loaded the same way and differ only in their cache and in
// the function of the platform service that creates them. A missing file is reported before a
// missing service, so a wrong name is found even without a window.
template <typename T, typename Create>
std::expected<Handle<T>, AssetError>
load_cached(std::map<std::string, Handle<T>, std::less<>>& cache, const AssetCatalog& catalog,
            std::string_view name, bool is_service_available, AssetError unavailable, Create create)
{
    if (const auto cached = cache.find(name); cached != cache.end())
    {
        return cached->second;
    }
    const auto data = catalog.find(name);
    if (!data.has_value())
    {
        return std::unexpected{AssetError::NotFound};
    }
    if (!is_service_available)
    {
        return std::unexpected{unavailable};
    }
    const std::expected<Handle<T>, platform::PlatformError> created =
        create(*data, extension_of(name));
    if (!created.has_value())
    {
        return std::unexpected{to_asset_error(created.error())};
    }
    cache.emplace(std::string{name}, *created);
    return *created;
}

} // namespace

AssetManager::AssetManager(const AssetCatalog& catalog, platform::Graphics* graphics,
                           platform::Audio* audio)
    : m_catalog{&catalog}, m_graphics{graphics}, m_audio{audio}
{
}

// The platform services free all their resources when they are destroyed, right after the
// manager, so the manager does not free them again.
AssetManager::~AssetManager() = default;
AssetManager::AssetManager(AssetManager&&) noexcept = default;
AssetManager& AssetManager::operator=(AssetManager&&) noexcept = default;

std::expected<Handle<Texture>, AssetError> AssetManager::load_texture(std::string_view name)
{
    return load_cached(m_textures, *m_catalog, name, m_graphics != nullptr,
                       AssetError::GraphicsUnavailable,
                       [this](std::span<const std::byte> data, std::string_view format)
                       { return m_graphics->load_texture(data, format); });
}

std::expected<Handle<Sound>, AssetError> AssetManager::load_sound(std::string_view name)
{
    return load_cached(m_sounds, *m_catalog, name, m_audio != nullptr, AssetError::AudioUnavailable,
                       [this](std::span<const std::byte> data, std::string_view format)
                       { return m_audio->load_sound(data, format); });
}

std::expected<Handle<Music>, AssetError> AssetManager::load_music(std::string_view name)
{
    return load_cached(m_music, *m_catalog, name, m_audio != nullptr, AssetError::AudioUnavailable,
                       [this](std::span<const std::byte> data, std::string_view format)
                       { return m_audio->load_music(data, format); });
}

std::expected<Handle<Font>, AssetError> AssetManager::load_font(std::string_view name, int size)
{
    // The same steps as load_cached, but a font is cached under its name and size.
    const std::pair<std::string, int> key{name, size};
    if (const auto cached = m_fonts.find(key); cached != m_fonts.end())
    {
        return cached->second;
    }
    const auto data = m_catalog->find(name);
    if (!data.has_value())
    {
        return std::unexpected{AssetError::NotFound};
    }
    if (m_graphics == nullptr)
    {
        return std::unexpected{AssetError::GraphicsUnavailable};
    }
    const auto created = m_graphics->load_font(*data, extension_of(name), size);
    if (!created.has_value())
    {
        return std::unexpected{to_asset_error(created.error())};
    }
    m_fonts.emplace(key, *created);
    return *created;
}

// Something is cached only if its service existed when it was loaded, and the services never
// change, so the services of cached resources are not null.
void AssetManager::unload(std::string_view name)
{
    if (const auto found = m_textures.find(name); found != m_textures.end())
    {
        m_graphics->unload_texture(found->second);
        m_textures.erase(found);
    }
    if (const auto found = m_sounds.find(name); found != m_sounds.end())
    {
        m_audio->unload_sound(found->second);
        m_sounds.erase(found);
    }
    if (const auto found = m_music.find(name); found != m_music.end())
    {
        m_audio->unload_music(found->second);
        m_music.erase(found);
    }
    std::erase_if(m_fonts,
                  [this, name](const auto& entry)
                  {
                      if (entry.first.first != name)
                      {
                          return false;
                      }
                      m_graphics->unload_font(entry.second);
                      return true;
                  });
}

void AssetManager::clear()
{
    for (const Handle<Texture> texture : m_textures | std::views::values)
    {
        m_graphics->unload_texture(texture);
    }
    for (const Handle<Sound> sound : m_sounds | std::views::values)
    {
        m_audio->unload_sound(sound);
    }
    for (const Handle<Music> music : m_music | std::views::values)
    {
        m_audio->unload_music(music);
    }
    for (const Handle<Font> font : m_fonts | std::views::values)
    {
        m_graphics->unload_font(font);
    }
    m_textures.clear();
    m_sounds.clear();
    m_music.clear();
    m_fonts.clear();
}

std::size_t AssetManager::loaded_count() const
{
    return m_textures.size() + m_sounds.size() + m_music.size() + m_fonts.size();
}

} // namespace ember::assets
