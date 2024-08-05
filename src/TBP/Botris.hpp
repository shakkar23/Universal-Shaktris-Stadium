#pragma once

#include <optional>
#include <vector>
#include "./../Util/json.hpp"

#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXUserAgent.h>

#include <memory>

// this class will just store a bunch of functions that will be used by the website
// it will append the json notifications to a vector of json objects in the class
class Botris
{
public:
	Botris();

	~Botris();

    void incoming_data(nlohmann::json data);

    void send_data(const nlohmann::json& data);

	std::optional<nlohmann::json> get_update();

private:
	std::vector<nlohmann::json> updates;
	std::unique_ptr<ix::WebSocket> web_socket;
	std::mutex updates_mutex;
};

