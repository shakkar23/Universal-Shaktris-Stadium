
#include "BotrisVisualizer.hpp"


bool BotrisVisualizer::update(const Shakkar::inputs& input) {
	try {
		std::string path = input.getDroppedFile();

		if (!path.empty()) {
			local_bot.start(path.c_str());
		}

		while (const auto server_update = server.get_update()) {

			auto type = server_update.value()["type"].get<std::string>();

			if (type == "authenticated") {
				session_id = server_update.value()["payload"]["sessionId"].get<std::string>();
			}


			if (!local_bot.is_running())
				continue; // to exaust the server updates

			auto json = server_update.value();
			if (type == "request_move") {
				std::cout << "BOTRIS request_move\n";
				/*
	{
		type: "request_move";
		payload: {
			gameState: GameState;
			players: PlayerData[];
		};
	}
				*/

				// get the path to the piece and then update the local board with the move given by the bot
				nlohmann::json message;

				message["type"] = "action";

				local_bot.TBP_suggest();
				auto moves = local_bot.TBP_suggestion();


				// get the first move and find the path to it
				if (moves.size() == 0) {
					// this should never happen
					std::cout << "No moves found from the bot provided\n";
					throw std::runtime_error("No moves found from the bot provided");
				}

				static bool first_hold = false;
				static bool first_hold_last_time = false;

				first_hold_last_time = first_hold;

				Piece move = moves.front();
				game.sonic_drop(game.board, move);
				std::cout << "Move: " << piece_type_to_json(move.type).get<std::string>() << " " << (int)move.position.x << " " << (int)move.position.y << std::endl;
				bool is_hold = game.current_piece.type != move.type;

				// error checking
				if (move.type != game.current_piece.type && move.type != game.queue.front())
				{
					if (game.hold.has_value()) {
						if (move.type != game.hold.value().type) {
							// print the first piece in the queue and the hold piece and the current piece
							std::cout << "Current piece: " << piece_type_to_json(game.current_piece.type).get<std::string>() << std::endl;
							std::cout << "Hold piece: " << piece_type_to_json(game.hold.value().type).get<std::string>() << std::endl;
							std::cout << "Queue piece: " << piece_type_to_json(game.queue.front()).get<std::string>() << std::endl;
							throw std::runtime_error("Move piece is not the same as the suggested piece");
						}
					}
					else {
						// print the first piece in the queue and the hold piece and the current piece
						std::cout << "Current piece: " << piece_type_to_json(game.current_piece.type).get<std::string>() << std::endl;
						std::cout << "Queue piece: " << piece_type_to_json(game.queue.front()).get<std::string>() << std::endl;
						throw std::runtime_error("Move piece is not the same as the suggested piece");
					}
				}

				Path path = pathfind(game.board, move);

				// presumably we are going to die soon so its fine that we just continue here
				if(path.size() == 0) {
					std::cout << "No path found for the move" << std::endl;
					continue;
					//throw std::runtime_error("No path found for the move");
				}

				first_hold = game.place_piece(move);
				game.board.clearLines();



				// update the game with more pieces from the queue
				{
					nlohmann::json queue = json["payload"]["gameState"]["queue"];
					std::vector<PieceType> queue_pieces;
					for (auto& piece : queue) {
						PieceType type = string_to_piece_type(piece.get<std::string>());
						queue_pieces.push_back(type);
					}

					if (first_hold_last_time)
					{
						auto it = std::find(game.queue.begin(), game.queue.end(), PieceType::Empty);
						if (it != game.queue.end())
							*it = queue_pieces[game.queue.size() - 1];

						local_bot.TBP_new_piece(queue_pieces[game.queue.size() - 1]);
					}

					auto it = std::find(game.queue.begin(), game.queue.end(), PieceType::Empty);
					if (it != game.queue.end())
						*it = queue_pieces[game.queue.size()];
					local_bot.TBP_new_piece(queue_pieces[game.queue.size()]);
				}
				local_bot.TBP_play(move); // assume the piece placed and everything is correct :)


				message["payload"]["commands"] = nlohmann::json::array();

				if (is_hold) {
					message["payload"]["commands"].push_back("hold");
				}

				for (auto& movement : path) {
					switch (movement) {
					case Movement::Left:
						message["payload"]["commands"].push_back("move_left");
						break;
					case Movement::Right:
						message["payload"]["commands"].push_back("move_right");
						break;
					case Movement::RotateClockwise:
						message["payload"]["commands"].push_back("rotate_cw");
						break;
					case Movement::RotateCounterClockwise:
						message["payload"]["commands"].push_back("rotate_ccw");
						break;
					case Movement::SonicDrop:
						message["payload"]["commands"].push_back("sonic_drop");
						break;
					}
				}
				// send the message to the server about our move that we want to do
				std::cout << "Sending message: " << message << std::endl;
				server.send_data(message);

			}
			else if (type == "round_started") {
				std::cout << "BOTRIS round_started\n";
				/*
	{
		type: 'round_started';
		payload: {
			startsAt: number;
			roomData: RoomData;
		}
	}

	RoomData:
	{
		id: string;
		host: PlayerInfo;
		private: boolean;
		ft: number;
		initialPps: number;
		finalPps: number;
		startMargin: number;
		endMargin: number;
		maxPlayers: number;
		gameOngoing: boolean;
		roundOngoing: boolean;
		startedAt: number | null;
		endedAt: number | null;
		lastWinner: SessionId | null;
		players: PlayerData[];
		banned: PlayerInfo[];
	}

	PlayerData:
	{
		sessionId: SessionId;
		playing: boolean;
		info: PlayerInfo;
		wins: number;
		gameState: GameState | null;
	}

	PlayerInfo:
	{
		userId: string;
		creator: string;
		bot: string;
	}

	GameState:
	{
		board: Block[][];
		queue: Piece[];
		garbageQueued: number;
		held: Piece | null;
		current: PieceData;
		isImmobile: boolean;
		canHold: boolean;
		combo: number;
		b2b: boolean;
		score: number;
		piecesPlaced: number;
		dead: boolean;
	}

	PieceData:
	{
		piece: Piece;
		x: number;
		y: number;
		rotation: 0 | 1 | 2 | 3;


	Piece:
	'I' | 'O' | 'J' | 'L' | 'S' | 'Z' | 'T'

	}*/

				// get the queue, the board will be empty

				nlohmann::json payload = json["payload"];
				nlohmann::json roomData = payload["roomData"];
				nlohmann::json players = roomData["players"];
				// find the player named "Nana"
				for (auto& player : players) {

					auto sessionId = player["sessionId"].get<std::string>();
					// get session id

					if (sessionId == session_id) {
						// get queue
						auto gameState = player["gameState"];

						auto queue = gameState["queue"];

						std::vector<PieceType> queue_pieces;
						for (auto& piece : queue) {
							PieceType type = string_to_piece_type(piece.get<std::string>());

							queue_pieces.push_back(type);
						}
						PieceType type = string_to_piece_type(gameState["current"]["piece"].get<std::string>());
						queue_pieces.insert(queue_pieces.begin(), type);
						std::optional<PieceType> hold = std::nullopt;
						if (!gameState["held"].is_null()) {
							PieceType hold_type = string_to_piece_type(gameState["held"]["type"].get<std::string>());
							hold = hold_type;
						}
						int combo = gameState["combo"].get<int>();
						bool back_to_back = gameState["b2b"].get<bool>();

						game = Game();

						game.board = Board();

						std::copy(queue_pieces.begin() + 1, queue_pieces.begin() + 1 + game.queue.size(), game.queue.begin());
						queue_pieces.resize(game.queue.size() + 1);

						game.hold = hold;
						game.current_piece = type;
						game.stats.combo = combo;
						game.stats.b2b = back_to_back;
						game.garbage_meter = 0;

						local_bot.TBP_start(game.board, queue_pieces, game.hold, game.stats.b2b, game.stats.combo);


						break;
					}
				}

			}
			else if (type == "player_action") {

				// not our event, ignore it
				if (json["payload"]["sessionId"].get<std::string>() != session_id) {
					continue;
				}


				// check if any of the events have a damage_tanked event
				nlohmann::json damage_tanked_event;
				for (const auto& event : json["payload"]["events"]) {
					if (event["type"].get<std::string>() == "damage_tanked") {
						damage_tanked_event = event;
						break;
					}
				}
				// no damage_tanked event, ignore the event
				if (damage_tanked_event.is_null())
					continue;

				std::cout << "BOTRIS garbage tanked\n";

				// update the bot with the new board and game state
				// the update could be for the opponent so only update if the session id matches
				nlohmann::json index_arr = damage_tanked_event["payload"]["holeIndices"];


				// for every index add one garbage line on said index
				for (auto& index : index_arr) {
					int idx = index.get<int>();
					game.add_garbage(1, idx);
				}


				std::vector<PieceType> queue_pieces;
				// make queue from current piece and queue in game
				queue_pieces.push_back(game.current_piece.type);
				queue_pieces.insert(queue_pieces.end(), game.queue.begin(), game.queue.end());


				local_bot.TBP_start(game.board, queue_pieces, game.hold, game.stats.b2b, game.stats.combo);

			}
			else if (type == "round_over") {
				std::cout << "BOTRIS round_over\n";

				local_bot.TBP_stop();
			}
		}
		gui.game = game;
	}
	catch (const std::exception& e) {
		std::cout << "\033[1;31m";
		std::cerr << "[Error]: " << e.what() << std::endl;
		std::cout << "\033[0m";
		return false;
	}
	return true;
}






