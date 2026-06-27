// ============================================================
//  pocket/engine/core/event.h
//  Type-safe event bus
// ============================================================
#pragma once

#include "core/types.h"
#include <functional>
#include <any>

namespace pk::event {

using SubscriberID = u64;

class Bus {
public:
    template <typename E>
    SubscriberID subscribe(std::function<void(const E&)> fn) noexcept {
        u64 tid = eventTypeID<E>();
        SubscriberID sid = m_nextID++;
        std::lock_guard<std::mutex> lk(m_mutex);
        m_subscribers[tid].push_back({sid, [fn](const std::any& a) {
            fn(std::any_cast<const E&>(a));
        }});
        return sid;
    }

    void unsubscribe(SubscriberID sid) noexcept {
        std::lock_guard<std::mutex> lk(m_mutex);
        for (auto& [_, list] : m_subscribers) {
            for (auto it = list.begin(); it != list.end(); ++it) {
                if (it->sid == sid) { list.erase(it); return; }
            }
        }
    }

    template <typename E>
    void publish(const E& evt) noexcept {
        u64 tid = eventTypeID<E>();
        std::vector<Subscriber> snapshot;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            auto it = m_subscribers.find(tid);
            if (it == m_subscribers.end()) return;
            snapshot = it->second;
        }
        std::any a = std::any(std::cref(evt));
        for (auto& s : snapshot) s.fn(a);
    }

private:
    template <typename E>
    static u64 eventTypeID() noexcept {
        static const u64 id = []{ static u64 x = 0; return ++x; }();
        return id;
    }

    struct Subscriber {
        SubscriberID sid;
        std::function<void(const std::any&)> fn;
    };

    std::mutex                           m_mutex;
    Map<u64, std::vector<Subscriber>>    m_subscribers;
    SubscriberID                         m_nextID{1};
};

Bus& bus() noexcept;

} // namespace pk::event

// Common events
namespace pk::events {

struct WindowResize { i32 w, h; };
struct KeyPress      { i32 key; i32 scancode; i32 mods; };
struct KeyRelease    { i32 key; i32 scancode; i32 mods; };
struct MouseMove     { f32 x, y; };
struct MousePress    { i32 button; f32 x, y; };
struct MouseRelease  { i32 button; f32 x, y; };
struct TouchStart    { i32 id; f32 x, y; };
struct TouchMove     { i32 id; f32 x, y; };
struct TouchEnd      { i32 id; f32 x, y; };
struct SceneLoaded   { String path; };

} // namespace pk::events
