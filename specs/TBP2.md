# Overview

This document specifies the Tetris Bot Protocol version 2 (TBP2), which is the default protocol used by this repository. TBP2 builds directly upon the original Tetris Bot Protocol MVP specification (TBP1) while maintaining its **stateful** architecture.

In TBP2, the bot maintains its own internal game state across turns, receiving incremental move and queue updates from the frontend. TBP2 extends TBP1 primarily to support competitive multi-player and versus play by providing a list of opponent boards and states, introducing garbage meter tracking, and changing a single field type (`back_to_back` from boolean to integer).

> **Note on SBP (Stateless Bot Protocol):** The Stateless Bot Protocol (SBP), documented separately in [`StatelessBotProtocol.md`](./StatelessBotProtocol.md), is an alternative protocol rather than a superset of TBP2. While SBP uses a fairly similar JSON schema, SBP is explicitly *stateless* (sending the entire game state via `play` on every turn and omitting `start`, `suggest`, `new_piece`, and `stop`). TBP2 remains stateful by default.

A TBP2 bot is generally a separate process that sends and receives JSON messages to communicate with a frontend. On most platforms, a bot is a separate executable program that a frontend executes and communicates with over standard input and output. On the web, a bot is a script that the frontend can launch as a Web Worker and communicate with using `postMessage`.

The typical lifecycle of a TBP2 bot is:

1. Bot sends `info`
1. Frontend sends `rules`
1. Bot sends `ready`
1. Frontend sends `start` (including initial board, queue, hold, meter, and opponent boards)
1. Frontend sends `suggest`
1. Bot sends `suggestion`
1. Frontend sends `play` (with the executed move and updated opponent boards)
1. Frontend sends `new_piece`
1. Go to step 5 until the game is over
1. Frontend sends `stop`
1. Go to step 4 for the next game
1. Frontend sends `quit`

# Reference

A bot conforming to TBP2 exists as a standalone executable that is executed by the frontend and communicates over standard input and output. Each message is JSON terminated by a line break (`\n`). JSON is chosen as the data format because it is ubiquitous, human-readable, and easily allows the protocol to be extended in the future.

On the web, a conforming bot is a script that can be executed as a Web Worker and communicates using JavaScript objects via the `postMessage` API.

Messages consist of an object with a `type` attribute indicating its type. Messages with an unrecognized type should be ignored. Unrecognized attributes should also be ignored.

---

## Messages (Frontend -> Bot)

### `rules`

The `rules` message tells the bot what game rules are in place. The bot must respond with either the `ready` message (if the bot can play with those rules) or `error` (if it cannot). The standard guideline rules are a loose default: 10×40 board, pieces spawn in the North orientation at x=4, roughly y=19, hold is allowed, and the Super Rotation System (SRS) is used.

Attribute   | Type    | Description
---------   | ----    | -----------
`min_nodes` | Integer | *(Optional)* If the bot advertises the `"set_min_nodes"` feature, specifies the search node budget or minimum nodes evaluated per move.

Example:
```json
{
  "type": "rules",
  "min_nodes": 2000
}
```

### `start`

The `start` message tells the bot to begin calculating from the specified position. This message must be sent before asking the bot for a move.

Compared to TBP1, TBP2 adds the `meter` field and the `opponents` array, and changes `back_to_back` from a boolean to an integer.

Attribute       | Type             | Description
---------       | ----             | -----------
`hold`          | String or `null` | Either a piece if hold is filled (e.g. `"J"`), or `null` if hold is empty.
`queue`         | Array of Strings | A list of upcoming pieces. Example: `["S", "Z", "O", "T", "L", "I"]`.
`combo`         | Integer          | The number of consecutive line clears that have been made.
`back_to_back`  | Integer          | The current Back-to-Back streak count or level (0 if inactive, 1+ if active). *(Changed from boolean in TBP1).*
`board`         | Array of Arrays  | A list of 40 lists of 10 board cells representing the matrix from bottom (y=0) to top (y=39).
`meter`         | Integer          | The amount of pending incoming garbage in the player's garbage meter. *(Added in TBP2).*
`opponents`     | Array of Objects | A list of opponent player states in the match, each containing the opponent's board and state. *(Added in TBP2).*

