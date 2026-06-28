// ============================================================
//  pocket/engine/core/ecs.h
//  Sparse-set based ECS. Simple, cache-friendly, fast.
// ============================================================
#pragma once

#include "core/types.h"
#include "core/log.h"
#include "core/alloc.h"

#include <tuple>
#include <utility>

namespace pk::ecs {

class World;

// Component storage base (type-erased)
class IComponentStore {
public:
    virtual ~IComponentStore() = default;
    virtual void remove(u64 entityID) = 0;
    virtual usize size() const noexcept = 0;
    virtual const char* typeName() const noexcept = 0;
    virtual const Vec<u64>& denseIds() const noexcept = 0;
};

template <typename T>
class ComponentStore final : public IComponentStore {
public:
    void insert(u64 id, T&& comp) {
        auto it = m_sparse.find(id);
        if (it != m_sparse.end()) {
            m_dense_data[it->second] = std::move(comp);
            return;
        }
        m_sparse[id] = m_dense_data.size();
        m_dense_ids.push_back(id);
        m_dense_data.push_back(std::move(comp));
    }

    template <typename... Args>
    T& emplace(u64 id, Args&&... args) {
        m_sparse[id] = m_dense_data.size();
        m_dense_ids.push_back(id);
        m_dense_data.emplace_back(std::forward<Args>(args)...);
        return m_dense_data.back();
    }

    T* get(u64 id) noexcept {
        auto it = m_sparse.find(id);
        if (it == m_sparse.end()) return nullptr;
        return &m_dense_data[it->second];
    }

    const T* get(u64 id) const noexcept {
        auto it = m_sparse.find(id);
        if (it == m_sparse.end()) return nullptr;
        return &m_dense_data[it->second];
    }

    void remove(u64 id) override {
        auto it = m_sparse.find(id);
        if (it == m_sparse.end()) return;
        usize idx = it->second;
        usize last = m_dense_data.size() - 1;
        if (idx != last) {
            m_dense_data[idx] = std::move(m_dense_data[last]);
            m_dense_ids[idx]  = m_dense_ids[last];
            m_sparse[m_dense_ids[idx]] = idx;
        }
        m_dense_data.pop_back();
        m_dense_ids.pop_back();
        m_sparse.erase(it);
    }

    bool contains(u64 id) const noexcept { return m_sparse.count(id) > 0; }

    usize size() const noexcept override { return m_dense_data.size(); }
    const char* typeName() const noexcept override { return "T"; }
    const Vec<u64>& denseIds() const noexcept override { return m_dense_ids; }

    Vec<T>&         data() noexcept       { return m_dense_data; }

private:
    Map<u64, usize> m_sparse;
    Vec<u64>        m_dense_ids;
    Vec<T>          m_dense_data;
};

class World {
public:
    World() = default;
    ~World() {
        for (auto& [_, ptr] : m_stores) delete ptr;
    }
    World(const World&)            = delete;
    World& operator=(const World&) = delete;

    EntityID create() noexcept {
        EntityID id{m_next++};
        m_alive.insert(id.value);
        return id;
    }

    bool alive(EntityID e) const noexcept {
        return m_alive.count(e.value) > 0;
    }

    void destroy(EntityID e) noexcept {
        if (!alive(e)) return;
        for (auto& [tid, store] : m_stores) store->remove(e.value);
        m_alive.erase(e.value);
    }

    template <typename T, typename... Args>
    T& add(EntityID e, Args&&... args) noexcept {
        auto& store = getOrCreateStore<T>();
        return store.emplace(e.value, T(std::forward<Args>(args)...));
    }

    template <typename T>
    T* get(EntityID e) noexcept {
        auto* store = findStore<T>();
        if (!store) return nullptr;
        return store->get(e.value);
    }

    template <typename T>
    const T* get(EntityID e) const noexcept {
        auto* store = findStore<T>();
        if (!store) return nullptr;
        return store->get(e.value);
    }

    template <typename T>
    bool has(EntityID e) const noexcept {
        auto* store = findStore<T>();
        return store ? store->contains(e.value) : false;
    }

    template <typename T>
    void remove(EntityID e) noexcept {
        auto* store = findStore<T>();
        if (store) store->remove(e.value);
    }

