"""The same firmware, the same assertions, on four different chips.

Needs no toolchain: the images come from this repo's latest release. Point
SRIG_FIRMWARE_DIR at a local build to run your own instead.

    pip install siliconrig pytest
    export SRIG_API_KEY=key_...
    pytest demo-shell/test -v
"""

import os
import re
import urllib.request
from pathlib import Path

import pytest
from siliconrig import Board

BOARDS = ["esp32-s3", "stm32-h753", "stm32-f446", "rp2350"]
RELEASE = "https://github.com/raws-labs/srig-examples/releases/latest/download"
FIRMWARE_DIR = Path(os.environ.get("SRIG_FIRMWARE_DIR") or Path(__file__).parent / ".firmware")


def firmware(board_type: str) -> str:
    """The published image for this board, downloaded once if it is not here."""
    name = f"demo-shell-{board_type}." + ("uf2" if board_type == "rp2350" else "bin")
    path = FIRMWARE_DIR / name
    if not path.exists():
        FIRMWARE_DIR.mkdir(parents=True, exist_ok=True)
        urllib.request.urlretrieve(f"{RELEASE}/{name}", path)
    return str(path)


@pytest.fixture(scope="module", params=BOARDS)
def board(request):
    board_type = request.param
    with Board(board_type, firmware=firmware(board_type)) as b:
        # The heartbeat rather than the banner: a board whose USB console is the
        # chip itself re-enumerates after a flash, so the banner can be gone
        # before anything is listening. The heartbeat repeats until the first
        # keypress.
        b.expect(f"[hb] {board_type}", timeout=90)
        b.flush()
        yield b


def test_selftest_passes(board):
    """Every on-chip check the firmware can run, reported as an exit code."""
    board.send("selftest\n")
    board.expect("##srig-exit:0##", timeout=30)


def test_reports_its_own_identity(board):
    """The chip id and sizes are read off the device, not compiled in."""
    board.send("id\n")
    out = board.expect("ram=", timeout=10)
    assert "uid=" in out and "flash=" in out, out
    uid = re.search(r"uid=([0-9A-F]+)", out)
    assert uid and set(uid.group(1)) != {"0"}, out


def test_bench_reports_a_rate(board):
    """A number that has to come from the silicon actually running code."""
    board.send("bench\n")
    out = board.expect("iters/s", timeout=30)
    rate = re.search(r"-> (\d+) iters/s", out)
    assert rate, out
    assert int(rate.group(1)) > 0, out
