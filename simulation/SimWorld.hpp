#pragma once
#include <random>

#include "Components.hpp"
#include "../Types.hpp"
#include "../map/Grid.hpp"
#include "ECS.hpp"
#include "../engine/EventBus.hpp"
#include "../ai/BehaviorTree.hpp"
#include "../map/MapGenerator.hpp"

namespace nexus
{
    // ─── Paper construct factory ──────────────────────────────────────────────
    struct PaperFactory
    {
        static EntityID deployCrane(ecs::World& world, map::Grid& grid,
                                          const GridPos& pos, const Faction faction,
                                          EventBus& bus, const FrameNum frame)
        {
            const EntityID e = world.createEntity();
            world.addComponent<components::Position>(e, {.grid = pos, .world = {static_cast<float>(pos.x),
                static_cast<float>(pos.y)}});
            world.addComponent<components::FactionTag>(e, {faction});
            world.addComponent<components::Health>(e, {.current = 20, .max = 20});
            world.addComponent<components::PaperConstruct>(e, {PaperType::Crane, 2, 2,
                0.f, 0.f, true, true, NULL_Entity});
            world.addComponent<components::Scout>(e, {.revealRate = 1.f, .returnsToBase = false, .homeBase = pos,
                .lifeTime = 25.f});
            world.addComponent<components::Movement>(e, {.speed = 3.5f});
            world.addComponent<components::Vision>(e, {.radius = 6, .visible = {}, .memory = {},
                .owner = faction});
            world.addComponent<components::Renderable>(e, {.color = sf::Color(200,230,255),
                .symbol = '^', .layer = 2, .visible = true, .opacity = 1.f, .isDecoy = false});
            bus.emit(PaperDeployedEvent{.entity = e, .type = PaperType::Crane, .pos = pos, .deployedBy = faction,
                .frame = frame});
            return e;
        }


        static EntityID deployBarrier(ecs::World& world, map::Grid& grid,
                                   const GridPos& pos, const Faction faction,
                                   EventBus& bus, const FrameNum frame) {
            const EntityID e = world.createEntity();
            world.addComponent<components::Position>(e, {.grid = pos, .world = {static_cast<float>(pos.x),
                static_cast<float>(pos.y)}});
            world.addComponent<components::FactionTag>(e, {faction});
            world.addComponent<components::Health>(e, {.current = 60, .max = 60});
            world.addComponent<components::PaperConstruct>(e, {.type = PaperType::Barrier, .durability = 5,
                .maxDurability = 5, .activationDelay = 0.f, .chainRadius = 0.f, .activated = true, .deployed = true,
                .owner = NULL_Entity});
            world.addComponent<components::TerrainMod>(e, {.original = grid.at(pos).terrain,
                .current = TerrainType::PaperWall});
            world.addComponent<components::Renderable>(e, {.color = sf::Color(240,220,180),
                .symbol = '#', .layer = 1, .visible = true, .opacity = 1.f, .isDecoy = false});
            grid.setTerrain(pos, TerrainType::PaperWall);
            bus.emit(PaperDeployedEvent{.entity = e, .type = PaperType::Barrier, .pos = pos, .deployedBy = faction,
                .frame = frame});
            bus.emit(TileChangedEvent{.pos = pos, .oldType = TerrainType::Open, .newType = TerrainType::PaperWall,
                .frame = frame});
            return e;
        }

        static EntityID deployClone(ecs::World& world, const GridPos& pos,
                                      const EntityID target, const Faction faction,
                                      EventBus& bus, const FrameNum frame) {
            const EntityID e = world.createEntity();
            world.addComponent<components::Position>(e, {.grid = pos, .world = {static_cast<float>(pos.x),
                                                             static_cast<float>(pos.y)}
                                                     });
            world.addComponent<components::FactionTag>(e, {faction});
            world.addComponent<components::Health>(e, {.current = 10, .max = 10});
            world.addComponent<components::PaperConstruct>(e, {.type = PaperType::CloneSlip, .durability = 1,
                .maxDurability = 1,
                .activationDelay = 0.f, .chainRadius = 0.f, .activated = true, .deployed = true, .owner = NULL_Entity});
            world.addComponent<components::CloneDecoy>(e, {.mimicTarget = target, .lifetime = 30.f,
                .attractsFire = true});
            world.addComponent<components::Movement>(e, {.speed = 2.f});
            world.addComponent<components::Renderable>(e, {.color = sf::Color(180,255,180),
                .symbol = '@', .layer = 2, .visible = true, .opacity = 0.75f, .isDecoy = true});
            bus.emit(PaperDeployedEvent{.entity = e, .type = PaperType::CloneSlip, .pos = pos, .deployedBy = faction,
                .frame = frame});
            return e;
        }

