# Minecraft Bedrock 1.26.60.4 analysis

## Scope and evidence

Input: local Android ARM64 ELF shared object, build ID
`eb291b883dfdfaff70c4999c2eabd61885ae3e15`. Its GNU build ID matches the
requested identifier. Addresses in this report are ELF virtual addresses, not
ASLR runtime pointers.

Evidence labels:

- **CONFIRMED** — directly established by ELF metadata or a relocation.
- **LIKELY** — a structural interpretation of the binary that still needs a
  runtime or symbol-level check.
- **GUESS** — not established; no such conclusion is used as a finding below.

## Findings

1. **CONFIRMED:** The library is little-endian ELF64 for AArch64. Its `.text`
   section is at VA `0x6834bc0`, and the GNU build ID is the one above.
2. **CONFIRMED:** The object retains 90,670 defined dynamic symbols and
   relocation-backed Itanium RTTI for several requested classes. The RTTI
   name-pointer relocations identify exact typeinfo addresses; the related
   candidate vtable prefixes are listed in [classes.json](./classes.json).
3. **LIKELY:** The RTTI-backed prefixes and contiguous slot addresses in
   [vtables.md](./vtables.md) are vtable candidates. Some are secondary-base
   tables (negative offset-to-top values); the primary table must not be
   inferred solely from the class name. Slot-zero disassembly is available
   without a semantic method name.
4. **CONFIRMED:** No Ghidra analysis was run: available memory was 7.8 GiB,
   below the requested 16 GiB threshold. `llvm-objdump` supports AArch64 and
   was available; LIEF and Capstone were not.
5. **CONFIRMED:** The claimed EoN hook source and `NormalTick`,
   `ActorManagerList`, and `RenderLevel` signatures are absent from this
   checkout. Therefore the requested player argument, field offsets, and
   signature bytes cannot be verified from those references.
6. **GUESS / UNVERIFIED:** All seven user-supplied player field offsets remain
   unverified; no pointer-chain edge or semantic field label beyond the
   supplied `player->level` claim is asserted.
7. **LIKELY leads only:** String evidence identifies FOV, HUD, block-render,
   and actor/bounds concepts, but no feature hook address or runtime
   decision-making path. Details and offsets are in [features.md](./features.md).

## Open questions and next steps

- Identify the actual EoN hook source/signature definitions that produced the
  reported unique matches, then reproduce those matches against this exact
  build ID.
- Use those verified function starts to trace the player/level/actor chain;
  none of the seven supplied offsets is confirmed here.
- Resolve each RTTI-backed vtable candidate to a complete table and named
  functions before treating any slot as a feature hook.
- Recover feature paths from verified call sites or symbols. String matches
  alone do not prove a FOV, HUD, block-visibility, or entity-iteration hook.
- Generate and verify signatures only after identifying each target routine.
- `signatures.json` intentionally contains an empty list and blocked status;
  no exact-once signature claim is made.

See [classes.md](./classes.md), [vtables.md](./vtables.md),
[player-chain.md](./player-chain.md),
[features.md](./features.md), and [signatures.json](./signatures.json) for the
current per-stage evidence and limitations.
