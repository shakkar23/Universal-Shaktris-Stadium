#include "Botris.hpp"



#include <iostream>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>




Botris::Botris() {
	ix::initNetSystem();
    std::string API_TOKEN = std::getenv("BOTRIS_API_TOKEN");
    std::string ROOM_KEY = "cw0edbx81ieo17mrh2akdx6e";
    std::string url("wss://botrisbattle.com/ws?token=" + API_TOKEN + "&roomKey=" + ROOM_KEY);

    web_socket = std::make_unique<ix::WebSocket>();

    
    web_socket->setUrl(url);
    web_socket->setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message)
            this->incoming_data(nlohmann::json::parse(msg->str));
        else if (msg->type == ix::WebSocketMessageType::Open) {
        }
        else if (msg->type == ix::WebSocketMessageType::Error) {
            std::cerr << "Websocket Error: " << msg->errorInfo.reason << std::endl;
        }

        });
    web_socket->start();
}

Botris::~Botris() {}

void Botris::incoming_data(nlohmann::json data) {
    std::lock_guard<std::mutex> lock(updates_mutex);
	updates.push_back(data);
}


void Botris::send_data(const nlohmann::json &data) {
    if(web_socket->getReadyState() == ix::ReadyState::Open)
        web_socket->send(data.dump());
}

std::optional<nlohmann::json> Botris::get_update()
{
    std::lock_guard<std::mutex> lock(updates_mutex);
	if(updates.empty())
		return std::nullopt;
	nlohmann::json data = updates.front();
	updates.erase(updates.begin());
	return data;
}