    // Iterate all entities that have ALL of Ts...
    // Simple, portable implementation. No std::apply - just index-based.
    template <typename... Ts, typename Fn>
    void each(Fn&& fn) noexcept {
        // Single-type fast path
        if constexpr (sizeof...(Ts) == 1) {
            using T = std::tuple_element_t<0, std::tuple<Ts...>>;
            ComponentStore<T>* store = findStore<T>();
            if (!store) return;
            auto& data = store->data();
            auto& ids  = store->denseIds();
            for (usize i = 0; i < data.size(); ++i) {
                fn(EntityID{ids[i]}, data[i]);
            }
            return;
        }

        // Multi-type path: iterate first type, lookup others by index
        using First = std::tuple_element_t<0, std::tuple<Ts...>>;
        ComponentStore<First>* driver = findStore<First>();
        if (!driver) return;

        // Array of store pointers (one per type, in order)
        std::tuple<ComponentStore<Ts>*...> stores{findStore<Ts>()...};

        // Check all non-null - use a helper
        bool all_present = checkAllNonNull(stores, std::index_sequence_for<Ts...>{});
        if (!all_present) return;

        // For each entity in driver
        auto& driver_ids = driver->denseIds();
        for (usize i = 0; i < driver_ids.size(); ++i) {
            u64 eid = driver_ids[i];
            // Get pointers to all components for this entity
            auto ptrs = getComponents<Ts...>(stores, eid, std::index_sequence_for<Ts...>{});
            // Check all non-null
            bool ok = checkAllNonNullPtrs(ptrs, std::index_sequence_for<Ts...>{});
            if (!ok) continue;
            // Call fn with dereferenced components
            callFnWithComponents<Ts...>(fn, eid, ptrs, std::index_sequence_for<Ts...>{});
        }
    }

private:
    // Helper: check all store pointers non-null
    template <typename Tuple, usize... Is>
    bool checkAllNonNull(const Tuple& t, std::index_sequence<Is...>) noexcept {
        return (... && (std::get<Is>(t) != nullptr));
    }

    // Helper: check all component pointers non-null
    template <typename Tuple, usize... Is>
    bool checkAllNonNullPtrs(const Tuple& t, std::index_sequence<Is...>) noexcept {
        return (... && (std::get<Is>(t) != nullptr));
    }

    // Helper: get all component pointers for an entity
    template <typename... Ts, typename Tuple, usize... Is>
    auto getComponents(const Tuple& stores, u64 eid, std::index_sequence<Is...>) noexcept {
        return std::make_tuple(
            std::get<Is>(stores)->get(eid)...
        );
    }

    // Helper: call fn with dereferenced components
    template <typename... Ts, typename Fn, typename Tuple, usize... Is>
    void callFnWithComponents(Fn& fn, u64 eid, const Tuple& ptrs, std::index_sequence<Is...>) noexcept {
        fn(EntityID{eid}, (*std::get<Is>(ptrs))...);
    }

public:

    usize entityCount() const noexcept { return m_alive.size(); }
    usize componentTypeCount() const noexcept { return m_stores.size(); }

    // Destroy all entities and clear all component stores
    void clear() noexcept {
        for (auto& [tid, store] : m_stores) delete store;
        m_stores.clear();
        m_alive.clear();
        m_next = 1;
    }

private:
    // Type-unique ID generator. Each T gets a unique u64.
    // Uses a static per-T counter via the address of a static function-local.
    template <typename T>
    static u64 typeIndex() noexcept {
        // Each T instantiation gets its own static variable, whose address is unique
        static char dummy = 0;
        return reinterpret_cast<u64>(&dummy);
    }

    template <typename T>
    ComponentStore<T>* findStore() noexcept {
        auto it = m_stores.find(typeIndex<T>());
        if (it == m_stores.end()) return nullptr;
        return static_cast<ComponentStore<T>*>(it->second);
    }

    template <typename T>
    const ComponentStore<T>* findStore() const noexcept {
        auto it = m_stores.find(typeIndex<T>());
        if (it == m_stores.end()) return nullptr;
        return static_cast<const ComponentStore<T>*>(it->second);
    }

    template <typename T>
    ComponentStore<T>& getOrCreateStore() noexcept {
        u64 tid = typeIndex<T>();
        auto it = m_stores.find(tid);
        if (it == m_stores.end()) {
            auto* s = new ComponentStore<T>();
            m_stores[tid] = s;
            return *s;
        }
        return *static_cast<ComponentStore<T>*>(it->second);
    }

    Map<u64, IComponentStore*> m_stores;
    Set<u64>                    m_alive;
    u64                         m_next{1};
};

// ============================================================
//  Common components
// ============================================================
struct Tag {
    String name;
};

struct Transform {
    f32 x{0}, y{0}, z{0};
    f32 rx{0}, ry{0}, rz{0};
    f32 sx{1}, sy{1}, sz{1};
};

struct Sprite {
    u32    textureID{0};
    f32    width{1}, height{1};
    f32    u0{0}, v0{0}, u1{1}, v1{1};
    f32    color[4]{1,1,1,1};
};

struct RigidBody {
    f32 vx{0}, vy{0};
    f32 mass{1};
    bool kinematic{false};
    bool useGravity{true};
};

struct Camera {
    f32 orthoSize{5};
    f32 near_{0.1f}, far_{100};
    f32 clearColor[4]{0.1f, 0.1f, 0.15f, 1};
    bool active{true};
};

} // namespace pk::ecs
