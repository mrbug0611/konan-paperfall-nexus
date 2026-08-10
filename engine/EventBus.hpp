#pragma once
#include <string>
#include <typeindex>
#include <functional>
#include <unordered_map>
#include <any>
#include <vector>

#include "../Types.hpp"

namespace std
{
}

namespace nexus
{
    // ─── Event types ──────────────────────────────────────────────────────────
    struct EntityMovedEvent {
        EntityID entity{};
        GridPos from{}, to{};
        FrameNum frame{};
    };

    struct ExplosionEvent {
        GridPos origin{};
        float radius{};
        int damage{};
        EntityID source{};
        FrameNum frame{};
        bool chainTrigger{false};
    };

    struct PaperDeployedEvent {
        EntityID entity{};
        PaperType type{};
        GridPos pos{};
        Faction deployedBy{};
        FrameNum frame{};
    };

    struct PaperDestroyedEvent {
        EntityID entity{};
        PaperType type{};
        GridPos pos{};
        FrameNum frame{};
    };

    struct FogRevealedEvent {
        EntityID revealer;
        std::vector<GridPos> tiles;
        Faction faction;
        FrameNum frame;
    };

    struct UnitDiedEvent {
        EntityID entity{};
        Faction faction{};
        GridPos pos{};
        FrameNum frame{};
    };

    struct TileChangedEvent {
        GridPos pos{};
        TerrainType oldType{}, newType{};
        FrameNum frame{};
    };

    struct AIAdaptedEvent {
        EntityID enemy;
        std::string adaptation;
        FrameNum frame;
    };

    struct ChainReactionEvent {
        std::vector<GridPos> chain;
        FrameNum frame;
    };

    struct GameOverEvent {
        bool playerWon;
        FrameNum frame;
    };

    // ─── Event bus ────────────────────────────────────────────────────────────
    class EventBus
    {

    public:
        // Replay: record all emitted events
        struct RecordedEvent {
            std::type_index type;
            std::any payload;
            FrameNum frame;
        };

        template <typename E>
        using Handler = std::function<void(const E&)>;

        template <typename E>
        void subscribe(Handler<E> handler)
        {
            const auto key = std::type_index(typeid(E)); // determine type of expression
            handlers_[key].push_back([h=std::move(handler)](const std::any& e)
            {
                h(std::any_cast<const E&>(e));
            });
        }

        template <typename E>
        void emit(E event)
        {
            // Buffer for deferred dispatch (avoids re-entrant loops)
            pending_.emplace_back(std::type_index(typeid(E)), std::any(std::move(event)));
        }

        void flush()
        {
            // swap so handlers emit new events safely
            decltype(pending_) current; // extract type without evaluating it
            current.swap(pending_);

            for (auto& [key, payload] : current)
            {
                if (auto it = handlers_.find(key); it != handlers_.end())
                {
                    for (auto& h : it->second) h(payload);
                }
            }
        }

        void enableRecording(const bool on)
        {
            recording_ = on;
        }

        const std::vector<RecordedEvent>& recording() const
        {
            return recorded_;
        }

        void clearRecording()
        {
            recorded_.clear();
        }

    private:
        // functional is a wrapper makes everything easier to work with
        std::unordered_map<std::type_index, std::vector<std::function<void(const std::any&)>>> handlers_;
        std::vector<std::pair<std::type_index, std::any>> pending_;

        bool recording_{false};
        std::vector<RecordedEvent> recorded_;



    };

};