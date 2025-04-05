// Created by psdab on 5/26/2024.

#ifndef NETWORKING_NET_TSQUEUE_H
#define NETWORKING_NET_TSQUEUE_H
#pragma once

#include "net_common.h"

namespace ps {
    namespace net {
        template <typename T>
        class tsqueue {
        public:
            tsqueue() = default;
            tsqueue(const tsqueue<T>()) = delete;
            virtual ~tsqueue() { clear(); };

            // returns reference to first element in queue
            const T& front() {
                std::scoped_lock lock(muxQueue);
                return deqQueue.front();
            }

            // returns reference to last element in queue
            const T& back() {
                std::scoped_lock lock(muxQueue);
                return deqQueue.back();
            }

            // adds element after the last element in the queue
            void push_back(const T& item) {
                std::scoped_lock lock(muxQueue);
                deqQueue.emplace_back(std::move(item));

                std::unique_lock<std::mutex> ul(muxBlocking);
                cvBlocking.notify_one();
            }

            // adds element after the last element in the queue
            void push_front(const T& item) {
                std::scoped_lock lock(muxQueue);
                deqQueue.emplace_front(std::move(item));

                std::unique_lock<std::mutex> ul(muxBlocking);
                cvBlocking.notify_one();
            }

            // returns true if queue is empty
            bool empty() {
                std::scoped_lock lock(muxQueue);
                return deqQueue.empty();
            }

            // returns the amount of elements in the queue
            size_t count() {
                std::scoped_lock lock(muxQueue);
                return deqQueue.count();
            };

            // clears the whole queue
            void clear() {
                std::scoped_lock lock(muxQueue);
                deqQueue.clear();
            }

            // removes and returns reference to first element in queue
            T pop_front() {
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.front());
                deqQueue.pop_front();
                return t;
            }

            // removes and returns reference to last element in queue
            T pop_back() {
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.back());
                deqQueue.pop_back();
                return t;
            }

            // block the thread until notified that an item has been pushed in to the queue
            void wait() {
                while (empty()) {
                    std::unique_lock<std::mutex> ul(muxBlocking);
                    cvBlocking.wait(ul);
                }
            }

        protected:
            std::mutex muxQueue;
            std::deque<T> deqQueue;
            std::condition_variable cvBlocking;
            std::mutex muxBlocking;
        };
    }
}

#endif
