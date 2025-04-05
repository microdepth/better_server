// Created by psdab on 5/26/2024.

#ifndef NETWORKING_NET_SERVER_H
#define NETWORKING_NET_SERVER_H
#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace ps {
    namespace net {
        template <typename T>
        class server_interface {
        public:
            server_interface(uint16_t port) : asio_acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {

            }

            virtual ~server_interface() {
                Stop();
            }

            bool Start() {
                try {
                    // add work to the context before telling it to run in another thread
                    WaitForClientConnection();
                    // run is needed to start running all work that we primed
                    context_thread = std::thread([this]() { context.run(); });
                } catch (std::exception& e) {
                    // something prohibited the server from listening
                    std::cerr << "[SERVER] exception: " << e.what() << "\n";
                    return false;
                }

                std::cout << "[SERVER] started!\n";
                return true;
            }
            bool Stop() {
                // request the context to close
                context.stop();

                // tidy up the context thread
                if (context_thread.joinable()) { context_thread.join(); }
                std::cout << "[SERVER] stopped!\n";
            }

            // async - instruct asio to wait for connection
            void WaitForClientConnection() {
                asio_acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
                    if (!ec) {
                        std::cout << "[SERVER] new connection: " << socket.remote_endpoint() << "\n";

                        std::shared_ptr<connection<T>> newconn =
                                std::make_shared<connection<T>>(connection<T>::owner::server,
                                        context, std::move(socket), messages_in);

                        // give the user a chance to deny connection
                        if (OnClientConnect(newconn)) {
                            // connection allowed, so add to container of new connections
                            deqConnections.push_back(std::move(newconn));

                            deqConnections.back()->ConnectToClient(this, id_counter++);

                            std::cout << "[" << deqConnections.back()->GetID() << "] connection approved\n";
                        } else {
                            std::cout << "[-----] connection denied\n";
                        }
                    } else {
                        // error has occured during acceptance
                        std::cout << "[SERVER] new connection error: " << ec.message() << "\n";
                    }

                    // prime the asio context with more work - waiting for another connection
                    WaitForClientConnection();
                });
            }

            // send a message to a specific client
            void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg) {
                if (client && client->IsConnected()) {
                    client->Send(msg);
                } else {
                    OnClientDisconnect(client);
                    client.reset();
                    deqConnections.erase(
                            std::remove(deqConnections.begin(), deqConnections.end(), client),
                            deqConnections.end());
                }
            }

            // send a message to all clients
            void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> ignore_client = nullptr) {
                bool InvalidClientExists = false;

                for (auto& client : deqConnections) {
                    // check if client exists
                    if (client && client->IsConnected()) {
                        if (client != ignore_client) { client->Send(msg); }
                    } else {
                        // the client couldn't be contacted, so assume it has disconnected
                        OnClientDisconnect(client);
                        client.reset();
                        InvalidClientExists = true;
                    }
                }

                // apparently it's bad to modify a container while using iterators to move through it,
                // so we set a flag and edit accordingly
                if (InvalidClientExists) {
                    // remove takes unwanted element out of the container
                    // and returns a container with empty spaces between elements extracted and moved to the end of the container,
                    // where erase shaves the empty spaces off so that memory isn't wasted
                    deqConnections.erase(
                            std::remove(deqConnections.begin(), deqConnections.end(), nullptr),
                             deqConnections.end());
                }
            }

            void Update(size_t MaxMessages = std::numeric_limits<size_t>::max(), bool wait = false) {
                if (wait) { messages_in.wait(); }

                size_t MessageCount = 0;
                while ((MessageCount < MaxMessages) && !messages_in.empty()) {
                    // grab the latest message
                    auto msg = messages_in.pop_front();

                    // pass to message handler
                    OnMessage(msg.remote, msg.msg);

                    MessageCount++;
                }
            }

            // called when a client is validated
            virtual void OnClientValidated(std::shared_ptr<connection<T>> client) {

            }
        protected:
            // called when a client connects, you can veto the connection by returning false
            virtual bool OnClientConnect(std::shared_ptr<connection<T>> client) {
                return false;
            }

            // called when a client appears to have disconnected
            virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client) {

            }

            // called when a message arrives
            virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg) {

            }

            // tsq for incoming message packets
            tsqueue<owned_message<T>> messages_in;

            // container of active validated connections
            std::deque<std::shared_ptr<connection<T>>> deqConnections;

            // context and thread to run it in
            asio::io_context context;
            std::thread context_thread;

            // the acceptor waits for connections to accept
            asio::ip::tcp::acceptor asio_acceptor;

            // clients will be identified in the system via an id
            uint32_t id_counter = 10000;
        private:

        };

    }
}

#endif
