# Reverse-engineering progress

Target: Android ARM64 `libminecraftpe.so`, Minecraft Bedrock 1.26.60.4
GNU build ID: `eb291b883dfdfaff70c4999c2eabd61885ae3e15`
Branch: `re/1.26.60.4-analysis`

## Stages

| Stage | Status | Notes |
|---|---|---|
| 1. Inventory | Done | ELF sections, dynamic-symbol count, and selected RTTI names/typeinfo and vtable candidates recorded. |
| 2. Vtables | Done | Contiguous relocation-backed slots and up to 40 instructions from each distinct slot-0 target are recorded; methods remain unnamed. |
| 3. Player chain | Blocked | The supplied offsets are recorded as unverified; `game_hooks.cpp` and the three claimed signature definitions are absent from this checkout. |
| 4. Feature targets | Done | FOV, HUD, block-render, and actor/bounds string leads are rated; no feature function addresses were established. |
| 5. Signatures | Blocked | `signatures.json` contains no fabricated patterns; the verifier exits blocked for an empty list. |

## Environment and inputs

- Available memory at start: 7.8 GiB; CPUs: 2. Ghidra headless was skipped per the 16 GiB minimum.
- `libminecraftpe.so` is local and ignored by Git. It must not be committed.
- `readelf`, `nm`, `strings`, `llvm-objdump`, and Python 3 are available. Python LIEF, Capstone, pyelftools, radare2, and rizin are not installed.
- The named `app/src/main/cpp/eon/game_hooks.cpp` file and `NormalTick`, `ActorManagerList`, `RenderLevel` signature text were not found in the repository.

## Resume

See [tools/re/README.md](../../tools/re/README.md) for reproduction commands. Continue with the remaining stages only when the exact target hooks or an appropriate debug/symbol source are available; keep all conclusions tied to this build ID.
