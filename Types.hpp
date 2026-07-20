// IDs, Enums, Grid Positions

#pragma once // include guard makes sure file is included only once during single compilation
#include <cstdint> // integer types
#include <functional> // handling callable objects
#include <limits>

namespace nexus {
    // ─── Fundamental IDs ───────────────────────────────────────────────────────
    using EntityID   = uint32_t; // alias for unsigned int 32 bits
    using ComponentT = uint32_t;
    using TileID     = uint32_t;
    using EventID    = uint64_t;
    using FrameNum   = uint64_t;

    // evaluate variable at compile time
    constexpr EntityID NULL_Entity = std::numeric_limits<EntityID>::max();
    constexpr TileID NULL_Tile = std::numeric_limits<TileID>::max();
    constexpr ComponentT MAX_COMPONENTS = 64;

    // ─── Grid coordinate ───────────────────────────────────────────────────────
    struct GridPos {
        int x{0}, y{0}; // 0 by default
        auto operator==(const GridPos& o) const noexcept -> bool { // noexcept means never throw exceptions
            return x == o.x && y == o.y;
        }

        auto operator!=(const GridPos& o) const noexcept -> bool {
            return !(*this == o);
        }

        auto operator+(const GridPos& o) const noexcept -> GridPos {
            return {x + o.x, y + o.y};
        }

        auto operator-(const GridPos& o) const noexcept -> GridPos {
            return {x - o.x, y - o.y};
        }
    };

    struct GridPosHash {
        auto operator()(const GridPos& p) const noexcept -> std::size_t {
            return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 16); //
        }
    };

    // ─── Directions ────────────────────────────────────────────────────────────
    constexpr GridPos DIR_N  { .x=0,.y=-1};
    constexpr GridPos DIR_S  { .x=0, .y=1};
    constexpr GridPos DIR_E  { .x=1, .y=0};
    constexpr GridPos DIR_W  {.x=-1, .y=0};
    constexpr GridPos DIR_NE { .x=1,.y=-1};
    constexpr GridPos DIR_NW {.x=-1,.y=-1};
    constexpr GridPos DIR_SE { .x=1, .y=1};
    constexpr GridPos DIR_SW {.x=-1, .y=1};
    constexpr GridPos DIRS_4[4]  = {DIR_N, DIR_S, DIR_E, DIR_W};
    constexpr GridPos DIRS_8[8]  = {DIR_N,DIR_S,DIR_E,DIR_W,DIR_NE,DIR_NW,DIR_SE,DIR_SW};

    // ─── Factions ─────────────────────────────────────────────────
    enum class Faction : uint8_t {
        Player = 0,
        Enemy = 1,
        Neutral = 2,
    };

    // ─── Paper construct types ─────────────────────────────────────────────────
    enum class PaperType : uint8_t {
        Crane = 0, // Scout
        Barrier = 1, // Wall/Cover
        CloneSlip = 2, //Decoy
        ExplosiveTag = 3, //Chain Bomb
        Bridge = 4, // Traversal Modifier
        Ramp = 5,
    };

    // ─── Tile base types ───────────────────────────────────────────────────────
    enum class TerrainType : uint8_t {
        Open        = 0,
        Wall        = 1,
        Water       = 2,
        Void        = 3,
        PaperWall   = 4,
        PaperBridge = 5,
        BurnedAsh   = 6,
    };

    // ─── Game phase ────────────────────────────────────────────────────────────
    enum class GamePhase : uint8_t {
        MainMenu,
        Playing,
        Paused,
        GameOver,
        ReplayView,
    };

} // namespace nexus