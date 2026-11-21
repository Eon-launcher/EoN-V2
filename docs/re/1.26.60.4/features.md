# Feature-target leads

The offsets below are `strings -a -n 5 -t x` **file offsets**. They are
string locations, not function addresses. String evidence is sufficient to
identify leads, not sufficient to identify a patch or hook point.

## Zoom / FOV

**LIKELY lead:** the binary contains the FOV settings and camera-option
vocabulary:

| File offset | String |
|---:|---|
| `0x243194f` | `fov_set` |
| `0x244dc2c` | `setFov` |
| `0x248a1a6` | `FieldOfView` |
| `0x250f8ef` | `CameraFovOptions` |
| `0x267f814` | `gfx_field_of_view` |
| `0x267fc56` | `video.fov` |
| `0x27d9786` | `options.fov` |
| `0x24370c1` | `Custom FOV value must be within [30.0, 110.0]` |
| `0x26330fc`, `0x25f28d9` | `zoom-in`, `zoom-out` |

The string table also contains scripting-reflection type names referring to
`ScriptCamera::setFov` and `ScriptCameraFovOptions`. These are **CONFIRMED
strings** only. No camera FOV getter, gameplay camera setup routine, or
function address was established. **Zoom hook: GUESS / unresolved.**

## HUD toggle

**LIKELY lead:** configuration and script/network vocabulary exist:

| File offset | String |
|---:|---|
| `0x2427caa` | `gfx_hidehud` |
| `0x250592c` | `hideHUD` |
| `0x25a1473` | `options.hidehud` |
| `0x26dd855` | `video.hide_hud` |
| `0x262a87c`, `0x262a88d` | `setHudVisibility`, `getHiddenHudElements` |
| `0x2548e75` | `SetHudPacket` |
| `0x2549a6b` | `This packet will toggle the HUD visibility.` |

The `SetHudPacket` string concerns a packet; it does not prove a local F1-style
flag. The setter/getter names occur in script reflection strings and do not
identify a native hook or object offset. **HUD target: LIKELY configuration
lead; native toggle function: unresolved.**

## Xray / block and chunk rendering

**LIKELY lead:** block/render terminology includes `Block Render Layer`
(`0x25b2860`), `block_face` (`0x2434480`), `NoOcclusion` (`0x24d3beb`),
`Level renderer camera - Render terrain alpha` (`0x2486e3a`), `Render terrain
blended (B)` (`0x25a235a`), and `Render player vision` (`0x24a7852`).

These are labels and names only. No face-visibility decision, chunk mesh
function, opacity/material selection, or conditional branch was tied to them.
In particular, the presence of `NoOcclusion` does not prove that it controls
block-face drawing. **Xray hook: GUESS / unresolved.**

## ESP / entity iteration and bounds

**LIKELY lead:** actor position/bounds concepts appear in strings:

| File offset | String |
|---:|---|
| `0x242bc65`, `0x246aa53` | `Actor Data Bounding Box`, `Actor Bounding Box` |
| `0x24c7e77` | `ActorUpdatePositionSystem` (type-name/string fragment) |
| `0x2449654` | `ActorUpdatePostTickPositionDeltaSystem::tick` |
| `0x25311c2`, `0x25e4292` | `AutonomousEntityList`, `AvailableActorIDList` |
| `0x25398ce` | `getPosition` |
| `0x260b417` | Description of an API returning an actor's absolute position |
| `0x25af612`, `0x26fe717` | `showBoundingBox`, `ShowBoundingBox` |

The strings suggest systems and API concepts, not a particular native entity
iteration path. They do not give `Actor`/`Player` instance field offsets or
the screen-space projection/bounds path. **ESP target: LIKELY data-concept
lead; iteration and layout: unresolved.**

## Candidate rating summary

| Feature | Highest justified rating | What is not established |
|---|---|---|
| Zoom | LIKELY settings/API lead | Getter, camera setup, hook address |
| HUD | LIKELY configuration/API lead | Local F1 flag, native toggle function |
| Xray | LIKELY render terminology lead | Face test, opacity/layer decision, hook |
| ESP | LIKELY actor/bounds terminology lead | Entity iteration, object offsets, projection |

`nm -D -C --defined-only` did not identify a corresponding Bedrock feature
implementation symbol. The lack of a recovered symbol does not prove that the
code is absent; the main game implementation is stripped. See
[player-chain.md](./player-chain.md) for the blocked layout trace.