        static EntityID deployExplosiveTag(ecs::World& world, const GridPos& pos,
                                            const Faction faction, const float fuseTime,
                                            EventBus& bus, const FrameNum frame) {
            const EntityID e = world.createEntity();
            world.addComponent<components::Position>(e, {.grid = pos, .world = {static_cast<float>(pos.x), static_cast<float>(pos.y)}});
            world.addComponent<components::FactionTag>(e, {faction});
            world.addComponent<components::Health>(e, {.current = 5, .max = 5});
            world.addComponent<components::PaperConstruct>(e, {.type = PaperType::ExplosiveTag, .durability = 1,
                .maxDurability = 1, .activationDelay = fuseTime, .chainRadius = 2.5f, .activated = false,
                .deployed = true, .owner = NULL_Entity});
            world.addComponent<components::Explosive>(e, {.damage = 50, .blastRadius = 2.5f, .primed = true,
                .chainReacts = true, .fuseTime = fuseTime, .triggered = false});
            world.addComponent<components::Renderable>(e, {.color = sf::Color(255,80,80),
                .symbol = '*', .layer = 2, .visible = true, .opacity = 1.f, .isDecoy = false});
            bus.emit(PaperDeployedEvent{.entity = e, .type = PaperType::ExplosiveTag, .pos = pos, .deployedBy = faction,
                .frame = frame});
            return e;
        }

        static EntityID deployBridge(ecs::World& world, map::Grid& grid,
                                       const GridPos& pos, const Faction faction,
                                       EventBus& bus, const FrameNum frame) {
            const EntityID e = world.createEntity();
            world.addComponent<components::Position>(e, {.grid = pos, .world = {static_cast<float>(pos.x),
                static_cast<float>(pos.y)}});
            world.addComponent<components::FactionTag>(e, {faction});
            world.addComponent<components::Health>(e, {.current = 40, .max = 40});
            world.addComponent<components::PaperConstruct>(e, {.type = PaperType::Bridge, .durability = 4,
                .maxDurability = 4, .activationDelay = 0.f, .chainRadius = 0.f, .activated = true, .deployed = true,
                .owner = NULL_Entity});
            world.addComponent<components::TerrainMod>(e, {.original = grid.at(pos).terrain,
                .current = TerrainType::PaperBridge});
            world.addComponent<components::Renderable>(e, {.color = sf::Color(220,200,150),
                .symbol = '=', .layer = 1, .visible = true, .opacity = 1.f, .isDecoy = false});
            grid.setTerrain(pos, TerrainType::PaperBridge);
            bus.emit(PaperDeployedEvent{.entity = e, .type = PaperType::Bridge, .pos = pos, .deployedBy = faction,
                .frame = frame});
            return e;
        }
    };

    // ─── Main simulation world ────────────────────────────────────────────────
    class SimWorld
    {

    public :
        static constexpr float TILE_SIZE = 16.f;

        // no unintended implicit type conversion
        explicit SimWorld(EventBus& bus) : bus_(bus), rng_(std::random_device{}())
        {

        }

        void init(const map::MapGenParams& params)
        {
            rng_.seed(params.seed);
            grid_ = map::MapGenerator::generate(params);
            frame_ = 0;

            // Player Commander
            const GridPos playerStart = findOpenTile(5, 5);
            commander_ = world_.createEntity();
            world_.addComponent<components::Position>(commander_,
    {.grid = playerStart, .world = {static_cast<float>(playerStart.x)*TILE_SIZE,
        static_cast<float>(playerStart.y)*TILE_SIZE}});
            world_.addComponent<components::Health>(commander_, {.current = 150, .max = 150});
            world_.addComponent<components::FactionTag>(commander_, {Faction::Player});
            world_.addComponent<components::PlayerControlled>(commander_, {});
            world_.addComponent<components::Commander>(commander_, {.paperCharges = 10, .maxPaperCharges = 10,
                .chargeRegenRate = 0.5f});
            world_.addComponent<components::Movement>(commander_, {.speed = 3.f});
            world_.addComponent<components::Vision>(commander_, {.radius = 8, .visible = {}, .memory = {},
                .owner = Faction::Player});
            world_.addComponent<components::Renderable>(commander_,
                {sf::Color(255,215,0), 'K', 3, true, 1.f,
                    false});
            world_.addComponent<components::Named>(commander_, {"Konan"});

            // Spawn Enemies
            spawnEnemyWave(5);

            // Subscribe to events for chain explosions
            bus_.subscribe<ExplosionEvent>([this](const ExplosionEvent& ev) {
                onExplosion();
            });

            bus_.subscribe<UnitDiedEvent>([this](const UnitDiedEvent& ev) {
                onUnitDied(ev);
});
        }

