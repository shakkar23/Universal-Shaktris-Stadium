# Overview

This document specifies the minimum viable product of the TBP, as well as the
minimum subset a conforming bot must implement. This version of the TBP is quite
limited, but extensions will allow extra functionality and information to be
added in the future.

A TBP bot is generally a separate process that sends and receives JSON messages
to communicate with a frontend. On most platforms, a bot is a separate
executable program that a frontend executes and communicates with over standard
input and output. On the web, a bot is a script that the frontend can launch as
a Web Worker and communicate with using `postMessage`.

The typical lifecycle of a TBP bot is this:

1. Bot sends `info`
1. Frontend sends `rules`
1. Bot sends `ready`
1. Frontend sends `play`
1. Bot sends `move`
1. Go to step 4 until the game is over
1. Frontend sends `quit`

# Reference

A bot conforming to the SBP exists as a standalone executable that is executed
by the frontend and communicates over standard input and output. Each message is
JSON terminated by a line break. JSON is chosen as the data format because it is
ubiquitous, human readable, and easily allows the protocol to be extended in the
future.

On the web, a conforming bot is a script that can be executed as a Web Worker
and communicates using JavaScript objects via the `postMessage` API.

Messages consist of an object with a `type` attribute indicating its type.
Messages with an unrecognized type should be ignored. Unrecognized attributes
should also be ignored.

## Messages (Frontend -> Bot)

### `rules`

The `rules` message tells the bot what game rules are in place. Currently, this
message is empty, and will be extended later. The bot must respond with either
the `ready` message, if the bot can play with those rules, or `error` if it
cannot. The standard guideline rules are a loose default; 10x40 board, pieces
spawn in the North orientation at x=4, roughly y=19, hold is allowed, and the
Super Rotation System is used. Since many Tetris games behave slightly
differently and these differences generally have little effect on the capacity
of a bot to pick moves, the defaults are not precisely specified.

### `play`

The `play` message tells the bot to begin calculating from the specified
position. This message must be sent before asking the bot for a move. This 
message also includes an array named `opponents` of the specified attributes, 
each element in the array being a player. The player the bot will not be in the 
`opponents` array.


Attribute       | Description
---------       | -----------
`hold`          | Either a pieces if hold is filled or `null` if hold is empty.
`queue`         | A list of pieces. Example: `["S", "Z", "O"]`
`combo`         | The number of consecutive line clears that have been made.
`back_to_back`  | A boolean indicating back to back status.
`board`         | A list of 40 lists of 10 board cells.
`meter`         | the amount of garbage in the meter.

A board cell can be either `null` to indicate that it is empty, or a string
indicating which piece was used to fill it, or `"G"` for garbage cells.

### `stop`

The `stop` message tells the bot to stop calculating.

### `quit`

The `quit` message tells the bot to exit.

### `ready`

The `ready` message tells the frontend that the bot understands the specified
game rules and is ready to receive a position to calculate from.

### `info`

The `info` message must be sent by the bot immediately after it it launched to
advertise to the frontend what TBP features it supports. Once the frontend
receives this message, it can inform the bot of the game rules using the `rules`
message.

Attribute  | Description
---------  | -----------
`name`     | A string, the human-friendly name of the bot. Example: `"Cold Clear"`
`version`  | A string version identifier. Example: `"Gen 14 proto 8"`
`author`   | A string identifying the author of the bot. Example: `"SoRA_X7"`
`features` | A list of supported features where the "SBP" feature is one of the features.

### `move`

The `move` message is sent in response to a `play` message. It informs the frontend of what moves the bot wishes to make. The frontend should play the most preferred valid move. If no moves are valid, the bot forfeits and the frontend should tell the bot to stop calculation. Whether a hold should be performed is inferred from the type of piece to be placed.

Attribute | Description
--------- | -----------
`moves`   | A list of moves in order of preference.

A move is an object with the following attributes:

Attribute  | Description
---------  | -----------
`location` | A piece location specifying where the piece should be placed.
`spin`     | One of `"none"`, `"mini"`, or `"full"` indicating whether a spin should be performed.


A piece location is an object with the following attributes:

Attribute     | Description
---------     | -----------
`type`        | The type of piece this is. Example: `"I"`
`orientation` | The rotation state of the piece. Spawn orientation is `"north"`, after a clockwise rotation `"east"`, etc.
`x`           | The x coordinate of the center of the piece, with 0 being the coordinate of the leftmost column.
`y`           | The y coordinate of the center of the piece, with 0 being the coordinate of the bottommost row.

The center of the piece is defined according to
[SRS true rotation](https://harddrop.com/wiki/File:SRS-true-rotations.png).
The piece centers for the L, J, T, S, and Z pieces are the stationary points
of basic rotation in SRS. The center of the O piece in the north orientation is
the bottom-left mino; for east, top-left mino; for south, the top-right mino;
for west, the bottom-right mino. The center of the I piece in the north
orientation is the middle-left mino; for east, the middle-top mino; for south,
the middle-right mino; for west, the middle-bottom mino.
