// Copyright (c) 2025 Justin Andreas Lacoste <me@justin.cx>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// SPDX-License-Identifier: MIT

// A very simple thread pool using jt::mpsc<>

#include <format>
#include <functional>
#include <iostream>
#include <jt.hpp>
#include <list>
#include <thread>

constexpr size_t WORKERS = 8;

using dispatch_function = std::function<void()>;

int
main() {
    jt::mpsc<dispatch_function> thread_pool;

    jt::mpsc<dispatch_function>::consumer &lock = thread_pool.rx();

    std::list<std::thread> worker_threads{};
    for (size_t i = 0; i < WORKERS; ++i) {
        std::thread worker = std::thread([&]() -> void {
            std::function<void()> dispatch;
            // `wait`ing on the consumer with multiple threads steals
            // the newest entry for all other workers.
            while ((dispatch = lock.wait())) {
                dispatch();
            }
        });
        worker_threads.push_front(std::move(worker));
    }

    for (size_t i = 0; i < 128; ++i) {
        auto dispatch = thread_pool.tx();
        // Dispatch a simple function that prints something to the
        // console and then blocks the worker thread for some amount of time.
        dispatch([=]() {
            std::cout << std::format("This is thread #{}, it: {}\n", std::hash<std::thread::id>{}(std::this_thread::get_id()), i);
            std::this_thread::sleep_for(std::chrono::milliseconds(i));
        });
    }

    for (std::thread &th : worker_threads) {
        th.join();
    }
}
