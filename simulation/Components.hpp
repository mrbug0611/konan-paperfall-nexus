#pragma once
#include <string>
#include <SFML/Graphics/Color.hpp> // use of color
#include <SFML/System/Vector2.hpp> // manipulate 2d coordinates and vectors

#include "../Types.hpp"

namespace nexus::components {
    // ─── Position ─────────────────────────────────────────────────────────────
    struct Position {
        GridPos grid;
        sf::Vector2f world; // pixel position for smooth movement
        float subX{0.F};
        float subY{0.F}; // lerp (linear interpolation) progress 0-1 (default value 0.0) f tells you it's a float

    };

    // ─── Health ───────────────────────────────────────────────────────────────
    struct Health {
        int current{100};
        int max{100};

        [[nodiscard]] auto isAlive() const noexcept -> bool {
            return current > 0;
        }

        [[nodiscard]] auto fraction() const noexcept -> float {
            return static_cast<float>(current) / static_cast<float>(max);
        }

    };

    // ─── Vision ───────────────────────────────────────────────────────────────
    struct Vision {
        int radius{5};
        std::vector<GridPos> visible; // currently visible tiles
        std::vector<GridPos> memory; // ever-seen tiles
        Faction owner{Faction::Player};
    };

    // ─── Movement ─────────────────────────────────────────────────────────────
    struct Movement {
        float speed{2.f}; // tiles per second
        std::vector<GridPos> path;
        size_t pathIndex{0};
        float moveProgress{0.F};
        bool moving{false};
    };

    // ─── Faction tag ──────────────────────────────────────────────────────────
    struct FactionTag {
        Faction faction{Faction::Neutral};
    };

    // ─── Render info ──────────────────────────────────────────────────────────
    struct Renderable {
        sf::Color color{sf::Color::White};
        char symbol{'?'};
        int layer{0}; // draw order
        bool visible{true};
        float opacity{1.f};
        bool isDecoy{false};


    };

    // ─── Paper construct ──────────────────────────────────────────────────────
    struct PaperConstruct {
        PaperType type{PaperType::Crane};
        int durability{3};
        int maxDurability{3};
        float activationDelay{0.f}; // seconds before active
        float chainRadius{2.f}; // for explosive tags
        bool activated{false};
        bool deployed{false};
        EntityID owner{NULL_Entity};


    };

    // ─── Clone decoy ──────────────────────────────────────────────────────────
    struct CloneDecoy {
        EntityID mimicTarget{NULL_Entity};
        float lifetime{30.f}; // seconds before dissolving
        bool attractsFire{true};

    };

    // ─── Explosive ────────────────────────────────────────────────────────────
    struct Explosive {
        int damage{40};
        float blastRadius{2.5f};
        bool primed{false};
        bool chainReacts{true};
        float fuseTime{0.f}; // 0 = instant on trigger
        bool triggered{false};

    };

    // ─── Scout behavior ───────────────────────────────────────────────────────
    struct Scout {
        float revealRate{1.F}; //tiles/second
        bool returnsToBase{false};
        GridPos homeBase{0,0};
        float lifeTime{60.F};

    };

    // ─── AI behavior ──────────────────────────────────────────────────────────
    enum class AIState : uint8_t {
        Idle, Patrol, Chase, Attack, Retreat, Scout, Defend, Ambush
    };

    struct AIBrain {
        AIState state{AIState::Patrol};
        EntityID target{NULL_Entity};
        GridPos patrolPoint{0,0};
        std::vector<GridPos> patrolRoute;
        size_t patrolIdx{0};

        // Adaptive counters
        int playerCranesSeen{0};
        int playerBarriersSeen{0};
        int playerDecoysSeen{0};
        int playerExplosionsSeen{0};
        bool avoidExplosionZones{false};
        bool ignoreDecoys{false};
        bool focusScouts{false};

        float alertCooldown{0.F};
        float lastSeenPlayerTime{-999.F};
        GridPos lastKnownPlayerPos{.x=-1, .y=-1};
    };

    // ─── Status effects ───────────────────────────────────────────────────────
    struct StatusEffects {
        bool burning{false};
        float burnDuration{0.F};
        int burnDamagePerSec{5};

        bool stunned{false};
        float stunDuration{0.F};

        bool concealed{false}; // behind barrier cover
        float concealment {0.F}; // 0-1
    };

    // ─── Terrain modifier (attached to tile entity) ───────────────────────────
    struct TerrainMod {
        TerrainType original{TerrainType::Open};
        TerrainType current{TerrainType::Open};
        float traversalCost{1.F};
        bool blocksMovement{false};
        bool blocksVision{false};
        bool onFire{false};
        float fireDuration{0.F};
    };

    // ─── Tag: player-controlled ───────────────────────────────────────────────
    struct PlayerControlled {};

    // ─── Commander (player's main unit) ───────────────────────────────────────
    struct Commander {
        int paperCharges{10};
        int maxPaperCharges{10};
        float chargeRegenRate{0.5F};
        float chargeRegenAccum{0.F};
        PaperType selectedType{PaperType::Crane};
    };

    // ─── Named ────────────────────────────────────────────────────────────────
    struct Named {
        std::string name;
    };


}// namespace nexus::components
