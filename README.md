# srig-examples

Example firmware and hardware-in-the-loop tests that run on real boards through
[SiliconRig](https://siliconrig.dev).

## Start without a toolchain

Every release ships prebuilt demo images, so the first thing you run needs no
compiler and no board on your desk:

```bash
curl -fsSL https://siliconrig.dev/install.sh | sh
export SRIG_API_KEY=key_...

curl -LO https://github.com/raws-labs/srig-examples/releases/latest/download/demo-shell-esp32-s3.bin
srig run demo-shell-esp32-s3.bin --board esp32-s3 --expect "0 failed"
```

That reserves a board, flashes it, watches the serial output, and exits 0 when
the firmware reports its self test passed. Swap the board and the image for
`stm32-h753`, `stm32-f446` or `rp2350`.

The same thing as a test suite, four chips in about a minute:

```bash
pip install siliconrig pytest
pytest demo-shell/test -v
```

## Examples

| | what it shows | needs |
|---|---|---|
| [`demo-shell/`](demo-shell) | an interactive serial shell on every board type, and a pytest suite that runs the same assertions on all four | one board, no toolchain |
| [`esp-now-demo/`](esp-now-demo) | two boards talking over ESP-NOW, with assertions on reception, RSSI and packet loss | two esp32-s3 boards, ESP-IDF |
| [`micropython/`](micropython) | MicroPython on every board type, so a session starts on a REPL instead of a blank board | one board; ESP-IDF and arm-none-eabi-gcc to build |
| [`zephyr/`](zephyr) | a Zephyr shell with the kernel, device and hwinfo commands, on the three board types it reaches today | one board; west and the Zephyr SDK to build |

`demo-shell` is also the firmware the boards run when nothing else is loaded, so
a fresh session already answers `help`.

## Writing your own

The pytest fixture is three lines. `Board` reserves a board, flashes it, and
gives you the serial console:

```python
from siliconrig import Board

with Board("stm32-h753", firmware="build/app.bin") as b:
    b.expect("READY", timeout=30)
    b.send("status\n")
    assert "OK" in b.read_until("OK", timeout=5)
```

Firmware that prints one stable line per event is far easier to assert on than
firmware that formats for humans.

## Docs

[Quickstart](https://siliconrig.dev/docs/getting-started/quickstart) |
[Python SDK](https://siliconrig.dev/docs/guides/python-sdk) |
[CI/CD](https://siliconrig.dev/docs/guides/cicd)
