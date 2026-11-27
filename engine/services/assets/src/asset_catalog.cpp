module ember.assets;

import std;

namespace ember::assets
{

void AssetCatalog::add(std::string_view name, std::span<const std::byte> data)
{
    // The map keeps its own copy of the name, because the caller's text may not live long
    // enough. The bytes are not copied; the span only refers to them.
    m_files.insert_or_assign(std::string{name}, data);
}

std::optional<std::span<const std::byte>> AssetCatalog::find(std::string_view name) const
{
    const auto found = m_files.find(name);
    if (found == m_files.end())
    {
        return std::nullopt;
    }
    return found->second;
}

bool AssetCatalog::contains(std::string_view name) const
{
    return m_files.contains(name);
}

std::size_t AssetCatalog::size() const
{
    return m_files.size();
}

} // namespace ember::assets
