/*
 * apiQueue.cpp
 *   Implementation of the REST API -> game-loop request bridge.
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

#include "apiQueue.hpp"

crow::response ApiRequestQueue::submit(Job job, std::chrono::milliseconds timeout) {
    if(inlineMode) {
        try {
            return job();
        } catch(...) {
            return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }
    }

    std::promise<crow::response> prom;
    std::future<crow::response> fut = prom.get_future();
    // Scoped mutex
    {
        std::lock_guard<std::mutex> lock(mtx);
        pending.push_back(Pending{std::move(job), std::move(prom)});
    }

    // timed-out job still runs on drain; its promise is abandoned
    if(fut.wait_for(timeout) != std::future_status::ready)
        return crow::response(crow::status::SERVICE_UNAVAILABLE);

    return fut.get();
}

void ApiRequestQueue::drain() {
    std::deque<Pending> batch;
    {
        std::lock_guard<std::mutex> lock(mtx);
        batch.swap(pending);
    }

    for(auto& p : batch) {
        try {
            p.promise.set_value(p.job());
        } catch(...) {
            try {
                p.promise.set_value(crow::response(crow::status::INTERNAL_SERVER_ERROR));
            } catch(...) {
                // promise already satisfied/abandoned; nothing to do
            }
        }
    }
}