        void update(const float dt)
        {
            frame_ += 1;
            bus_.flush();

            updateMovement(dt);
            updateScouts(dt);
            updateCloneDecoys(dt);
            updateExplosives(dt);
            updateStatusEffects(dt);
            updateCommander(dt);
            updateAI(dt);
            updateVision();
            grid_.updateFire(dt);
            ai::adaptAI(world_, bus_, frame_);
            checkWinCondition();
            bus_.flush();
        }

        // ── Deployment API ────────────────────────────────────────────────────
        bool deployPaper(const GridPos& pos, const PaperType type)
        {
            if (!grid_.inBounds(pos))
            {
                return false;
            }

            if (!grid_.at(pos).isPassable() && type != PaperType::Barrier)
            {
                return false;
            }

            if (!world_.hasComponent<components::Commander>(commander_))
            {
                return false;
            }

            auto& cmd = world_.getComponent<components::Commander>(commander_);

            if (cmd.paperCharges <= 0)
            {
                return false;
            }

            cmd.paperCharges -= 1;

            switch (type) {
            case PaperType::Crane:
                PaperFactory::deployCrane(world_, grid_, pos, Faction::Player, bus_, frame_);
                break;
            case PaperType::Barrier:
                PaperFactory::deployBarrier(world_, grid_, pos, Faction::Player, bus_, frame_);
                break;
            case PaperType::CloneSlip:
                PaperFactory::deployClone(world_, pos, commander_, Faction::Player, bus_, frame_);
                break;
            case PaperType::ExplosiveTag:
                PaperFactory::deployExplosiveTag(world_, pos, Faction::Player, 2.f, bus_, frame_);
                break;
            case PaperType::Bridge:
                PaperFactory::deployBridge(world_, grid_, pos, Faction::Player, bus_, frame_);
                break;
            default: break;
            }

            return true;
        }

        void triggerExplosionAt(const GridPos& pos)
        {
            for (const EntityID e : world_.view<components::Explosive, components::Position>())
            {
                if (auto& ep = world_.getComponent<components::Position>(e).grid; ep == pos)
                {
                    auto& ex = world_.getComponent<components::Explosive>(e);
                    ex.triggered = true;
                }

            }
        }

        void moveCommanderTo(const GridPos& dest)
        {
            if (!grid_.inBounds(dest) || !grid_.at(dest).isPassable())
            {
                return ;
            }

            const auto& pos = world_.getComponent<components::Position>(commander_).grid;
            auto& mv = world_.getComponent<components::Movement>(commander_);
            mv.path = pathfinding::findPath(grid_, pos, dest, false);
            mv.pathIndex = 1;
            mv.moving = !mv.path.empty();
        }

        // ── Accessors ─────────────────────────────────────────────────────────
        ecs::World&     ecs()          noexcept { return world_; }
        map::Grid&      grid()         noexcept { return grid_; }
        EntityID        commanderID()  const noexcept { return commander_; }
        FrameNum        frameNum()     const noexcept { return frame_; }
        bool            isGameOver()   const noexcept { return gameOver_; }
        bool            playerWon()    const noexcept { return playerWon_; }
        int             enemiesLeft()  const noexcept { return enemyCount_; }

