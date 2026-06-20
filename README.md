# JUCE Dante Audio Backend — Test Application

A command-line test application for the Dante audio backend for the [JUCE](https://juce.com/) framework.
The backend uses the Dante DEP (Dante Embedded Platform) shared-memory API to expose a Dante device
as a standard JUCE `AudioIODevice`.

## What it does

- **List mode** — scans all available audio backends and prints each device with its input/output channel count
- **Loopback mode** — opens a named device and copies all input channels to the corresponding output channels

## Dependencies

| Dependency | Location |
|---|---|
| JUCE (fork with Dante backend) | `../JUCE` (`add-dante-backend` branch) |
| Dante DEP SDK | `dante-dep-sdk/` |
| Dante Embedded Platform (DEP) | must be running on the system for Dante device to appear |
| Nix (provides the build toolchain) | system |

## Dante DEP SDK

The SDK ships as a single header and a single prebuilt static library:

```
dante-dep-sdk/
  include/dante/DanteAudio.hpp   — public API
  lib/libDanteAudio.a             — prebuilt: Audinate DEP objects + DanteAudio.cpp objects
  src/DanteAudio.cpp             — source used to build the combined .a (see below)
  cmake/DanteAudioBuffers.cmake  — imports the library as a CMake target
```

`libDanteAudio.a` is a combination of the Audinate-supplied DEP objects and the `DanteAudio.cpp`
wrapper layer. It is committed pre-built so that consumers only need the header and the `.a`.

### Rebuilding the combined `.a`

The `dante_sdk_dist` CMake target documents and reproduces this build step. It compiles
`DanteAudio.cpp`, merges the result with the original Audinate `libDanteAudio.a`, and writes the
combined library alongside a copy of the header to `build/dante-dep-sdk/`:

```sh
nix develop . --command cmake --build build --target dante_sdk_dist
```

Output:

```
build/dante-dep-sdk/
  include/dante/DanteAudio.hpp
  lib/libDanteAudio.a
```

To update the committed `.a` after changing `DanteAudio.cpp`, copy the rebuilt library back:

```sh
cp build/dante-dep-sdk/lib/libDanteAudio.a dante-dep-sdk/lib/libDanteAudio.a
```

Then commit both `DanteAudio.cpp` and the updated `libDanteAudio.a` together.

## Setup

Clone this repository and the JUCE fork side by side:

```sh
git clone git@github.com:oblongular/JUCE-Dante-TestApp.git
git clone git@github.com:oblongular/JUCE.git
git -C JUCE checkout add-dante-backend
```

## Build

From inside the `JUCE-Dante-TestApp` directory:

```sh
mkdir -p build
nix develop . --command cmake . -B build
nix develop . --command cmake --build build --target JUCE-Dante-TestApp
```

The binary is at `build/JUCE-Dante-TestApp_artefacts/JUCE-Dante-TestApp`.

The `dante_sdk_dist` target also builds as part of the default build, producing `build/dante-dep-sdk/`.
The test app links against the committed prebuilt `dante-dep-sdk/lib/libDanteAudio.a` directly;
the SDK dist build runs in parallel and does not affect the test app link.

## Usage

### List available devices

```sh
./build/JUCE-Dante-TestApp_artefacts/JUCE-Dante-TestApp
```

Example output:

```
[ALSA]
dev:  0   2i|2o  HDA Intel PCH: ALC294 Analog (hw:0,0)

[Dante]
dev:  1  32i|32o  Dante
```

If the Dante Embedded Platform is not running, the Dante entry will appear as `0i|0o  Dante-Not-Present`.

### Loopback on a device

```sh
./build/JUCE-Dante-TestApp_artefacts/JUCE-Dante-TestApp -l <device-name>
```

### Loopback with custom Dante TX latency

```sh
./build/JUCE-Dante-TestApp_artefacts/JUCE-Dante-TestApp -l <device-name> -t <microseconds>
```

The `-t` option sets the Dante TX latency in microseconds (default: 1000µs).
Lower values reduce loopback latency but increase the risk of glitches.

To use 32 samples @ 48kHz (i.e. 667µs), we can run the test app like so:

```sh
rt-run-dsp.sh /tmp/JUCE-Dante-TestApp -l Dante -t 667
```

Press `Ctrl+C` to stop the loopback.

## Options

| Option | Description |
|---|---|
| `-l <name>` | Device name to use for loopback |
| `-t <us>` | Dante TX latency in microseconds (default: 1000) |
