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

## Phase 4b: ESP

ESP has a separate Android ARM64 path. It installs NormalTick and RenderLevel
hooks only when the loaded `libminecraftpe.so` GNU Build ID is
`eb291b883dfdfaff70c4999c2eabd61885ae3e15`, then resolves the three expected
functions by signature. It reads actor bounds and camera state, projects the
eight corners of each visible bound onto the preloader HUD surface, and submits
white wireframe boxes as HUD draw commands. If the build ID, signatures, or
hooks are unavailable, ESP cannot be enabled.

This gate identifies the exact library image, not the game account, server, or
world. The actor/renderer field offsets and function ABIs are specific to that
image; validate ESP in-game on the matching ARM64 build before treating it as
runtime-verified. Xray, MobAura, and Scaffold remain unavailable until their
own implementations and target-build interfaces are verified.
