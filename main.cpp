#include "ps_net.h"
#include <iostream>
using namespace std;

enum class CustomMsgTypes : uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
    ChatMessageAll
};

class CustomServer : public ps::net::server_interface<CustomMsgTypes> {
public:
    CustomServer(uint16_t port) : ps::net::server_interface<CustomMsgTypes>(port) {

    }
protected:
    bool OnClientConnect(std::shared_ptr<ps::net::connection<CustomMsgTypes>> client) override {
        ps::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerAccept;
        client->Send(msg);
        return true;
    }

    // called when client appears to have disconnected
    void OnClientDisconnect(std::shared_ptr<ps::net::connection<CustomMsgTypes>> client) override {
        std::cout << "removing client [" << client->GetID() << "]\n";
    }

    // called when a message arrives
    void OnMessage(std::shared_ptr<ps::net::connection<CustomMsgTypes>> client, ps::net::message<CustomMsgTypes>& msg) override {
        switch (msg.header.id) {
            case CustomMsgTypes::ServerPing: {
                std::cout << "[" << client->GetID() << "]: server ping\n";
                client->Send(msg);
                cout << "sent\n";
            } break;
            case CustomMsgTypes::MessageAll: {
                std::cout << "[" << client->GetID() << "]: message all\n";
                ps::net::message<CustomMsgTypes> out_msg;
                out_msg.header.id = CustomMsgTypes::ServerMessage;
                out_msg << client->GetID();
                MessageAllClients(out_msg, client);
            } break;
            case CustomMsgTypes::ChatMessageAll: {
                MessageAllClients(msg);
            } break;
        }
    }
private:

};

int main() {
    CustomServer server(60000);
    server.Start();

    while (1) {
        server.Update(std::numeric_limits<size_t>::max(), true);
    }

    return 0;
}