# EEG Firmware for nRF5340 Application Core

## Table Of Contents

<!--toc:start-->

- [EEG Firmware for nRF5340 Application Core](#eeg-firmware-for-nrf5340-application-core)
  - [Development Environment](#development-environment)
    - [Hardware](#hardware)
    - [Multicore](#multicore)
    - [Build & Flash](#build-flash)

<!--toc:end-->

### Development Environment

Nordic Connect SDK 2.6.1

#### Hardware

The hardware for this firmware can be referenced at the following [eeg_hw](https://github.com/bradkim06/eeg_hw)

> Important Note⚠️ : This hardware (v0.2-dev) is in the early stages of development, and the following issues must be solved
>
> - Improvement of the power circuit (noise)
> - Improvement of the grounding design (noise)

#### Multicore

The Bluetooth host stack runs on the application core, while the HCI
(Host Controller Interface) and controller stack run on the network core.
Through the CONFIG_BT_HCI_IPC([multi-image build](https://docs.nordicsemi.com/bundle/ncs-2.5.2/page/nrf/device_guides/working_with_nrf/nrf53/nrf5340.html#multi-image_builds)), the network core is built simultaneously

#### Build & Flash

This firmware is customized for my custom board. Modifications are necessary to use it on other boards.
For detailed instructions, please refer to the Nordic Academy:
[Lesson 3 – Adding custom board support](https://academy.nordicsemi.com/courses/nrf-connect-sdk-intermediate/lessons/lesson-3-adding-custom-board-support/)

```bash
west build -p
west flash
```
