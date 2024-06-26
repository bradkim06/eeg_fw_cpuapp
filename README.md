# EEG Firmware for nRF5340 Application Core

## Development Environment

Nordic Connect SDK 2.6.1

### Multicore

The Bluetooth host stack runs on the application core, while the HCI (Host Controller Interface) and controller stack run on the network core.
Through the CONFIG_BT_HCI_IPC([multi-image build](https://docs.nordicsemi.com/bundle/ncs-2.5.2/page/nrf/device_guides/working_with_nrf/nrf53/nrf5340.html#multi-image_builds)), the network core is built simultaneously

### Build & Flash

> This firmware is dependent on its own custom board. Therefore, paths and naming conventions for the custom board settings are specified in the CMakeLists.txt file

```bash
west build -p
west flash
```
