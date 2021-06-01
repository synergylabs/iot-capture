# Capture: Centralized Library Management for Heterogeneous IoT Devices

This repository contains source code for the system Capture described in this paper:
```bibtex
@INPROCEEDINGS{zhang21capture,
    title = {Capture: Centralized Library Management for Heterogeneous IoT Devices},
    author = {Han Zhang and Abhijith Anilkumar and Matt Fredrikson and Yuvraj Agarwal},
    booktitle = {{USENIX} Security Symposium}
    year = {2021},
}
```

We will continue improving this repository for general release. Due to recent time constraints, parts of the codebase is not yet fully prepared.

## Dependencies

### Arduino IDE and tool chain

In order to flash code into Arduino or ESP32 on Mac (maybe other platform as well), you need to install the driver. Specifically, you need to follow instructions from https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers

## Device Setups

The main hub setup involves several scripts in [bin](bin/) and a dedicated folder:  [device_setup](device_setup/).
Please refer to those files to install and configure network interfaces.

## Source Code

Project related source code are inside `src/` folder.

- `capture_core`: Code for the main Capture hub.
- `capture_esp32`: Library code for ESP32 applications and library integrations.
- `capture_linux`: Driver and adapter code for the Linux demo app used in evaluation.

## Demo Programs

Demo applications.

- [demo_esp32](demo_esp32): ESP32 apps for library replacement and SDK API integration approaches.

- [demo_pi](demo_pi): Submodule to the MagicMirror app. [Link-to-specific-commit](https://github.com/michmich/MagicMirror/tree/5bf90ae31d600e3f595ffd242b99515fa7905b1a).

- Macro-benchmark involves three sub folders (`macro-benchmark-*`).

## Acknowledgment and External Links

- Dreamcatcher project official repo: https://github.com/jericks-umich/Dreamcatcher

- Vigilia codebase including how to use TOMOYO Linux on Raspberry Pi: `git clone git://plrg.eecs.uci.edu/vigilia.git`

- Reference IFTTT macro-benchmark from Mi et. al., IMC 2017: https://github.com/mixianghang/IFTTT_measurement.git

- Hostap / wpa2_supplicant official website: https://w1.fi

- Arduino IDE setup for ESP32: https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/
