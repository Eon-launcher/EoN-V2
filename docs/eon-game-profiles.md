<!-- Modified by EoN Launcher V2. -->
# EON game profiles

The profile loader reads an optional
`<EON mod config directory>/profiles/1.26.60.4.json`. Start from
[`1.26.60.4.json.example`](../app/src/main/cpp/eon/profiles/1.26.60.4.json.example)
and copy it to that path. `null` means a slot is unset. Function offsets are
relative to the load base of `libminecraftpe.so`; they are not absolute
addresses. The loader accepts non-negative JSON integers or hexadecimal
strings (for example, `"0x1234"`). Unknown function slot names and malformed
offsets invalidate the profile.

The example deliberately leaves every function slot unset. The current Fly,
Speed, and HighJump modules require, respectively:

- Fly: `LocalPlayer_normalTick`, `Player_getAbilities`,
  `LayeredAbilities_getLayer`, `Abilities_setAbility`
- Speed: `SpeedMoveSystem`
- HighJump: `JumpHeight`

The profile has no struct-field offsets: these Phase 4a modules do not read or
write raw game struct fields. The `fields` object remains empty until a verified
module implementation requires a specific field.

## Version and library information

At pinned preloader commit
`cc08b40a31471d3e2dedb267f5f17b28da2b0604`, `include/pl/Mod.hpp` defines
`ModInfo::version` as mod metadata (lines 27-38) and `ModContext` accessors for
mod directories (lines 43-70). It does not expose the running Minecraft game
version or the base address of `libminecraftpe.so`.

The public memory API provides `resolveSignature` and
`resolveSignatures` in `include/pl/memory/Signature.hpp` (lines 19-30), but no
library-base accessor. EON therefore finds the loaded library base with
`dl_iterate_phdr`; it does not calculate addresses until profile readiness
passes.

No launcher or JNI changes were made to propagate the selected Minecraft
version. To honor that constraint, `running_game_version` is an explicit,
operator-supplied value in the profile file, not a version detected from the
running process. It must equal `game_version` for `Game::ready` to pass. This
manual assertion cannot independently prove which game binary is running;
the profile is not safe to activate based on that value alone. All offsets in
the example are unset, so the movement modules remain locked regardless.

## Phase 4a boundary

The reference client was consulted only for feature intent: Fly toggles a
creative flying ability, Speed modifies movement-system behavior, and
HighJump changes the jump-height result. Its older function declarations are
not treated as verified 1.26.60.4 ARM64 ABIs. The movement modules remain
fail-closed and install no hooks until those ABIs and profile offsets are
verified. No raw fields or ABI assumptions were copied from the reference.
