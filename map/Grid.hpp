#pragma once
#include <cmath>

#include "../Types.hpp"

namespace nexus::map {
    // ─── Tile ─────────────────────────────────────────────────────────────────
    struct Tile {
        mutable TerrainType terrain{TerrainType::Open};
        mutable float traversalCost{1.0f};
        mutable bool blocksMovement{false};
        mutable bool blocksVision{false};
        mutable bool onFire{false};
        mutable float fireDuration{0.f};
        mutable int elevation{0}; // 0=flat, +1=ramp up, -1=ramp down
        EntityID construct{NULL_Entity}; // paper entity on this tile

        [[nodiscard]] auto isPassable() const noexcept -> bool {
            return !blocksMovement;
        }
    };

    // ─── Fog of war per faction ───────────────────────────────────────────────
    struct FogLayer {
        enum class State : uint8_t { Hidden=0, Memory=1, Visible=2 };
        std::vector<State> cells;
        int width{0};
        int height{0};

        void init(const int w, const int h) {
            width = w;
            height = h;
            cells.assign(w*h, State::Hidden);
        }

        [[nodiscard]] auto get(const int x, const int y) const noexcept -> State {
            if (x < 0 || x >= width || y < 0 || y >= height) {
                return State::Hidden;
            }

            return cells[(y*width) + x];
        }

        void set(const int x, const int y, const State s) noexcept {
            if (x < 0 || x >= width || y < 0 || y >= height) {
                return;
            }
            cells[(y*width) + x] = s;
        }

        // Set all visible cells to memory
        void stepFrame() {
            for (auto& c : cells) {
                if (c == State::Visible) {
                    c = State::Memory;
                }
            }
        }



    };

    class Grid {
    public:
        Grid() = default; // automatic default constructor
        Grid(const int w, const int h) : width_(w), height_(h), tiles_(w*h) {//save width/height/tiles into grid's personal memory
            playerFog_.init(w, h);
            enemyFog_.init(w, h);

        }

        [[nodiscard]] auto width()  const noexcept -> int {
            return width_;
        }
        [[nodiscard]] auto height() const noexcept -> int {
            return height_;
        }

        [[nodiscard]] auto inBounds(const int x, const int y) const noexcept -> bool {
           return x >= 0 && x < width_ && y >= 0 && y < height_;
        }

        [[nodiscard]] auto inBounds(const GridPos& p) const noexcept -> bool {
            return inBounds(p.x, p.y);
        }

        auto at(const int x, const int y) -> Tile& {
            return tiles_[(y*width_)+x];
        }

        [[nodiscard]] auto at(int x, int y) const -> const Tile& {
            return tiles_[(y*width_)+x];
        }
        [[nodiscard]] auto at(const GridPos& p) const -> const Tile& {
            return at(p.x, p.y);
        }

        //terrain mutation
        void setTerrain(const GridPos& p, const TerrainType t) {
            if (!inBounds(p)) {
                return;
            }
            auto& tile = at(p);
            tile.terrain = t;
            switch (t) {
                case TerrainType::Open:
                case TerrainType::PaperWall:
                    tile.blocksMovement = true;
                    tile.blocksVision = true;
                    tile.traversalCost = 999.F;
                    break;

                case TerrainType::Water:
                    tile.traversalCost = 4.F;
                    break;

                case TerrainType::PaperBridge:
                    tile.blocksMovement = false;
                    tile.blocksVision = false;
                    tile.traversalCost = 1.5F;
                    break;

                case TerrainType::BurnedAsh:
                    tile.blocksMovement = false;
                    tile.traversalCost = 1.2F;
                    break;

                default:
                    tile.blocksMovement = false;
                    tile.blocksVision = false;
                    tile.traversalCost = 1.0F;
            }
            dirtyPathCache_ = true;

        }

        void ignite(const GridPos& p, const float duration = 5.F) const {
            if (!inBounds(p)) {
                return;
            }

            auto& t = at(p);
            t.onFire = true;
            t.fireDuration = duration;
        }

