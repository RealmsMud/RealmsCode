/*
 * apiQueue.hpp
 *   Marshals REST API work from Crow worker threads onto the game loop thread.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <future>
#include <mutex>

#include <crow.h>

class ApiRequestQueue {
public:
    using Job = std::function<crow::response()>;

    crow::response submit(Job job, std::chrono::milliseconds timeout = std::chrono::seconds(5));
    void drain();

    void setInlineMode(bool v) { inlineMode = v; }
    [[nodiscard]] bool isInlineMode() const { return inlineMode; }

private:
    struct Pending {
        Job job;
        std::promise<crow::response> promise;
    };

    std::mutex mtx;
    std::deque<Pending> pending;
    bool inlineMode = false;
};
