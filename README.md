# Hello 3DS

A small native Nintendo 3DS homebrew installation test.

- Top screen: Hello, World! / Hello, Nicholas!
- A: increment the input-test counter.
- START: exit the application.
- Intended for original and New 3DS systems with custom firmware.
- No network operations, save files, or SD-card writes in the application.

## Build

Install devkitPro's `3ds-dev` group, makerom 0.19.0, and bannertool 1.2.3.
Set DEVKITPRO and DEVKITARM, and put makerom, bannertool, and devkitPro tools on PATH.

    make cia
    make 3dsx

Outputs are in `output/`. This build used devkitARM GCC 16.1.0 and libctru from
the official devkitpro/devkitarm container image, amd64 manifest
`sha256:15b79ce75822c289538d8153da5fa7aafe5e6adc32ad8a575a197beca0f0761b`.

Title ID: `000400000F7A9100`. Version: 1.0.0.

## Install

On your 3DS, open **FBI > Remote Install > Scan QR Code** and scan:

![Install Hello 3DS](install-qr.png)

[Download the CIA](https://raw.githubusercontent.com/CloneDeath/hello-3ds/master/output/hello-3ds.cia)

Press A to test input, or START to exit.

The CIA was compiled and its container content hash checked. It has not been
run on physical 3DS hardware or in an emulator.

Build Makefile and RSF are adapted from TricksterGuy/3ds-template; see
LICENSE-template.txt. Homebrew application source and generated icon/banner
are provided under the MIT license in LICENSE.txt.
