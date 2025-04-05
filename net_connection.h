// Created by psdab on 5/26/2024.

#ifndef NETWORKING_NET_CONNECTION_H
#define NETWORKING_NET_CONNECTION_H
#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace ps {
    namespace net {
        // forward declaration
        template <typename T>
        class server_interface;

        template <typename T>
        class connection : public std::enable_shared_from_this<connection<T>> {
        public:
            enum class owner {
                server,
                client
            };

            connection(owner parent, asio::io_context& context, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& in) :
            context(context), socket(std::move(socket)), messages_in(in) {
                OwnerType = parent;

                if (OwnerType == owner::server) {
                    HandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
                    HandshakeCheck = scramble(HandshakeOut);
                } else {
                    HandshakeIn = 0;
                    HandshakeOut = 0;
                }
            };

            virtual ~connection() {

            };

            uint32_t GetID() const {
                return id;
            }

            void ConnectToClient(ps::net::server_interface<T>* server, uint32_t id = 0) {
                if (OwnerType == owner::server) {
                    if (socket.is_open()) {
                        this->id = id;

                        // client has attempted to connect to the server,
                        // but we want the client to validate itself first,
                        // so we write out the handshake data to be validated
                        WriteValidation();

                        // create a task that sits and waits for asynchronously
                        // for the client to respond to the validation request
                        ReadValidation(server);
                    }
                }
            }

            void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
                if (OwnerType == owner::client) {
                    asio::async_connect(socket, endpoints, [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
                        if (!ec) {
                            ReadValidation();
                        }
                    });
                }
            };

            bool Disconnect() {
                if (IsConnected()) {
                    asio::post(context, [this]() { socket.close(); });
                }
            };

            bool IsConnected() const {
                return socket.is_open();
            };

            void Send(const message<T>& msg) {
                asio::post(context, [this, msg]() {
                    bool WritingMessage = !messages_out.empty();
                    messages_out.push_back(msg);
                    if (!WritingMessage) {
                        WriteHeader();
                    }
                });
            };
        private:
            // ASYNC - prime context to read a message header
            void ReadHeader() {
                asio::async_read(socket, asio::buffer(&tempMessageIn.header, sizeof(message_header<T>)), [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (tempMessageIn.header.size > 0) {
                            tempMessageIn.body.resize(tempMessageIn.header.size);
                            ReadBody();
                        } else {
                            AddToIncomingMessageQueue();
                        }
                    } else {
                        std::cout << "[" << id << "] read header fail\n";
                        socket.close();
                    }
                });
            }

            // ASYNC - prime context to read a message body
            void ReadBody() {
                asio::async_read(socket, asio::buffer(tempMessageIn.body.data(), tempMessageIn.body.size()), [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        AddToIncomingMessageQueue();
                    } else {
                        std::cout << "[" << id << "] read body fail\n";
                        socket.close();
                    }
                });
            }

            // ASYNC - prime context to write a message header
            void WriteHeader() {
                asio::async_write(socket, asio::buffer(&messages_out.front().header, sizeof(message_header<T>)), [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (messages_out.front().body.size() > 0) {
                            WriteBody();
                        } else {
                            messages_out.pop_front();

                            if (!messages_out.empty()) {
                                WriteHeader();
                            }
                        }
                    } else {
                        std::cout << "[" << id << "] write header fail\n";
                        socket.close();
                    }
                });
            }

            // ASYNC - prime context to write a message body
            void WriteBody() {
                asio::async_write(socket, asio::buffer(messages_out.front().body.data(), messages_out.front().body.size()), [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        messages_out.pop_front();

                        if (!messages_out.empty()) {
                            WriteHeader();
                        }
                    } else {
                        std::cout << "[" << id << "] write body fail\n";
                        socket.close();
                    }
                });
            }

            void AddToIncomingMessageQueue() {
                if (OwnerType == owner::server) {
                    messages_in.push_back({this->shared_from_this(), tempMessageIn});
                } else {
                    messages_in.push_back({nullptr, tempMessageIn});
                }

                ReadHeader();
            }

            uint64_t scramble(uint64_t input) {
                uint64_t out = input ^ 0xfadedbeefcafe;
                out = (out & 0xabcdef) >> 3 | (out & 0xfedcab) << 12;
                return out ^ 0xdeadfacade;
            }

            void WriteValidation() {
                asio::async_write(socket, asio::buffer(&HandshakeOut, sizeof(uint64_t)), [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (OwnerType == owner::client) {
                            ReadHeader();
                        }
                    } else {
                        socket.close();
                    }
                });
            }

            void ReadValidation(ps::net::server_interface<T>* server = nullptr) {
                asio::async_read(socket, asio::buffer(&HandshakeIn, sizeof(size_t)), [this, server](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (OwnerType == owner::server) {
                            if (HandshakeIn == HandshakeCheck) {
                                std::cout << "client validated\n";
                                server->OnClientValidated(this->shared_from_this());

                                ReadHeader();
                            } else {
                                std::cout << "client disconnected (fail validation)\n";
                            }
                        } else {
                            HandshakeOut = scramble(HandshakeIn);

                            WriteValidation();
                        }
                    } else {
                        std::cout << "client disconnected (read validation)\n";
                        socket.close();
                    }
                });
            }

        protected:
            // each connection unique socket to a remote
            asio::ip::tcp::socket socket;

            // this context is shared across the whole asio instance
            asio::io_context& context;

            // queue that holds all messages to be sent to the remote side of this connection
            tsqueue<message<T>> messages_out;

            // queue that holds all messages that have been received from the remote side of this connection.
            // this queue is a reference because the owner of this connection (client) is expected to provide a queue.
            tsqueue<owned_message<T>>& messages_in;

            // incoming messages are constructed asynchronously, so we will
            // store part of the assembled message here until its ready
            message<T> tempMessageIn;

            // the "owner" decides how some of the connection behaves
            owner OwnerType = owner::server;

            uint32_t id = 0;

            uint64_t HandshakeOut = 0;
            uint64_t HandshakeIn = 0;
            uint64_t HandshakeCheck = 0;
        };
    }
}

#endif