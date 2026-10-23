# Overview

TBP2 is the protocol the stadium speaks to bots that don't advertise `SBP`
(the [Stateless Bot Protocol](StatelessBotProtocol.md)). It is
[TBP](https://github.com/tetris-bot-protocol/tbp-spec/tree/main/text) (the MVP
and the `randomizer` extension) with the additions below, mostly so a bot can
see its opponent and the versus rules. Everything not listed here is as in TBP.

# Additions

## `info` features

Feature         | Meaning
-------         | -------
`set_threads`   | The bot accepts `threads` in `rules`.
`set_min_nodes` | The bot accepts `min_nodes` in `rules`.
`skip_suggest`  | The frontend never sends `suggest`. The bot sends a `suggestion` unprompted after every `start` and `play`.

## `rules`

Attribute       | Description
---------       | -----------
`threads`       | Search threads to use. Sent only to bots with `set_threads`, and only when the frontend sets it.
`min_nodes`     | The least search work to do before suggesting. Sent only to bots with `set_min_nodes`.
`garbage_delay` | Turns an attack spends in transit before it joins the receiver's meter. Always sent; omitted means `0`.

Garbage works per turn, where both players place one piece at once:

- An attack cancels, in order, the sender's meter, then garbage still in
  transit to the sender (soonest-arriving first), then the opponent's attack
  sent the same turn. Garbage always cancels in transit (zero pass-through).
- What's left is in transit for `garbage_delay` turns, then joins the
  receiver's meter. With `0` it joins on the turn it is sent, so it can
  land under the receiver's piece that same turn.
- A placement that clears no lines tanks the whole meter as one block with a
  single random hole.

## `start`

Attribute      | Description
---------      | -----------
`back_to_back` | An integer, the back to back count, instead of TBP's boolean.
`meter`        | Integer, garbage in the meter. The stadium currently always sends `0`.
`opponents`    | A list of the other players, each with its own `board`, `hold`, `queue`, `combo`, `back_to_back` and `meter` as above.
`randomizer`   | As in TBP's randomizer extension (`seven_bag`), but sent without the `randomizer` attribute on `rules`.

`start` can come while the bot is calculating, with no `stop` before it, and
replaces the position. The stadium sends it at the start of every game and,
mid-game, instead of `play` and `new_piece` after the bot's move tanked
garbage. A bot can replay its last move against the new board to see how much
garbage landed.

## `play`

Attribute   | Description
---------   | -----------
`opponents` | The other players as in `start`, as they stand after this turn's moves (garbage tanked this turn included).

The `move` carries the spin the frontend scored, which can differ from the one
suggested: a spin counts only if some reachable rotation lands the piece there
with that spin (T only, three-corner rule). A `full` claim that only earns a
mini scores as `mini`; otherwise it scores as `none`.
