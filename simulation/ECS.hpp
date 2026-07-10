// World, Sparse set Component
#pragma once
#include <bitset>

#include "../Types.hpp"
#include <cassert>
#include <memory>
#include <ranges>

namespace nexus::ecs {
    // ─── Component type registry ──────────────────────────────────────────────
    class ComponentRegistry {
    public:
        template<typename T> // can adapt to variety of types
        static ComponentT id() {
            static ComponentT cid = nextID();
            assert(cid < MAX_COMPONENTS && "Component Limit Exceeded");
            return cid;

        }
    private:
        static ComponentT nextID() {
            static ComponentT counter = 0;
            return counter++;
        }
    };

    // ─── Sparse set for O(1) add/remove/lookup ───────────────────────────────
    template<typename T>
    class SparseSet {
        public:
            void insert(const EntityID e, T component) {
                if (e >= sparse_.size()) {
                    sparse_.resize(e+1, UINT32_MAX);
                }
                assert(sparse_[e] == UINT32_MAX && "Duplicate Component");
                sparse_[e] = static_cast<uint32_t>(dense_entities_.size()); //convert between data types
                dense_entities_.push_back(e);
                dense_data_.push_back(std::move(component));
            }

            void remove(const EntityID e) {
                if (e >= sparse_.size() || sparse_[e] == UINT32_MAX) {
                    return;
                }

                uint32_t idx = sparse_[e];
                const EntityID last = dense_entities_.back();
                dense_entities_[idx] = last;
                dense_data_[idx] = std::move(dense_data_.back());
                sparse_[last] = idx;
                dense_entities_.pop_back();
                dense_data_.pop_back();
                sparse_[e] = UINT32_MAX;

            }

            [[nodiscard]] bool has(const EntityID e) const noexcept {
                    return e < sparse_.size() && sparse_[e] != UINT32_MAX;
                }

            T& get(const EntityID e) {
                assert(has(e));
                return dense_data_[sparse_[e]];
            }

            const T& get(const EntityID e) const {
                    assert(has(e));
                    return dense_data_[sparse_[e]];
                }

            std::vector<EntityID>& entities() noexcept {
                return dense_entities_;
            }

            [[nodiscard]] const std::vector<EntityID>& entities() const noexcept {
                return dense_entities_;
            }

            [[nodiscard]] size_t size() const noexcept {
                return dense_entities_.size();
            }

        private:
            std::vector<uint32_t> sparse_;
            std::vector<EntityID> dense_entities_;
            std::vector<T> dense_data_;

    };

    // ─── Abstract pool base ───────────────────────────────────────────────────
    struct IPool {
        virtual ~IPool() = default;
        virtual void remove(EntityID e) = 0;
        [[nodiscard]] virtual bool has() const = 0;
    };

    template<typename T>
    struct Pool : IPool {
        SparseSet<T> set;
        void remove(EntityID e) override {
            set.remove(e);
        }

        [[nodiscard]] bool has() const override {
            return set.has();
        }
    };

    // ─── ECS World ────────────────────────────────────────────────────────────
    class World {
        public:
            // Entity Lifecycle
            EntityID createEntity() {
                EntityID id;

                if (!free_.empty()) {
                    id = free_.back();
                    free_.pop_back();
                }

                else {
                    id = nextID_++;
                    if (id >= signatures_.size()) {
                        signatures_.resize(id + 1);
                    }
                }

                signatures_[id].reset();
                alive_.push_back(id);
                return id;


            }

            void destroyEntity(const EntityID e) {
                for (const auto &pool: pools_ | std::views::values) {
                    pool -> remove(e);
                }
                signatures_[e].reset();
                std::erase(alive_, e);
                free_.push_back(e);
            }

            bool isAlive(const EntityID e) const noexcept {
                return e < signatures_.size() &&
                    std::ranges::find(alive_, e) != alive_.end();
            }

            // component add / get/ remove/ has
            template<typename T>
            T& addComponent(EntityID e, T comp) {
                auto cid = ComponentRegistry::id<T>(); // auto means compiler automatically deduce data type
                auto& pool = getOrCreatePool<T>(cid);
                pool.set.insert(e, std::move(comp));
                signatures_[e].set(cid);
                return pool.set.get(cid);
            }

            template<typename T>
            T& getComponent(EntityID e) {
                return getPool<T>().set.get(e);
            }

            template<typename T>
            bool hasComponent(EntityID e) const noexcept {
                auto cid = ComponentRegistry::id<T>();
                return cid < pools_.size() && pools_.count(cid) && pools_.at(cid)->has(e);
            }

            template<typename T>
            void removeComponent(EntityID e) {
                auto cid = ComponentRegistry::id<T>();
                getPool<T>().set().remove(e);
                if (e < signatures_.size()) {
                    signatures_[e].reset(cid);
                }
            }

            // view: Iterate over entities with all listed components
            template<typename... Ts> // accept 0, 1 or many datatypes
            std::vector<EntityID> view() const {
                std::vector<EntityID> result;

                if (alive_.empty()) {
                    return result;
                }

                // gather and intersect
                for (EntityID e : alive_) {
                    if ((hasComponent<Ts>(e) && ...)) {
                        result.push_back(e);
                    }
                }

                return result;
            }

            const std::vector<EntityID>& allEntries() const noexcept {
                return alive_;
            }

            size_t entityCount() const noexcept {
                return alive_.size();
            }
        private:

            template<typename T>
            Pool<T> getOrCreatePool(const ComponentT cid) {
                if (!pools_.contains(cid)) {
                    pools_[cid] = std::make_unique<Pool<T>>();
                }

                return *static_cast<Pool<T>*>(pools_[cid].get());
            }

            template<typename T>
            Pool<T>& getPool() {
                auto cid = ComponentRegistry::id<T>();
                auto it = pools_.find(cid);

                if (it == pools_.end()) {
                    throw std::runtime_error("No pool found for id " + std::to_string(cid));
                }

                return *static_cast<Pool<T>*>(it->second.get());
            }

            template<typename T>
            const Pool<T>& getPool() const {
                auto cid = ComponentRegistry::id<T>();
                auto it = pools_.find(cid);
                if (it == pools_.end()) throw std::runtime_error("Component pool not found");
                return *static_cast<const Pool<T>*>(it->second.get());
            }
            std::vector<EntityID> free_;
            EntityID nextID_ = 0;
            std::vector<std::bitset<MAX_COMPONENTS>> signatures_;
            std::vector<EntityID> alive_;
            std::unordered_map<ComponentT, std::unique_ptr<IPool>> pools_;
    };



};// namespace nexus::ecs