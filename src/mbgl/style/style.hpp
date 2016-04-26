#pragma once

#include <mbgl/style/observer.hpp>
#include <mbgl/style/transition_options.hpp>

#include <mbgl/style/source.hpp>
#include <mbgl/text/glyph_store.hpp>
#include <mbgl/sprite/sprite_store.hpp>
#include <mbgl/map/mode.hpp>
#include <mbgl/map/zoom_history.hpp>

#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/worker.hpp>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/feature.hpp>
#include <mbgl/util/color.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace mbgl {

class FileSource;
class GlyphAtlas;
class SpriteAtlas;
class LineAtlas;
class Tile;
class Bucket;
class TileCoordinate;

namespace style {
class Layer;
}

struct RenderItem {
    inline RenderItem(const style::Layer& layer_,
                      const Tile* tile_ = nullptr,
                      Bucket* bucket_ = nullptr)
        : tile(tile_), bucket(bucket_), layer(layer_) {
    }

    const Tile* const tile;
    Bucket* const bucket;
    const style::Layer& layer;
};

struct RenderData {
    Color backgroundColor = {{ 0, 0, 0, 0 }};
    std::set<style::Source*> sources;
    std::vector<RenderItem> order;
};

namespace style {

class UpdateParameters;

class Style : public GlyphStore::Observer,
              public SpriteStore::Observer,
              public Source::Observer,
              public util::noncopyable {
public:
    Style(FileSource&, float pixelRatio);
    ~Style();

    void setJSON(const std::string& data, const std::string& base);

    void setObserver(style::Observer*);

    bool isLoaded() const;

    // Fetch the tiles needed by the current viewport and emit a signal when
    // a tile is ready so observers can render the tile.
    void update(const UpdateParameters&);

    void cascade(const TimePoint&, MapMode);
    void recalculate(float z, const TimePoint&, MapMode);

    bool hasTransitions() const;

    std::exception_ptr getLastError() const {
        return lastError;
    }

    Source* getSource(const std::string& id) const;
    void addSource(std::unique_ptr<Source>);

    std::vector<std::unique_ptr<Layer>> getLayers() const;
    Layer* getLayer(const std::string& id) const;
    void addLayer(std::unique_ptr<Layer>,
                  optional<std::string> beforeLayerID = {});
    void removeLayer(const std::string& layerID);

    bool addClass(const std::string&, const TransitionOptions& = {});
    bool removeClass(const std::string&, const TransitionOptions& = {});
    bool hasClass(const std::string&) const;
    void setClasses(const std::vector<std::string>&, const TransitionOptions& = {});
    std::vector<std::string> getClasses() const;

    RenderData getRenderData() const;

    std::vector<Feature> queryRenderedFeatures(
            const std::vector<TileCoordinate>& queryGeometry,
            const double zoom,
            const double bearing,
            const optional<std::vector<std::string>>& layerIDs);

    float getQueryRadius() const;

    void setSourceTileCacheSize(size_t);
    void onLowMemory();

    void dumpDebugLogs() const;

    FileSource& fileSource;
    std::unique_ptr<GlyphStore> glyphStore;
    std::unique_ptr<GlyphAtlas> glyphAtlas;
    std::unique_ptr<SpriteStore> spriteStore;
    std::unique_ptr<SpriteAtlas> spriteAtlas;
    std::unique_ptr<LineAtlas> lineAtlas;

private:
    std::vector<std::unique_ptr<Source>> sources;
    std::vector<std::unique_ptr<Layer>> layers;
    std::vector<std::string> classes;
    optional<TransitionOptions> transitionProperties;

    std::vector<std::unique_ptr<Layer>>::const_iterator findLayer(const std::string& layerID) const;

    // GlyphStore::Observer implementation.
    void onGlyphsLoaded(const FontStack&, const GlyphRange&) override;
    void onGlyphsError(const FontStack&, const GlyphRange&, std::exception_ptr) override;

    // SpriteStore::Observer implementation.
    void onSpriteLoaded() override;
    void onSpriteError(std::exception_ptr) override;

    // Source::Observer implementation.
    void onSourceLoaded(Source&) override;
    void onSourceError(Source&, std::exception_ptr) override;
    void onTileLoaded(Source&, const TileID&, bool isNewTile) override;
    void onTileError(Source&, const TileID&, std::exception_ptr) override;
    void onPlacementRedone() override;

    style::Observer nullObserver;
    style::Observer* observer = &nullObserver;

    std::exception_ptr lastError;

    ZoomHistory zoomHistory;
    bool hasPendingTransitions = false;

public:
    bool shouldReparsePartialTiles = false;
    bool loaded = false;
    Worker workers;
};

} // namespace style
} // namespace mbgl
