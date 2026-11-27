module ember.assets;

import ember.core;
import ember.ecs;
import ember.platform;

namespace ember::assets
{

void AssetsPlugin::build(core::Application& app) const
{
    ecs::World& world = app.world();
    // The manager keeps the address of the catalog stored in the world, which does not move.
    const AssetCatalog& catalog = world.insert_resource(AssetCatalog{});
    world.insert_resource(AssetManager{catalog, world.try_resource<platform::Graphics>(),
                                       world.try_resource<platform::Audio>()});
}

} // namespace ember::assets
