#pragma once
#include <utility>

#include "../Types.hpp"
#include "../simulation/ECS.hpp"
#include "../map/Grid.hpp"
#include "../engine/EventBus.hpp"
#include "../map/Pathfinder.hpp"
#include "../simulation/Components.hpp"

namespace nexus::ai
{
    enum class BTStatus {Running, Success, Failure};

    // ─── Behavior tree nodes ──────────────────────────────────────────────────
    struct BTNode
    {
        // Virtual destructor allows derived classes to clean up properly when deleted via a base pointer.
        // Virtual = override/redefine
        virtual ~BTNode() = default;
        // =0 means pure virtual function/it's abstract
        virtual BTStatus tick(EntityID self, ecs::World& world, map::Grid& grid, EventBus& bus, float dt) = 0;
    };

    using BTNodePtr = std::unique_ptr<BTNode>;

    struct Sequence : BTNode
    {
        std::vector<BTNodePtr> children;
        size_t current{0};
        BTStatus tick(const EntityID s, ecs::World& w, map::Grid& g, EventBus& b, const float dt) override
        {
            while (current < children.size())
            {
                const auto r = children[current]->tick(s, w, g, b, dt);
                if (r == BTStatus::Failure)
                {
                    current = 0;
                    return BTStatus::Failure;
                }

                if (r == BTStatus::Running)
                {
                    return BTStatus::Running;
                }

                current += 1;

            }

            current = 0;
            return BTStatus::Success;
        }
    };

    struct Selector : BTNode
    {
        std::vector<BTNodePtr> children;
        BTStatus tick(const EntityID s, ecs::World& w, map::Grid& g, EventBus& b, const float dt) override
        {
            for (const auto& c : children)
            {
                if (const auto r = c->tick(s, w, g,b, dt); r == BTStatus::Failure)
                {
                    return r;
                }
            }

            return BTStatus::Failure;
        }

    };

    // ─── Leaf: check if target in sight ──────────────────────────────────────
    struct CanSeeTarget : BTNode
    {
        BTStatus tick(const EntityID self, ecs::World& world,
              map::Grid& grid, EventBus&, float) override
        {
            if (!std::as_const(world).hasComponent<components::AIBrain>(self)) {
                return BTStatus::Failure;
            }

            auto& brain = world.getComponent<components::AIBrain>(self);

            if (brain.target == NULL_Entity)
            {
                return BTStatus::Failure;
            }


            if (!world.isAlive(brain.target))
            {
                brain.target = NULL_Entity;
                return BTStatus::Failure;
            }

            const auto& myPos = world.getComponent<components::Position>(self).grid;
            const auto& tgtPos = world.getComponent<components::Position>(brain.target).grid;

            return pathfinding::hasLineOfSight(grid, myPos, tgtPos) ? BTStatus::Success : BTStatus::Failure;

        }
    };

    // ─── Leaf: find nearest player unit ──────────────────────────────────────
    struct FindTarget : BTNode
    {
        BTStatus tick(const EntityID self, ecs::World& world,
              map::Grid&, EventBus&, float) override
        {
            auto& brain = world.getComponent<components::AIBrain>(self);
            auto& [x, y] = world.getComponent<components::Position>(self).grid;

            float bestDist = 9999.F;
            EntityID best = NULL_Entity;

            for (const EntityID e : world.view<components::Position, components::FactionTag>())
            {
                if (auto& [faction] = world.getComponent<components::FactionTag>(e); faction != Faction::Player)
                {
                    continue;
                }

                // Ignores decoys if adapted
                if (brain.ignoreDecoys && world.hasComponent<components::CloneDecoy>(e))
                {
                    continue;
                }

                auto& p = world.getComponent<components::Position>(e).grid;

                if (const float d = std::abs(static_cast<float>(x - p.x)) +
                    std::abs(static_cast<float>(y - p.y)); d < bestDist)
                {
                    bestDist = d;
                    best = e;
                }

            }

            brain.target = best;
            return (best != NULL_Entity) ? BTStatus::Success : BTStatus::Failure;
        }

    };

    // ─── Leaf: move toward target ─────────────────────────────────────────────
    struct MoveToTarget : BTNode
    {
        BTStatus tick(const EntityID self, ecs::World& world,
                      map::Grid& grid, EventBus& bus, float) override
        {
            const auto& brain = world.getComponent<components::AIBrain>(self);

            if (brain.target == NULL_Entity || !world.isAlive(brain.target))
            {
                return BTStatus::Failure;
            }

            const auto& myPos = world.getComponent<components::Position>(self).grid;
            const auto& tgtPos = world.getComponent<components::Position>(brain.target).grid;
            auto& mv = world.getComponent<components::Movement>(self);

            if (!mv.moving || mv.path.empty() || mv.path.back() != tgtPos)
            {
                mv.path = pathfinding::findPath(grid, myPos, tgtPos, true);
                mv.pathIndex = 1;
                mv.moving = !mv.path.empty();
            }

            return mv.moving ? BTStatus::Running : BTStatus::Failure;
        }
    };

    // ─── Leaf: attack target ──────────────────────────────────────────────────
    struct AttackTarget : BTNode
    {
        float attackRange{1.5F};
        float attackCooldown{1.F};
        float cooldownTimer{0.F};
        int damage{15};