    private:
        // ── Systems ───────────────────────────────────────────────────────────
        void updateMovement(const float dt)
        {
            for (const EntityID e : world_.view<components::Position, components::Movement>())
            {
                auto& mv = world_.getComponent<components::Movement>(e);
                auto& pos = world_.getComponent<components::Position>(e);

                if (!mv.moving || mv.path.empty())
                {
                    continue ;
                }

                if (mv.pathIndex >= mv.path.size())
                {
                    mv.moving = false;
                    continue;
                }

                const GridPos next = mv.path[mv.pathIndex];
                const float speed = mv.speed * TILE_SIZE;
                const float dx = static_cast<float>(next.x) * TILE_SIZE - pos.world.x;
                const float dy     = static_cast<float>( next.y) * TILE_SIZE - pos.world.y;
                const float dist = std::hypot(dx, dy);

                if (const float step = speed * dt; step >= dist)
                {
                    pos.world = {static_cast<float>(next.x)*TILE_SIZE, static_cast<float>(next.y)*TILE_SIZE};
                    const GridPos old = pos.grid;
                    pos.grid = next;
                    mv.pathIndex++;

                    if (mv.pathIndex >= mv.path.size())
                    {
                        mv.moving = false;
                    }
                    bus_.emit(EntityMovedEvent{.entity = e, .from = old, .to = next, .frame = frame_});
                }

                else
                {
                    pos.world.x += dx/dist * step;
                    pos.world.y += dy/dist * step;
                }


            }


        }

        void updateScouts(const float dt)
        {
            std::vector<EntityID> toRemove;

            for (EntityID e : world_.view<components::Scout, components::Position>())
            {
                auto& scout = world_.getComponent<components::Scout>(e);
                scout.lifeTime -= dt;

                if (scout.lifeTime <= 0.F)
                {
                    toRemove.push_back(e);
                    continue;
                }

                // Move in a wandering spiral
                auto& pos = world_.getComponent<components::Position>(e).grid;

                if (auto& mv = world_.getComponent<components::Movement>(e); !mv.moving)
                {
                    // Pick Random Adjacent open tile

                    if (auto nbs = grid_.neighbors(pos, true); !nbs.empty())
                    {
                        std::uniform_int_distribution<size_t> ri(0, nbs.size() - 1);
                        GridPos dest = nbs[ri(rng_)];
                        mv.path = {pos, dest};
                        mv.pathIndex = 1;
                        mv.moving = true;
                    }
                }
            }
            for (const EntityID e : toRemove)
            {
                destroyPaperConstruct(e);
            }
        }

        void destroyPaperConstruct(const EntityID e)
        {
            if (!world_.isAlive(e))
            {
                return;
            }

            if (world_.hasComponent<components::Position>(e))
            {
                const auto& pos = world_.getComponent<components::Position>(e).grid;

                if (world_.hasComponent<components::TerrainMod>(e))
                {
                    const auto& tm = world_.getComponent<components::TerrainMod>(e);
                    grid_.setTerrain(pos, tm.original);
                    bus_.emit(TileChangedEvent{.pos = pos, .oldType = tm.current, .newType = tm.original,
                        .frame = frame_});
                }

                if (world_.hasComponent<components::PaperConstruct>(e))
                {
                    const auto& pc = world_.getComponent<components::PaperConstruct>(e);
                    bus_.emit(PaperDestroyedEvent{.entity = e, .type = pc.type, .pos = pos, .frame = frame_});
                }
            }

            world_.destroyEntity(e);
        }

        void updateCloneDecoys(const float dt)
        {
            std::vector<EntityID> toRemove;

            for (EntityID e : world_.view<components::CloneDecoy, components::Position>())
            {
                auto& decoy = world_.getComponent<components::CloneDecoy>(e);
                decoy.lifetime -= dt;
                if (decoy.lifetime <= 0.F)
                {
                    toRemove.push_back(e);
                    continue;
                }

                // Mirror Target Movement
                if (decoy.mimicTarget != NULL_Entity && world_.isAlive(decoy.mimicTarget))
                {
                    auto& myPos = world_.getComponent<components::Position>(e);
                    const auto& tgtPos = world_.getComponent<components::Position>(decoy.mimicTarget);

                    //Offset by a bit to look like a real unit

                    if (auto& mv = world_.getComponent<components::Movement>(e); !mv.moving)
                    {
                        if (GridPos offset{.x = tgtPos.grid.x + 2, .y = tgtPos.grid.y + 1}; grid_.inBounds(offset) &&
                            grid_.at(offset).isPassable())
                        {
                            mv.path = pathfinding::findPath(grid_, myPos.grid, offset, false);
                            mv.pathIndex = 1;
                            mv.moving = !mv.path.empty();
                        }
                    }
                }
            }

            for (const EntityID e : toRemove)
            {
                destroyPaperConstruct(e);
            }
        }