// in house pathfinding algorithm for the bot without the Game class, bad decision move to somewhere else
BotrisVisualizer::Path BotrisVisualizer::pathfind(const Board& board, const Piece& piece) {

	struct FullPiece {
		Piece piece;
		Path path;
	};
	Game game;
	game.board = board;


	std::vector<FullPiece> moves;
	std::vector<bool> visited = std::vector<bool>(6444);

	std::vector<FullPiece> open_nodes;
	open_nodes.reserve(150);
	std::vector<FullPiece> next_nodes;
	next_nodes.reserve(150);

	{
		Piece initial_piece = piece.type;
		open_nodes.push_back({ initial_piece, {} });
		moves.push_back({ initial_piece, {} });
		visited[initial_piece.compact_hash()] = true;
	}

	// limiting the depth of the search to inf but can be increased later on if needed
	while (open_nodes.size() != 0) {
		for (const auto& node : open_nodes) {
			for (size_t i = 0; i < 5; i++) {

				Path tmp_history = node.path;
				tmp_history.push_back((Movement)i);

				// TODO allow inputs to be done simultaneously
				Piece tmp = node.piece;
				game.process_movement(tmp, (Movement)i);
				const FullPiece newFullPiece = { tmp, tmp_history };

				const auto tmp_hash = newFullPiece.piece.compact_hash();

				// havent found the hash, new state!
				if (!visited[tmp_hash]) {
					// add the new hash to the state
					visited[tmp_hash] = true;

					moves.push_back(newFullPiece);

					next_nodes.push_back(newFullPiece);
				}
			}
		}
		open_nodes = next_nodes;
		next_nodes.clear();
	}

	// find the move that matches the piece parameter 
	for (auto& move : moves) {
		if (move.piece.compact_hash() == piece.compact_hash()) {
			return  move.path;
		}
	}

	return {};
}

