// Files embedded into the executable: the catalog that names them, the manager that loads them
// as textures, sounds, music and fonts, and the plugin that adds both to the world.
export module ember.assets;

export import :asset_catalog;
export import :asset_manager;

import ember.core;

export namespace ember::assets
{

/// Inserts an empty `AssetCatalog` and an `AssetManager` that loads from it.
///
/// The game fills the catalog, usually in `core::Stage::Startup`, and loads its assets through
/// the manager. The manager uses the `platform::Graphics` and `platform::Audio` that exist
/// when the plugin is built, so add this plugin after the platform plugin. Without them,
/// loading returns `AssetError::GraphicsUnavailable` or `AssetError::AudioUnavailable`.
/// @note The world destroys resources in reverse order of insertion, so with this order the
/// manager is destroyed before the catalog and the platform services it uses.
struct AssetsPlugin
{
    /// Inserts the catalog and the manager.
    /// @param app Application to add the resources to.
    void build(core::Application& app) const;
};

} // namespace ember::assets