        void updateExplosives(const float dt)
        {
            std::vector<EntityID> toDetonate;

            for (EntityID e : world_.view<components::Explosive, components::Position>())
            {
                auto& ex = world_.getComponent<components::Explosive>(e);

                if (!ex.primed && !ex.triggered)
                {
                    continue;
                }

                if (ex.triggered)
                {
                    ex.fuseTime -= dt;

                    if (ex.fuseTime <= 0.F)
                    {
                        toDetonate.push_back(e);
                    }
                }

            }

            for (const EntityID e : toDetonate)
            {
                const auto& ex = world_.getComponent<components::Explosive>(e);
                auto& pos = world_.getComponent<components::Position>(e).grid;
                bus_.emit(ExplosionEvent{.origin = pos, .radius = ex.blastRadius, .damage = ex.damage, .source = e,
                    .frame = frame_, .chainTrigger = false});
                doExplosion(pos, ex.blastRadius, ex.damage);
                destroyPaperConstruct(e);
            }
        }

        void doExplosion(const GridPos& center, const float radius, const int damage) {
            const int r = static_cast<int>(radius);
            for (int dy=-r; dy<=r; ++dy) for (int dx=-r; dx<=r; ++dx) {
                if (dx*dx+dy*dy > r*r) continue;
                GridPos pos{.x = center.x+dx, .y = center.y+dy};
                if (!grid_.inBounds(pos)) continue;

                // Damage entities
                for (const EntityID e : world_.view<components::Position, components::Health>()) {
                    if (auto& ep = world_.getComponent<components::Position>(e).grid; ep == pos) {
                        auto& hp = world_.getComponent<components::Health>(e);
                        hp.current -= damage;
                        if (!hp.isAlive()) {
                            auto& ft = world_.getComponent<components::FactionTag>(e);
                            bus_.emit(UnitDiedEvent{.entity = e, .faction = ft.faction, .pos = pos, .frame = frame_});
                        }
                    }
                }

                // Ignite tile
                if (grid_.at(pos).terrain != TerrainType::Wall)
                    grid_.ignite(pos, 4.f);

                // Chain reaction
                if (grid_.at(pos).construct != NULL_Entity) {
                    if (const EntityID c = grid_.at(pos).construct; world_.isAlive(c) &&
                        world_.hasComponent<components::Explosive>(c)) {
                        if (auto& ex = world_.getComponent<components::Explosive>(c); ex.chainReacts &&
                            !ex.triggered) {
                            ex.triggered = true; ex.fuseTime = 0.15f;
                        }
                    }
                }
            }
        }

        void updateStatusEffects(const float dt)
        {
            std::vector<EntityID> toBurn;

            for (EntityID e : world_.view<components::StatusEffects, components::Health>())
            {
                auto& se = world_.getComponent<components::StatusEffects>(e);
                auto& hp = world_.getComponent<components::Health>(e);

                if (se.burning)
                {
                    se.burnDuration -= dt;
                    hp.current -= static_cast<int>(static_cast<float>(se.burnDamagePerSec) * dt);

                    if (se.burnDuration <= 0.F)
                    {
                        se.burning = false;

                    }

                    if (!hp.isAlive())
                    {
                        toBurn.push_back(e);
                    }
                }

                if (se.stunned)
                {
                    se.stunDuration -= dt;
                }

                if (se.stunDuration <= 0.F)
                {
                    se.stunned = false;
                }
            }

            for (const EntityID e : toBurn)
            {
                auto& [faction] = world_.getComponent<components::FactionTag>(e);
                const auto& p = world_.getComponent<components::Position>(e).grid;
                bus_.emit(UnitDiedEvent{.entity = e, .faction = faction, .pos = p, .frame = frame_});
            }
        }

        void updateCommander(const float dt)
        {
            if (!world_.isAlive(commander_))
            {
                return;
            }

            auto& cmd = world_.getComponent<components::Commander>(commander_);
            cmd.chargeRegenAccum += dt * cmd.chargeRegenRate;

            while (cmd.chargeRegenAccum >= 1.F && cmd.paperCharges < cmd.maxPaperCharges)
            {
                cmd.paperCharges += 1;
                cmd.chargeRegenAccum -= 1.F;
            }
        }

        void updateAI(const float dt)
        {
            for (EntityID e : world_.view<components::AIBrain, components::FactionTag>())
            {
                if (!world_.hasComponent<components::Movement>(e))
                {
                    continue;
                }

                if (auto& [faction] = world_.getComponent<components::FactionTag>(e); faction != Faction::Enemy)
                {
                    continue;
                }

                if (auto it = aiBehaviorTrees_.find(e); it != aiBehaviorTrees_.end())
                {
                    it->second->tick(e, world_, grid_, bus_, dt);
                }


            }
        }

