# ELF and class inventory

Target build ID: `eb291b883dfdfaff70c4999c2eabd61885ae3e15`.
All addresses below are ELF virtual addresses.

## Sections

Values are from `readelf -W -S libminecraftpe.so`; sizes are hexadecimal.

| Section | Type | VA | File offset | Size |
|---|---|---:|---:|---:|
| `.note.android.ident` | NOTE | `0x14320000` | `0x1430c000` | `0x98` |
| `.note.gnu.build-id` | NOTE | `0x14320098` | `0x1430c098` | `0x24` |
| `.dynsym` | DYNSYM | `0x143200c0` | `0x1430c0c0` | `0x219438` |
| `.gnu.version` | VERSYM | `0x14539500` | `0x14525500` | `0x2cc5a` |
| `.gnu.version_r` | VERNEED | `0x14566160` | `0x14552160` | `0x80` |
| `.gnu.hash` | GNU_HASH | `0x2463f0` | `0x2463f0` | `0xaeaf4` |
| `.dynstr` | STRTAB | `0x2f4ee4` | `0x2f4ee4` | `0x7ffae6` |
| `.rela.dyn` | RELA | `0xaf49d0` | `0xaf49d0` | `0x18188a0` |
| `.rela.plt` | RELA | `0x145661e0` | `0x145521e0` | `0x10cc98` |
| `.rodata` | PROGBITS | `0x2419f00` | `0x2419f00` | `0x136c0f4` |
| `.gcc_except_table` | PROGBITS | `0x3785ff4` | `0x3785ff4` | `0x971c6c` |
| `.eh_frame_hdr` | PROGBITS | `0x40f7c60` | `0x40f7c60` | `0x60ac94` |
| `.eh_frame` | PROGBITS | `0x47028f8` | `0x47028f8` | `0x21322b0` |
| `.text` | PROGBITS | `0x6834bc0` | `0x6834bc0` | `0xcd9e7c8` |
| `malloc_hook` | PROGBITS | `0x135d3388` | `0x135d3388` | `0x300` |
| `.plt` | PROGBITS | `0x135d3690` | `0x135d3690` | `0xb3320` |
| `.data.rel.ro` | PROGBITS | `0x1368a9b0` | `0x136869b0` | `0x90a3b8` |
| `.fini_array` | FINI_ARRAY | `0x13f94d68` | `0x13f90d68` | `0x10` |
| `.init_array` | INIT_ARRAY | `0x13f94d78` | `0x13f90d78` | `0x4a28` |
| `.dynamic` | DYNAMIC | `0x14678000` | `0x14660000` | `0x260` |
| `.got` | PROGBITS | `0x13f999e0` | `0x13f959e0` | `0x12c98` |
| `.got.plt` | PROGBITS | `0x13fac678` | `0x13fa8678` | `0x59998` |
| `.relro_padding` | NOBITS | `0x14006010` | `0x14002010` | `0xff0` |
| `.data` | PROGBITS | `0x1400a010` | `0x14002010` | `0x38d98` |
| `.bss` | NOBITS | `0x14042dc0` | `0x1403ada8` | `0x2c60a0` |
| `.comment` | PROGBITS | `0x0` | `0x1403ada8` | `0x499` |
| `.shstrtab` | STRTAB | `0x0` | `0x1403b241` | `0x11f` |

The stripped file has `.dynsym`/`.dynstr`, but no `.symtab` or `.strtab`.
There are 90,670 defined dynamic symbols. Representative exported names
include `AB64ToARGB`, `MultiplayerGetSessionHelper`, and Xbox Live multiplayer
API functions. Searches of the defined dynamic symbols did not find exported
symbols for the requested Bedrock core classes or EoN hook names. Reproduce
the full export listing with `nm -D --defined-only -C libminecraftpe.so`.

## RTTI and vtable candidates

`classes.json` records exact class-name string addresses, the relocation that
points to each name, the resulting typeinfo address, and every RTTI-backed
vtable-prefix candidate. **CONFIRMED** typeinfo entries are based on
`R_AARCH64_RELATIVE` relocations from the typeinfo name pointer to an
Itanium-ABI class name. Candidate vtable addresses are **LIKELY**: the
reported location is the two-word prefix before a relocation to that
typeinfo, with a following function pointer when one was available.

| Class | Typeinfo VA | Candidate table VAs with an initial function pointer |
|---|---:|---|
| `LocalPlayer` | `0x139f7ec0` | `0x139f6e90` |
| `Player` | `0x13d449f0` | `0x13d42cc8` |
| `Actor` | `0x13d06510` | `0x13cf56d8` |
| `Mob` | `0x13cc81f0` | `0x13cb8500`, `0x13cc4cc0` |
| `Level` | `0x13e0ca68` | `0x13e0a0e8`, `0x13e0ade0`, `0x13e0ae40` |
| `ClientInstance` | `0x13890f40` | `0x1388fe60`, `0x13890c10`, `0x13890c28`, `0x13890c68`, `0x13890cd0`, `0x13890d08` |
| `LevelRenderer` | `0x13a56258` | `0x13a54f70`, `0x13a550a0` |
| `BlockSource` | `0x13d6eaf0` | `0x13d6dcc0` |
| `Dimension` | `0x13e0d750` | `0x13e0cec8`, `0x13e0d070`, `0x13e0d180`, `0x13e0d1b0` |
| `LevelRendererPlayer` | `0x13a56318` | `0x13a557c0`, `0x13a55930`, `0x13a55a40`, `0x13a55a78`, `0x13a55aa0` |
| `ChunkSource` | `0x13e126a0` | `0x13e11408` |
| `LevelChunk`, `Chunk`, `SubChunk` | Not recovered | None established |

Multiple candidates for a typeinfo are expected where a class has secondary
bases or construction tables. The addresses do not by themselves distinguish
the primary vtable from construction or secondary tables. See
[vtables.md](./vtables.md) for slot addresses and bounded disassembly.

## Strings that identify relevant components

The string table includes class-qualified messages/signatures such as
`virtual void ClientInstance::requestLeaveGame(bool, bool)`,
`virtual void Level::tick()`, `virtual void Dimension::tick()`,
`virtual void Player::tickWorld(const Tick &)`, and
`virtual void LevelRendererPlayer::preRenderUpdate(...)`. These are
**CONFIRMED strings**, not recovered function addresses. Addresses and
interpretations are not inferred from them.

## Commands

See [the reproduction README](../../tools/re/README.md). In particular,
`python3 tools/re/scan_rtti.py libminecraftpe.so --classes-only` recreates
the structured RTTI/vtable-candidate data in `classes.json`.
