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

    T* get(u64 id) noexcept {
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

    // Iteration
    template <typename Fn>
    void each(Fn&& fn) {
        for (usize i = 0; i < m_dense_data.size(); ++i) {
            fn(m_dense_ids[i], m_dense_data[i]);
        }
    }

    Vec<u64> const& ids()      const noexcept { return m_dense_ids; }
    Vec<T>&         data()     noexcept       { return m_dense_data; }
    usize           count()    const noexcept { return m_dense_data.size(); }

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
        store.insert(e.value, T(std::forward<Args>(args)...));
        return *store.get(e.value);
    }

    template <typename T>
    T* get(EntityID e) noexcept {
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
    template <typename... Ts, typename Fn>
    void each(Fn&& fn) noexcept {
        // Pick the smallest store as driver
        auto* driver = smallestStore<Ts...>();
        if (!driver) return;

        // For each entity in driver, check others, call fn
        // Use apply pattern
        eachImpl<Ts...>(std::forward<Fn>(fn), driver);
    }

    usize entityCount() const noexcept { return m_alive.size(); }
    usize componentTypeCount() const noexcept { return m_stores.size(); }

private:
    template <typename T>
    static u64 typeIndex() noexcept {
        static const u64 id = []{ static u64 x = 0; return x++; }();
        return id;
    }

    template <typename T>
    ComponentStore<T>* findStore() const noexcept {
        auto it = m_stores.find(typeIndex<T>());
        if (it == m_stores.end()) return nullptr;
        return static_cast<ComponentStore<T>*>(it->second);
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

    template <typename... Ts>
    IComponentStore* smallestStore() const noexcept {
        IComponentStore* best = nullptr;
        usize bestSize = SIZE_MAX;
        IComponentStore* arr[] = { findStore<Ts>()... };
        for (auto* s : arr) {
            if (!s) return nullptr;
            if (s->size() < bestSize) {
                bestSize = s->size();
                best = s;
            }
        }
        return best;
    }

    template <typename... Ts, typename Fn>
    void eachImpl(Fn&& fn, IComponentStore* driver) noexcept {
        // Determine which type the driver store is by checking addresses
        // For simplicity, we iterate over all stores together by id set intersection
        // Use the dense id list of the smallest store
        // We need to know which type it is - we use a recursive search
        eachImplDispatch<Ts...>(std::forward<Fn>(fn), driver, std::index_sequence_for<Ts...>{});
    }

    template <typename... Ts, typename Fn, usize... Is>
    void eachImplDispatch(Fn&& fn, IComponentStore* driver, std::index_sequence<Is...>) noexcept {
        // Use store of T0 as driver if it matches, otherwise find correct one
        // For simplicity here: iterate T0's store and filter
        using First = std::tuple_element_t<0, std::tuple<Ts...>>;
        auto* d = findStore<First>();
        if (!d) return;
        auto& ids = d->ids();
        auto& data = d->data();
        for (usize i = 0; i < ids.size(); ++i) {
            u64 eid = ids[i];
            // Get all other components
            auto ptrs = std::make_tuple(getComponentPtr<Ts>(eid, Is)...);
            if (allValid<Ts...>(ptrs, std::index_sequence_for<Ts...>{})) {
                fn(EntityID{eid}, *std::get<Is* const>(ptrs)...);
            }
        }
        (void)driver;
    }

    template <typename T, usize I>
    T* getComponentPtr(u64 eid, std::integral_constant<usize, I>) noexcept {
        auto* s = findStore<T>();
        if (!s) return nullptr;
        // First component (I==0) should reuse `data` instead of lookups
        return s->get(eid);
    }

    template <typename... Ts, typename Tuple, usize... Is>
    bool allValid(Tuple& t, std::index_sequence<Is...>) noexcept {
        return (... && (std::get<Is>(t) != nullptr));
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
    static const char* StaticTypeName() { return "Tag"; }
};

struct Transform {
    f32 x{0}, y{0}, z{0};
    f32 rx{0}, ry{0}, rz{0};
    f32 sx{1}, sy{1}, sz{1};
    static const char* StaticTypeName() { return "Transform"; }
};

struct Sprite {
    u32    textureID{0};
    f32    width{1}, height{1};
    f32    u0{0}, v0{0}, u1{1}, v1{1};
    f32    color[4]{1,1,1,1};
    static const char* StaticTypeName() { return "Sprite"; }
};

struct RigidBody {
    f32 vx{0}, vy{0};
    f32 mass{1};
    bool kinematic{false};
    bool useGravity{true};
    static const char* StaticTypeName() { return "RigidBody"; }
};

struct Camera {
    f32 orthoSize{5};
    f32 near_{0.1f}, far_{100};
    f32 clearColor[4]{0.1f, 0.1f, 0.15f, 1};
    bool active{true};
    static const char* StaticTypeName() { return "Camera"; }
};

} // namespace pk::ecs
