# Player / level chain

## Result

**The requested chain was not recovered.** The checkout does not contain
`app/src/main/cpp/eon/game_hooks.cpp`, any `game_hooks.cpp`, or the claimed
`NormalTick`, `ActorManagerList`, and `RenderLevel` signature definitions.
Repository-wide searches (excluding the local library) found no occurrences.
Without the exact `NormalTick` match and its containing function, there is no
evidence for its first argument or a chain from that argument to a player,
level, actor collection, position, or bounding box.

The RTTI addresses for `LocalPlayer`, `Player`, `Actor`, and `Level` in
[classes.json](./classes.json) confirm type metadata exists; RTTI does not
identify object fields or establish that one object points to another.

## Supplied offsets

These are the values provided as existing assumptions in the task. Their
semantics and use sites could not be tied to a disassembly, so every offset
remains **GUESS / UNVERIFIED** for this build.

| Supplied offset | Intended meaning | Status |
|---:|---|---|
| `+0x1f0` | Claimed `player->level` field | GUESS; no instruction evidence |
| `+0x228` | Not specified | GUESS; no instruction evidence |
| `+0x230` | Not specified | GUESS; no instruction evidence |
| `+0x238` | Not specified | GUESS; no instruction evidence |
| `+0x430` | Not specified | GUESS; no instruction evidence |
| `+0x478` | Not specified | GUESS; no instruction evidence |
| `+0x614` | Not specified | GUESS; no instruction evidence |

No field labels or pointer-chain edges are assigned to the latter six offsets.
Do not use these as verified struct layouts.

## Pointer/tag and address notes

- The report's ELF addresses are static virtual addresses for this exact
  build, not runtime process addresses.
- The task's note to clear the top byte of a tagged heap pointer before
  comparing it with `/proc/self/maps` is runtime guidance supplied by the
  requester; no process mapping or live pointer was available for independent
  verification here.
- No ASLR base, runtime object address, or pointer chain is reported.

## What is needed to continue

Provide the relevant source/signature definitions or a reproducible description
of each unique match. Then locate and disassemble the matching routine, confirm
its AArch64 calling convention and use sites, and trace every load from that
argument before assigning field names or offsets. Validate any candidate
layout against a matching runtime build before using it.