        void updateVision()
        {
            // Step: clear previous visible
            grid_.playerFog().stepFrame();

            for (const EntityID e : world_.view<components::Vision, components::Position>())
            {
                const auto& vis = world_.getComponent<components::Vision>(e);
                auto& pos = world_.getComponent<components::Position>(e).grid;

                if (vis.owner == Faction::Player)
                {
                    grid_.revealForm(pos, vis.radius, grid_.playerFog());
                }

                else
                {
                    grid_.revealForm(pos, vis.radius, grid_.enemyFog());
                }
            }
        }

        void onExplosion()
        {
            // Track for AI Adaption
            for (const EntityID e : world_.view<components::AIBrain>())
            {
                auto& brain = world_.getComponent<components::AIBrain>(e);
                brain.playerExplosionsSeen += 1;
            }
        }

        void onUnitDied(const UnitDiedEvent& ev)
        {
            if (ev.entity == commander_)
            {
                gameOver_ = true;
                playerWon_ = false;
            }

            if (world_.isAlive(ev.entity))
            {
                world_.destroyEntity(ev.entity);
                aiBehaviorTrees_.erase(ev.entity);

            }

            if (ev.faction == Faction::Enemy)
            {
                enemyCount_ -= 1;

                if (enemyCount_ <= 0)
                {
                    gameOver_ = true;
                    playerWon_ = true;
                }
            }
        }

        void spawnEnemyWave(const int count)
        {
            std::uniform_int_distribution rx(grid_.width() - 15, grid_.width() - 5);
            std::uniform_int_distribution ry(3, grid_.height()-5);

            for (int i = 0; i < count; ++i)
            {
                GridPos pos;

                for (int tries = 0; tries < 20; ++tries)
                {
                    pos = {.x = rx(rng_), .y = ry(rng_)};

                    if (grid_.inBounds(pos) && grid_.at(pos).isPassable())
                    {
                        break;
                    }
                }

                EntityID e = world_.createEntity();
                world_.addComponent<components::Position>(e, {pos,
{static_cast<float>(pos.x)*TILE_SIZE,static_cast<float>(pos.y)*TILE_SIZE}});
                world_.addComponent<components::Health>(e, {.current = 80, .max = 80});
                world_.addComponent<components::FactionTag>(e, {Faction::Enemy});
                world_.addComponent<components::Movement>(e, {.speed = 2.f});
                world_.addComponent<components::Vision>(e, {.radius = 5, .visible = {}, .memory = {},
                    .owner = Faction::Enemy});
                world_.addComponent<components::Renderable>(e,
                    {.color = sf::Color(255,100,100), .symbol = 'E', .layer = 2, .visible = true,
                        .opacity = 1.f,.isDecoy = false});

                // Patrol Route
                components::AIBrain brain;
                brain.state = components::AIState::Patrol;
                brain.patrolRoute = {pos, {.x = pos.x-4, .y = pos.y}, {pos.x-4, pos.y+3}, {pos.x, pos.y+3}};
                world_.addComponent<components::AIBrain>(e, brain);
                world_.addComponent<components::StatusEffects>(e, {});

                aiBehaviorTrees_[e] = ai::buildEnemyTree(pos);
                enemyCount_ += 1;
            }
        }

        void checkWinCondition()
        {
            if (gameOver_)
            {
                return ;
            }

            if (!world_.isAlive(commander_))
            {
                gameOver_ = true;
                playerWon_ = false;
                bus_.emit(GameOverEvent{.playerWon = false, .frame = frame_});
            }
        }

        GridPos findOpenTile(const int nearX, const int nearY) const
        {
            for (int r = 0; r < 20; ++r)
            {
                for (int dy=-r; dy<=r; ++dy)
                {
                    for (int dx=-r; dx<=r; ++dx)
                    {
                        if (GridPos p{.x = nearX+dx, .y = nearY+dy}; grid_.inBounds(p) && grid_.at(p).isPassable())
                        {
                            return p;
                        }
                    }
                }
            }

            return {.x = 1, .y = 1};
        }

        ecs::World  world_;
        map::Grid   grid_;
        EventBus&   bus_;
        EntityID    commander_{NULL_Entity};
        FrameNum    frame_{0};
        bool        gameOver_{false};
        bool        playerWon_{false};
        int         enemyCount_{0};
        std::mt19937 rng_;
        std::unordered_map<EntityID, std::unique_ptr<ai::BTNode>> aiBehaviorTrees_;
    };
};
