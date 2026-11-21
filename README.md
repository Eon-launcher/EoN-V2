# EoN Launcher V2

<!-- Modified by EoN Launcher V2: replace upstream project branding and setup instructions. -->

![EoN Launcher V2 icon](app/src/main/res/mipmap-xxxhdpi/ic_launcher.png)

EoN Launcher V2 is an Android launcher for Minecraft Bedrock Edition. It manages game instances, worlds, resource packs, accounts, and native mods in one app.

Native mod features are intended only for private worlds or servers that explicitly permit them. Do not use modifications to evade server checks or violate server rules.

## Requirements

- Android 9.0 (API 28) or later
- ARM64 (`arm64-v8a`)
- A licensed Minecraft Bedrock Edition installation from Google Play

## Build

The GitHub Actions workflow builds a debug APK on pushes to `main` and can be run manually:

1. Open the repository's **Actions** tab.
2. Select **EoN Launcher V2 Debug APK**, then **Run workflow**.
3. Download the `EoN-debug-apk` artifact from the completed run.

For a local source build, install JDK 21, Android SDK platform 36, build-tools 36.0.0, NDK 28.2.13676358, and CMake 3.22.1, then run:

```bash
./gradlew assembleDebug
```

Checkout submodules recursively to fetch the pinned native dependencies:

```bash
git clone --recurse-submodules https://github.com/txqeaandrei-png/EoN-V2.git
```

## License and credits

EoN Launcher V2 is distributed under the Apache License 2.0. See [LICENSE](LICENSE), the [worldtools notice](app/src/main/assets/worldtools/NOTICE.txt), and the [Pojav Controls notice](pojav_controls/NOTICE) for license and attribution details.

Based on LeviLauncher-Android by LiteLDev (Apache 2.0).
