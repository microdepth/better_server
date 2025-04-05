// Created by psdab on 5/26/2024.

#ifndef NETWORKING_NET_CLIENT_H
#define NETWORKING_NET_CLIENT_H
#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace ps {
    namespace net {
        template  <typename T>
        class client_interface {
        public:
            client_interface() {
                // initialize the socket with the io context
            }

            virtual ~client_interface() {
                // if the client is being destroyed, disconnect it from the server
                Disconnect();
            }

            // connect to server with hostname/ip and port
            bool Connect(const std::string& host, const uint16_t port) {
                try {
                    // resolve hostname/ip into tangible physical address
                    asio::ip::tcp::resolver resolver(context);
                    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

                    // create connection
                    m_connection = std::make_unique<connection<T>>(connection<T>::owner::client, context, asio::ip::tcp::socket(context), messages_in);

                    // tell the connection object to connect to server
                    m_connection->ConnectToServer(endpoints);

                    // start context thread
                    context_thread = std::thread([this]() { context.run(); });

                } catch (std::exception& e) {
                    std::cerr << "client exception: " << e.what() << "\n";
                    return false;
                }
                return true;
            }

            // disconnect from server
            void Disconnect() {
                if (IsConnected()) {
                    m_connection->Disconnect();
                }

                context.stop();
                if (context_thread.joinable()) { context_thread.join(); }
                m_connection.release();
            }

            // check if the client is connected to the server
            bool IsConnected() {
                if (m_connection) {
                    return m_connection->IsConnected();
                } else {
                    return false;
                }
            }

            // send message to the server
            void Send(const message<T>& msg)
            {
                if (IsConnected()) {
                    m_connection->Send(msg);
                }
            }

            tsqueue<owned_message<T>>& Incoming() {
                return messages_in;
            }
        protected:
            // asio context handles the data transfer
            asio::io_context context;
            // thread for the context to execute work commands
            std::thread context_thread;
            // the client has a single instance of a "connection" object which handles data transfer
            std::unique_ptr<connection<T>> m_connection;
        private:
            // tsq for incoming messages
            tsqueue<owned_message<T>> messages_in;
        };
    }
}

#endif
