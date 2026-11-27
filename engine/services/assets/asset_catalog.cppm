export module ember.assets:asset_catalog;

import std;

export namespace ember::assets
{

/// Names of the files embedded into the executable and their contents.
///
/// The game embeds every file with `#embed` and registers it under its file name. The
/// extension of the name tells the asset manager the format of the file:
///
/// ```
/// constexpr unsigned char girl_png[] = {
/// #embed "assets/girl.png"
/// };
/// catalog.add("girl.png", std::as_bytes(std::span{girl_png}));
/// ```
/// @note Stores views of the bytes, not copies, so the bytes must live as long as the catalog.
/// Arrays filled by `#embed` at namespace scope live for the whole program.
class AssetCatalog
{
public:
    /// Registers a file, replacing the file registered under the same name.
    /// @param name Name of the file with its extension, for example "girl.png".
    /// @param data Contents of the file.
    void add(std::string_view name, std::span<const std::byte> data);

    /// Returns the contents of a file.
    /// @param name Name under which the file is registered.
    /// @return The contents, or nothing if no file is registered under this name.
    [[nodiscard]] std::optional<std::span<const std::byte>> find(std::string_view name) const;

    /// Returns true if a file is registered under the name.
    bool contains(std::string_view name) const;

    /// Returns the number of registered files.
    std::size_t size() const;

private:
    // std::less<> lets the map look up a std::string_view without creating a std::string.
    std::map<std::string, std::span<const std::byte>, std::less<>> m_files{};
};

} // namespace ember::assets
