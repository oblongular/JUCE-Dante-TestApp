# JUCE Dante Audio Backend — Test Application

A command-line test application for the Dante audio backend for the [JUCE](https://juce.com/) framework.
The backend uses the Dante DEP (Dante Embedded Platform) shared-memory API to expose a Dante device
as a standard JUCE `AudioIODevice`.

## What it does

- **List mode** — scans all available audio backends and prints each device with its input/output channel count
- **Loopback mode** — opens a named device and copies all input channels to the corresponding output channels

## Dependencies

| Dependency | Source |
|---|---|
| JUCE 8.0.14 + Dante backend | fetched by Nix from `github.com/oblongular/JUCE`, `add-dante-backend` branch |
| Dante DEP Client SDK | fetched by Nix from `github.com/oblongular/Dante-DEP-Client-SDK` |
| Dante Embedded Platform (DEP) | must be running on the system for Dante device to appear |
| Nix (provides the build toolchain) | system |

## Setup

Clone the repository:

```sh
git clone git@github.com:oblongular/JUCE-Dante-TestApp.git
```

JUCE and the Dante DEP Client SDK are fetched and managed automatically by the Nix flake.

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

### Loopback on the Dante endpoint

```sh
./build/JUCE-Dante-TestApp_artefacts/JUCE-Dante-TestApp -l
```

Defaults to the `DanteEP` shared-memory endpoint. Use `-s`/`--shm` to target a
different one:

```sh
./build/JUCE-Dante-TestApp_artefacts/JUCE-Dante-TestApp -l -s <name>
```

### Loopback with custom TX lead / RX lag

```sh
./build/JUCE-Dante-TestApp_artefacts/JUCE-Dante-TestApp -l -t <microseconds> -r <microseconds>
```

`-t`/`--txlead` keeps the write cursor ahead of DEP's live edge (default: 1000µs);
`-r`/`--rxlag` keeps the read cursor behind it (default: 0µs). Both are one-way
added latency — lower values reduce loopback latency but increase the risk of
glitches. See the [Dante DEP Client SDK's diagram](https://github.com/oblongular/Dante-DEP-Client-SDK/blob/main/docs/txlead-rxlag.svg)
for the underlying mechanism.

**Rounding**: the SDK converts `-t`/`-r`'s microsecond value to frames via
*truncating* integer division (`frames = us × sampleRate / 1e6`, floored, not
rounded) — see `DanteAudio.cpp`'s `reset()`. At 48kHz, 1 frame = 20.833...µs, so
most microsecond values don't land on a whole frame boundary, and the result
always rounds **down**. To guarantee at least N frames, round the µs value
**up**, not to the nearest integer:

- 32 frames = 666.666...µs exactly. `666` floors to `31` frames (one short);
  `667` floors to `32.016` → `32` frames (correct) — `667` is the *smallest*
  integer µs value that still reaches 32, not an arbitrary rounding.

```sh
rt-run-dsp.sh /tmp/JUCE-Dante-TestApp -l -t 667
```

Press `Ctrl+C` to stop the loopback.

## Options

| Option | Description |
|---|---|
| `-l, --loopback` | Run loopback mode |
| `-s, --shm <name>` | DEP shared-memory endpoint name (default: `DanteEP`) |
| `-t, --txlead <us>` | Dante TX lead in microseconds (default: 1000) |
| `-r, --rxlag <us>` | Dante RX lag in microseconds (default: 0) |

## Running a binary built outside Nix

On Ubuntu, one may build and run this test application.
A binary compiled on Ubuntu will not "just run" on NixOS
due to differences in shared library paths.

On NixOS, use `steam-run` to provide an FHS-compatible environment:

```sh
chmod +x ./Defeedback
NIXPKGS_ALLOW_UNFREE=1 nix run nixpkgs#steam-run --impure -- ./Defeedback
```
