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

The `-t` option sets the Dante TX latency in microseconds (default: 370 µs).
Lower values reduce loopback latency but increase the risk of glitches.

At 48 kHz, 1 sample ≈ 21 µs. The DepLoopback reference application uses 17 samples ≈ 354 µs:

```sh
./build/JUCE-Dante-TestApp_artefacts/JUCE-Dante-TestApp -l Dante -t 354
```

Press `Ctrl+C` to stop the loopback.

## Options

| Option | Description |
|---|---|
| `-l <name>` | Device name to use for loopback |
| `-t <us>` | Dante TX latency in microseconds (default: 370) |
