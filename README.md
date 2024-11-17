# Prospector ZMK Module

All the necessary stuff for Prospector to display things with ZMK. Currently functional albeit barebones.

## Features

- Highest active layer roller
- Peripheral battery
- Peripheral connection status

## Usage

Your ZMK keyboard should be set up with a dongle as central.

Add this module to your `config/west.yml` with these new entries under `remotes` and `projects`:

```
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: carrefinho                            # <--- add this
      url-base: https://github.com/carrefinho     # <--- and this
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: prospector-zmk-module                 # <--- and these
      remote: carrefinho                          # <---
      revision: main                              # <---
  self:
    path: config
```

Then add the `prospector_adapter` shield to the dongle in your `build.yaml`:

```
---
include:
  - board: seeeduino_xiao_ble
    shield: [YOUR KEYBOARD SHIELD]_dongle prospector_adapter
    snippet: studio-rpc-usb-uart
```

## Configuration

