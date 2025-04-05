// Created by psdab on 5/2/2024.

#ifndef BETTER_SERVER_NET_MESSAGE_H
#define BETTER_SERVER_NET_MESSAGE_H
#pragma once

#include "net_common.h"

namespace ps {
    namespace net {
        template <typename T>
        struct message_header {
            T id{};
            uint32_t size = 0; // size of the message
        };

        template <typename T>
        struct message {
            message_header<T> header{};
            std::vector<uint8_t> body;

            // returns the size of the message in bytes
            size_t size() const {
                return body.size();
            }

            // override for std::cout compatibility
            friend std::ostream& operator<<(std::ostream& os, const message<T> msg) {
                os << "id: " << int(msg.header.id) << "size: " << msg.header.size;
                return os;
            }

            // operator overloads for dealing with strings in messages
            template <typename Datatype>
            friend ps::net::message<Datatype>& operator<<(ps::net::message<Datatype>& msg, const std::string& str) {
                size_t stringSize = str.size();
                size_t originalSize = msg.size();

                msg.body.resize(originalSize + stringSize);
                std::memcpy(msg.body.data() + originalSize, str.data(), stringSize);

                msg << stringSize;

                msg.header.size = msg.size();
                return msg;
            };
            template <typename Datatype>
            friend ps::net::message<Datatype>& operator>>(ps::net::message<Datatype>& msg, std::string& str) {
                size_t stringSize;
                msg >> stringSize;

                size_t start = msg.body.size() - stringSize;
                str.resize(stringSize);
                std::memcpy(str.data(), msg.body.data() + start, stringSize);

                msg.body.resize(start);
                msg.header.size = msg.size();
                return msg;
            }

            // operator overloads for dealing with standard layout data types in messages
            template <typename Datatype, typename = std::enable_if_t<!std::is_same_v<Datatype, std::string>>>
            friend message<T>& operator<<(message<T>& msg, const Datatype& data) {
                // check that the type of data being pushed is trivially copyable
                static_assert(std::is_standard_layout<Datatype>::value, "data is too complex to be pushed into vector");

                // cache the current size of the body vector
                size_t i = msg.body.size();

                // resize the vector by the size of the data being pushed
                msg.body.resize(msg.body.size() + sizeof(Datatype));

                // copy the data into the newly allocated vector space
                std::memcpy(msg.body.data() + i, &data, sizeof(Datatype));

                // re-evaluate message size
                msg.header.size = msg.size();

                // return the target message so it can be chained
                return msg;
            }
            template <typename Datatype, typename = std::enable_if_t<!std::is_same_v<Datatype, std::string>>>
            friend message<T>& operator>>(message<T>& msg, Datatype& data) {
                // check that the type of the data being pulled is trivially copyable
                static_assert(std::is_standard_layout<Datatype>::value, "data is too complex to be pushed into vector");

                // cache the location towards the end of the vector where the pulled data starts
                size_t i = msg.body.size() - sizeof(Datatype);

                // copy data from the vector into the data variable
                std::memcpy(&data, msg.body.data() + i, sizeof(Datatype));

                // shrink the vector to remove read bytes, and reset end position
                msg.body.resize(i);

                // re-evaluate message size
                msg.header.size = msg.size();

                // return the target message so it can be chained
                return msg;
            }
        };

        template <typename T>
        class connection;

        template <typename T>
        struct owned_message {
            std::shared_ptr<connection<T>> remote = nullptr;
            message<T> msg;

            // override for std::cout compatibility
            friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg) {
                os << msg.msg;
                return os;
            };
        };
    }
}

#endif