A board cell can be either `null` to indicate that it is empty, or a string indicating which piece was used to fill it (`"I"`, `"O"`, `"T"`, `"L"`, `"J"`, `"S"`, `"Z"`), or `"G"` for garbage cells.

Each opponent object in `opponents` contains:

Attribute       | Type             | Description
---------       | ----             | -----------
`board`         | Array of Arrays  | The 40×10 board matrix of the opponent.
`combo`         | Integer          | *(Optional)* The opponent's combo count.
`back_to_back`  | Integer          | *(Optional)* The opponent's back-to-back streak count.
`meter`         | Integer          | *(Optional)* The opponent's garbage meter.
`queue`         | Array of Strings | *(Optional)* The opponent's upcoming piece queue.
`hold`          | String or `null` | *(Optional)* The opponent's held piece.

Example:
```json
{
  "type": "start",
  "hold": null,
  "queue": ["T", "S", "Z", "J", "L", "O", "I"],
  "combo": 0,
  "back_to_back": 0,
  "board": [
    [null, null, null, null, null, null, null, null, null, null],
    ...
  ],
  "meter": 0,
  "opponents": [
    {
      "board": [
        [null, null, null, null, null, null, null, null, null, null],
        ...
      ],
      "combo": 0,
      "back_to_back": 0,
      "meter": 0,
      "queue": ["Z", "S", "T", "L", "O", "I", "J"]
    }
  ]
}
```

### `suggest`

The `suggest` message tells the bot to suggest moves in order of preference. The bot responds with a `suggestion` message.

If the bot advertises the `"skip_suggest"` feature, the frontend may omit this message, as the bot automatically produces a suggestion upon receiving a state update.

Example:
```json
{
  "type": "suggest"
}
```

### `play`

The `play` message tells the bot that a move was played. It is only valid to send this message while the bot is calculating. The bot updates its internal state according to the executed move and continues calculating from that updated position.

In TBP2, `play` also includes the `opponents` array so that the bot stays informed of opponent board changes and attacks during ongoing play.

Attribute   | Type             | Description
---------   | ----             | -----------
`move`      | Object           | The move that was executed.
`opponents` | Array of Objects | An updated list of opponent states (with their latest board matrices and meters). *(Added in TBP2).*

A `move` is an object with the following attributes:

Attribute  | Type   | Description
---------  | ----   | -----------
`location` | Object | A piece location specifying where the piece should be placed.
`spin`     | String | One of `"none"`, `"mini"`, or `"full"` indicating whether a spin was performed.

A piece location is an object with the following attributes:

Attribute     | Type    | Description
---------     | ----    | -----------
`type`        | String  | The type of piece this is (e.g. `"I"`, `"T"`, `"L"`).
`orientation` | String  | The rotation state of the piece: `"north"`, `"east"`, `"south"`, or `"west"`.
`x`           | Integer | The x coordinate of the center of the piece (0 is leftmost column).
`y`           | Integer | The y coordinate of the center of the piece (0 is bottommost row).

Whether a hold was performed is inferred by the bot from the piece type of the move.

Example:
```json
{
  "type": "play",
  "move": {
    "location": {
      "type": "T",
      "orientation": "north",
      "x": 4,
      "y": 0
    },
    "spin": "none"
  },
  "opponents": [
    {
      "board": [
        [null, null, null, null, null, null, null, null, null, null],
        ...
      ],
      "combo": 0,
      "back_to_back": 0,
      "meter": 2,
      "queue": ["S", "T", "L", "O", "I", "J", "Z"]
    }
  ]
}
```

### `new_piece`

The `new_piece` message informs the bot that a new piece has arrived at the end of the preview queue. This message is sent after a `play` message (or upon the initial hold).

Attribute | Type   | Description
--------- | ----   | -----------
`piece`   | String | The piece type of the new piece added to the queue (e.g. `"I"`).

Example:
```json
{
  "type": "new_piece",
  "piece": "I"
}
```

### `stop`

