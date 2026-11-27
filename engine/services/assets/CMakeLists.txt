add_library(ember_assets STATIC)
add_library(ember::assets ALIAS ember_assets)

target_sources(ember_assets
    PUBLIC
        FILE_SET CXX_MODULES FILES
            assets.cppm
            asset_catalog.cppm
            asset_manager.cppm
)

# Public: the interface of the asset manager uses the platform services.
target_link_libraries(ember_assets PUBLIC ember::platform)

ember_enable_warnings(ember_assets)