        void updateFire(float dt) {
            for (int y=0; y < height_; ++y) {
                for (int x=0; x < width_; ++x) {
                    auto& t = tiles_[(y*width_) + x];

                    if (!t.onFire) {
                        continue;
                    }

                    t.fireDuration -= dt;

                    if (t.fireDuration <= 0.F) {
                        t.onFire = false;

                        // paper walls to burn ash
                        if (t.terrain == TerrainType::PaperWall) {
                            setTerrain({.x=x, .y=y}, TerrainType::BurnedAsh);
                        }
                    }

                }
            }
        }

        // Fog of war
        auto playerFog() noexcept -> FogLayer& {
            return playerFog_;
        }
        auto enemyFog()  noexcept -> FogLayer& {
            return enemyFog_;
        }
        [[nodiscard]] auto playerFog() const noexcept -> const FogLayer& {
            return playerFog_;
        }

        [[nodiscard]] auto isVisibleToPlayer(const GridPos& p) const noexcept -> bool {
            return playerFog_.get(p.x, p.y) == FogLayer::State::Visible;
        }

        [[nodiscard]] auto isInPlayerMemory(const GridPos& p) const noexcept -> bool {
            return playerFog_.get(p.x,p.y) != FogLayer::State::Hidden;
        }

        void revealForm(const GridPos& center, const int radius, FogLayer& fog) {
            fog.stepFrame();
            fog.set(center.x, center.y, FogLayer::State::Visible);

            // cast rays in 360 degrees
            // evaluated at compile time
            constexpr int RAYS = 360;
            constexpr float pi = std::numbers::pi_v<__float128>;
            for (int i = 0; i < RAYS; ++i) {
                const float angle = static_cast<float>(i) * (pi * 2.F / RAYS);
                const float dx = std::cos(angle);
                const float dy = std::sin(angle);
                float rx = static_cast<float>(center.x) + 0.5F;
                float ry = static_cast<float>(center.y) + 0.5F;

                for (int step=0; step<=radius; ++step) {
                    const int tx = static_cast<int>(rx);
                    const int ty = static_cast<int>(ry);

                    if (!inBounds(tx,ty)) {
                        break;
                    }

                    fog.set(tx, ty, FogLayer::State::Visible);

                    if (at(tx, ty).blocksVision) {
                        break;
                    }

                    rx += dx;
                    ry += dy;
                }
            }
        }

        [[nodiscard]] auto isPathCacheDirty() const noexcept -> bool {
            return dirtyPathCache_;
        }
        void clearPathCache() noexcept {
            dirtyPathCache_ = false;
        }

        //neighbors for pathfinding
        [[nodiscard]] auto neighbors(const GridPos& p, const bool diagonal = false)
        const noexcept -> std::vector<GridPos> {
            std::vector<GridPos> result;
            const GridPos* dirs = diagonal ? DIRS_8 : DIRS_4;
            const int count = diagonal ? 8 : 4;

            for (int i = 0; i < count; ++i) {
                GridPos n = p + dirs[i];

                if (inBounds(n) && at(n).isPassable()) {
                    result.push_back(n);
                }
            }

            return result;

        }

        struct FromPos { GridPos pos; };
        struct ToPos { GridPos pos; };

         [[nodiscard]] auto cost(const FromPos from, const ToPos to) const noexcept -> float {
            (void) from; // silence compiler warning of unused variable

            if (!inBounds(to.pos)) {
                return 999.F;
            }

            return at(to.pos).traversalCost;
        }

        //procedural generation helpers
        void fill(TerrainType t) {
             for (auto& tile : tiles_) {
                 tile.terrain = t;
                 tile.blocksMovement = t==TerrainType::Wall;
                 tile.blocksVision = t==TerrainType::Wall;
                 tile.traversalCost = t==TerrainType::Wall ? 999.F : 1.F;
             }
             dirtyPathCache_ = true;
         }

        auto rawTiles() noexcept -> std::vector<Tile>& {
             return tiles_;
         }
    private:
        int width_{0};
        int height_{0};
        std::vector<Tile> tiles_;
        FogLayer playerFog_;
        FogLayer enemyFog_;
        bool dirtyPathCache_{false};

    };
}; // namespace nexus::map