The `stop` message tells the bot to stop calculating.

Example:
```json
{
  "type": "stop"
}
```

### `quit`

The `quit` message tells the bot to exit.

Example:
```json
{
  "type": "quit"
}
```

---

## Messages (Bot -> Frontend)

### `info`

The `info` message must be sent by the bot immediately after it is launched to advertise to the frontend what TBP features it supports. Once the frontend receives this message, it can inform the bot of the game rules using the `rules` message.

Attribute  | Type             | Description
---------  | ----             | -----------
`name`     | String           | The human-friendly name of the bot. Example: `"Cold Clear"`.
`version`  | String           | A string version identifier. Example: `"2020-05-05"`.
`author`   | String           | A string identifying the author of the bot. Example: `"MinusKelvin"`.
`features` | Array of Strings | A list of supported feature strings (e.g. `"skip_suggest"`, `"set_min_nodes"`).

Example:
```json
{
  "type": "info",
  "name": "Cold Clear",
  "version": "2020-05-05",
  "author": "MinusKelvin",
  "features": ["skip_suggest", "set_min_nodes"]
}
```

### `ready`

The `ready` message tells the frontend that the bot understands the specified game rules and is ready to receive a position to calculate from.

Example:
```json
{
  "type": "ready"
}
```

### `suggestion`

The `suggestion` message is sent in response to a `suggest` message (or following state updates if `"skip_suggest"` is enabled). It informs the frontend of what moves the bot wishes to make in order of preference.

The frontend should play the most preferred valid move. If no moves are valid (or the `moves` array is empty), the bot forfeits and the frontend should tell the bot to stop calculation.

Attribute | Type             | Description
--------- | ----             | -----------
`moves`   | Array of Objects | A list of moves in order of preference.

Each move is an object containing `location` and `spin`, matching the move definition described under `play`.

Example:
```json
{
  "type": "suggestion",
  "moves": [
    {
      "location": {
        "type": "L",
        "orientation": "north",
        "x": 8,
        "y": 0
      },
      "spin": "none"
    }
  ]
}
```

### `error`

The `error` message informs the frontend that the bot encountered an invalid state, unsupported rules configuration, or a protocol error.

Attribute | Type   | Description
--------- | ----   | -----------
`reason`  | String | *(Optional)* A description of the error.

Example:
```json
{
  "type": "error",
  "reason": "Unsupported rule set"
}
```

---

## Piece Geometry and SRS True Rotation

The center of the piece is defined according to [SRS true rotation](https://harddrop.com/wiki/File:SRS-true-rotations.png).

* The piece centers for the **L, J, T, S, and Z** pieces are the stationary points of basic rotation in SRS (the central mino of their 3×3 matrix).
* The center of the **O** piece:
  * `north`: bottom-left mino
  * `east`: top-left mino
  * `south`: top-right mino
  * `west`: bottom-right mino
* The center of the **I** piece:
  * `north`: middle-left mino
  * `east`: middle-top mino
  * `south`: middle-right mino
  * `west`: middle-bottom mino

---

## Summary of Differences: TBP1 vs TBP2 vs SBP

Feature / Aspect     | TBP1 (`0000-mvp.md`)   | TBP2 (This Document)               | SBP (`StatelessBotProtocol.md`)
-------------------- | ---------------------- | ---------------------------------- | -------------------------------
**State Model**      | Stateful               | **Stateful** (Default)             | **Stateless** (Alternative)
**`opponents` List** | Not present            | **Present** in `start` and `play`  | Present in `play`
**`meter` Field**    | Not present            | **Present** in `start` & opponents | Present in `play` & opponents
**`back_to_back`**   | `Boolean`              | **`Integer`** (streak count/level) | `Boolean` or `Integer`
**Game Start**       | `start`                | **`start`**                        | `play`
**Move Updates**     | `suggest`/`suggestion` | **`suggest`/`suggestion`**        | `play` triggers response
**Piece Delivery**   | `new_piece`            | **`new_piece`**                    | Full queue in every `play`
**Inter-Game Reset** | `stop`                 | **`stop`**                         | Not required (stateless)
