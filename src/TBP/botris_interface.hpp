#pragma once


#include "./../Util/json.hpp"

#include "Constants.hpp"
#include "Board.hpp"

inline auto piece_type_to_json(PieceType type) -> nlohmann::json {
	switch (type) {
	case PieceType::S:
		return "S";
	case PieceType::Z:
		return "Z";
	case PieceType::J:
		return "J";
	case PieceType::L:
		return "L";
	case PieceType::T:
		return "T";
	case PieceType::O:
		return "O";
	case PieceType::I:
		return "I";
	case PieceType::Empty:
		return "I";
	default:
		return nullptr;
	}
}

inline auto color_to_json(ColorType type) -> nlohmann::json {
	switch (type) {
	case ColorType::S:
		return "S";
	case ColorType::Z:
		return "Z";
	case ColorType::J:
		return "J";
	case ColorType::L:
		return "L";
	case ColorType::T:
		return "T";
	case ColorType::O:
		return "O";
	case ColorType::I:
		return "I";
	case ColorType::Empty:
		return nullptr;
	case ColorType::Garbage:
		return "G";
	default:
		return nullptr;
	}
}

inline auto string_to_piece_type(const std::string& str) -> PieceType {
	if (str == "S") {
		return PieceType::S;
	}
	else if (str == "Z") {
		return PieceType::Z;
	}
	else if (str == "J") {
		return PieceType::J;
	}
	else if (str == "L") {
		return PieceType::L;
	}
	else if (str == "T") {
		return PieceType::T;
	}
	else if (str == "O") {
		return PieceType::O;
	}
	else if (str == "I") {
		return PieceType::I;
	}
	else {
		throw std::runtime_error("non existent piece type");
	}
}

inline auto json_to_color(const nlohmann::json& js) -> ColorType {
	if(js.is_null()) {
		return ColorType::Empty;
	}
	std::string str = js.get<std::string>();
	switch (str[0])
	{
	case 'S':
		return ColorType::S;
	case 'Z':
		return ColorType::Z;
	case 'J':
		return ColorType::J;
	case 'L':
		return ColorType::L;
	case 'T':
		return ColorType::T;
	case 'O':
		return ColorType::O;
	case 'I':
		return ColorType::I;
	case 'G':
		return ColorType::Garbage;
	default:
		throw std::runtime_error("out of bounds color type");
		break;
	}
}


inline auto json_to_board(nlohmann::json json) -> Board {
	Board board;
	for (int y = 0; y < json.size() && y < Board::height; ++y) {
		for (int x = 0; x < json[0].size(); ++x) {
			if (!json[y][x].is_null()) {
				board.set(x, y);
			}
		}
	}
	return board;
};

inline auto json_to_piece_type(nlohmann::json json) -> Piece {
	Piece piece(PieceType::Empty);
	piece.type = string_to_piece_type(json["type"].get<std::string>());
	piece.rotation = (RotationDirection)json["rotation"].get<int>();
	piece.position = { (int8_t)json["x"].get<int>(), (int8_t)json["y"].get<int>() };
	return piece;
};


inline auto int_to_rotation(int num) -> RotationDirection {
	switch (num)
	{
	case 0:
		return RotationDirection::North;
	case 1:
		return RotationDirection::East;
	case 2:
		return RotationDirection::South;
	case 3:
		return RotationDirection::West;
	default:
		throw std::runtime_error("out of bounds rotation");
		break;
	}

}