        BTStatus tick(const EntityID self, ecs::World& world,
              map::Grid&, EventBus& bus, const float dt) override
        {
            cooldownTimer -= dt;

            if (cooldownTimer > 0.F)
            {
                return BTStatus::Running;
            }

            auto& brain = world.getComponent<components::AIBrain>(self);

            if (brain.target == NULL_Entity || !world.isAlive(brain.target))
            {
                return BTStatus::Failure;
            }

            auto& [x, y]  = world.getComponent<components::Position>(self).grid;
            const auto& tgtPos = world.getComponent<components::Position>(brain.target).grid;
            // distance formula

            if (const float dist = std::hypot(
                static_cast<float>(tgtPos.x - x), static_cast<float>(tgtPos.y - y)); dist < attackRange)
            {
                auto& hp = world.getComponent<components::Health>(brain.target);
                hp.current -= damage;
                cooldownTimer = attackCooldown;

                if (!hp.isAlive())
                {
                    bus.emit
                    (UnitDiedEvent{.entity = brain.target, .faction = Faction::Player, .pos = tgtPos, .frame = 0});
                    brain.target = NULL_Entity;
                }

            return BTStatus::Success;}

        return BTStatus::Failure;
        }

    };

    // ─── Leaf: patrol between waypoints ──────────────────────────────────────
    struct PatrolRoute : BTNode {
        BTStatus tick(const EntityID self, ecs::World& world,
                      map::Grid& grid, EventBus&, float) override
        {
            auto& brain = world.getComponent<components::AIBrain>(self);
            const auto& pos   = world.getComponent<components::Position>(self).grid;
            auto& mv    = world.getComponent<components::Movement>(self);

            if (brain.patrolRoute.empty())
            {
                return BTStatus::Failure;
            }

            GridPos dest = brain.patrolRoute[brain.patrolIdx];

            if (pos == dest)
            {
                brain.patrolIdx = (brain.patrolIdx + 1) % brain.patrolRoute.size();
                dest = brain.patrolRoute[brain.patrolIdx];
            }

            if (!mv.moving || mv.path.empty() || mv.path.back() != dest)
            {
                mv.path = pathfinding::findPath(grid, pos, dest, false);
                mv.pathIndex = 1;
                mv.moving = !mv.path.empty();
            }

            return BTStatus::Running;
        }
    };

    // ─── Leaf: retreat ────────────────────────────────────────────────────────
    struct Retreat : BTNode
    {
        GridPos safePoint;
        BTStatus tick(const EntityID self, ecs::World& world,
               map::Grid& grid, EventBus&, float) override
        {
            const auto& pos = world.getComponent<components::Position>(self).grid;
            auto& mv  = world.getComponent<components::Movement>(self);

            if (const auto& hp  = world.getComponent<components::Health>(self); hp.fraction() > 0.4F)
            {
                return BTStatus::Failure; // only retreat when low
            }

            if (!mv.moving || mv.path.empty() || mv.path.back() != safePoint)
            {
                mv.path = pathfinding::findPath(grid, pos, safePoint, true);
                mv.pathIndex = 1;
                mv.moving = !mv.path.empty();
            }

            return BTStatus::Running;

        }
    };

    // ─── AI adaptation logic ──────────────────────────────────────────────────
    inline void adaptAI(ecs::World& world, EventBus& bus, const FrameNum frame)
    {
        for (const EntityID e : world.view<components::AIBrain, components::FactionTag>())
        {
            if (auto& [faction] = world.getComponent<components::FactionTag>(e); faction != Faction::Enemy)
            {
                continue;
            }


            auto& brain = world.getComponent<components::AIBrain>(e);

            // if player used lots of explosives avoid trap zones
            if (brain.playerExplosionsSeen > 3 && !brain.avoidExplosionZones)
            {
                brain.avoidExplosionZones = true;
                bus.emit(AIAdaptedEvent
                {.enemy = e, .adaptation = "Started avoiding explosion zones", .frame = frame});
            }

            // if player uses lots of decoys ignore them
            if (brain.playerDecoysSeen > 4 && !brain.ignoreDecoys)
            {
                brain.ignoreDecoys = true;
                bus.emit(AIAdaptedEvent{.enemy = e, .adaptation = "Learned to ignore paper decoys", .frame = frame});

            }

            // if player scouts constantly focus on destroying scouts
            if (brain.playerCranesSeen > 5 && !brain.focusScouts)
            {
                brain.focusScouts = true;
                bus.emit(AIAdaptedEvent
                {.enemy = e, .adaptation = "Prioritizing scout crane destruction", .frame = frame});

            }
        }
    }

    // ─── Build behavior tree for enemy unit ───────────────────────────────────
    inline BTNodePtr buildEnemyTree(const GridPos retreatPos)
    {
        auto root = std::make_unique<Selector>();

        // if hp low retreat
        {
            auto seq = std::make_unique<Sequence>();
            auto ret = std::make_unique<Retreat>();
            ret->safePoint = retreatPos;
            seq->children.push_back(std::move(ret));
            root->children.push_back(std::move(seq));
        }


        // if you can see target attack
        {
            auto seq = std::make_unique<Sequence>();
            seq->children.push_back(std::make_unique<CanSeeTarget>());
            seq->children.push_back(std::make_unique<AttackTarget>());
            root->children.push_back(std::move(seq));

        }

        // find and chase target
        {
            auto seq = std::make_unique<Sequence>();
            seq->children.push_back(std::make_unique<FindTarget>());
            seq->children.push_back(std::make_unique<MoveToTarget>());
            root->children.push_back(std::move(seq));
        }

        // default patrol
        root->children.push_back(std::make_unique<PatrolRoute>());
        return root;
    }
}