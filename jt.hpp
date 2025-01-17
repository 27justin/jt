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
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <iterator>
#include <list>
#include <mutex>

namespace jt {

template<typename T>
using fifo = std::back_insert_iterator<T>;

template<typename T>
using lifo = std::front_insert_iterator<T>;

// You may want to overload the `mpsc` template.
// The template parameters are as follows:
// 1. `value`, the type you're going to send to the consumer
// 2. `insertion_strategy`, the strategy used to insert values into the consumer queue.
//    default is FIFO (`std::front_insert_iterator`, or `jt::fifo`), for LIFO use `std::back_insert_iterator`, or `jt::lifo`
// 3. `queue`, the datatype for the consumer.  Has to be compatible with the `insertion_strategy`.

template<typename T>
using default_queue = std::deque<T>;

template<typename value, template<typename> class insertion_strategy = fifo, template<typename> class queue = default_queue>
struct mpsc {
    struct consumer {
        private:
        queue<value>            queue_;
        mutable std::mutex      queue_lock_;
        std::condition_variable cv_;

        public:
        consumer() {}
        consumer(const consumer &) = delete;
        consumer(const consumer &&) = delete;

        value wait() {
            std::unique_lock<std::mutex> lock(queue_lock_);
            cv_.wait(lock, [this] { return !queue_.empty(); });
            value val = std::move(queue_.front());
            queue_.pop_front();
            return val;
        }

        protected:
        void enqueue(value &&val) {
            {
                std::lock_guard<std::mutex> lock(queue_lock_);
                auto                        strat = insertion_strategy<queue<value>>(queue_);
                strat = std::move(val);
            }
            cv_.notify_one();
        }
        friend class producer;
        friend struct mpsc;
    };

    struct producer {
        consumer *channel_;
        void      dispatch(value &&val) const { channel_->enqueue(std::move(val)); }
        void      operator()(value &&val) const { this->dispatch(std::move(val)); }
    };

    private:
    consumer consumer_ = consumer();

    public:
    consumer &rx() { return consumer_; }

    producer tx() const { return producer{ .channel_ = const_cast<consumer *>(&consumer_) }; }
};

// A simple implementation of a single-producer/multi-consumer
// (SP/MC) channel.
//
// This structure should be used sparingly, the implementation duplicates each message
// for each consumer, should the producer be faster than the consumer in processing time,
// the consumers will build up a large backlog that can bloat memory usage.
//
// Requirements: `value` has to implement both the move-/ and copy-constructor.
template<typename value>
struct spmc {
    struct producer;
    struct consumer {
        private:
        mutable std::deque<value>       queue_;
        mutable std::mutex              lock_;
        mutable std::condition_variable cv_;
        // Used to log-off
        producer *producer_;

        consumer(producer *producer)
          : producer_(producer) {
            producer_->adopt(*this);
        }
        friend struct spmc;

        public:
        ~consumer() { producer_->remove(this); }

        explicit consumer(const consumer &other) {
            queue_ = other.queue_;
            producer_ = other.producer_;
            producer_->adopt(*this);
        };

        explicit consumer(const consumer &&other)
          : producer_(other.producer_) {
            producer_->adopt(*this);
        }

        value wait() const {
            std::unique_lock<std::mutex> lock(lock_);
            cv_.wait(lock, [this] { return !queue_.empty(); });
            value val = std::move(queue_.front());
            queue_.pop_front();
            return val;
        }
        void enqueue(value &&val) {
            {
                std::lock_guard lock(lock_);
                queue_.emplace_front(val);
            }
            cv_.notify_one();
        }
    };

    struct producer {
        private:
        std::list<consumer *> consumers_;
        mutable std::mutex    lock_;
        friend struct spmc;

        public:
        producer() {}
        producer(const producer &) = delete;
        producer(const producer &&) = delete;

        void remove(consumer *sub) {
            std::lock_guard lock(lock_);
            auto            it = std::remove(consumers_.begin(), consumers_.end(), sub);
            consumers_.erase(it, consumers_.end());
        }

        void dispatch(value &&val) const {
            std::lock_guard lock(lock_);
            for (consumer *c : consumers_) {
                // We have to copy it since multiple consumers get it.
                c->enqueue(value(val));
            }
        }

        void operator()(value &&val) const { this->dispatch(std::move(val)); }

        void adopt(consumer &cons) {
            std::lock_guard lock(lock_);
            consumers_.push_back(&cons);
        }
    };

    private:
    producer *producer_ = new producer{};

    public:
    producer &tx() { return *producer_; }

    consumer rx() {
        consumer con = consumer(producer_);
        producer_->adopt(con);
        return consumer(std::move(con));
    }
};
}
