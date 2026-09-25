#!/usr/bin/env bash
#
# Build and test the ESP32-DIV firmware for the board selected in
# ESP32-DIV/BoardConfig.h. This is the same sequence the CI workflow runs, so a
# green run here is what CI should produce.
#
#   tools/build-and-test.sh                 # test + build (default)
#   tools/build-and-test.sh --test-only     # host-side pin map checks only
#   tools/build-and-test.sh --install       # install arduino-cli core + libs first
#   tools/build-and-test.sh --dry-run       # print the commands, run nothing
#   tools/build-and-test.sh --upload --port /dev/ttyACM0
#
# Requires bash 3.2+ (macOS default) and g++ for the tests. arduino-cli is only
# needed for the build/upload phases.
set -euo pipefail

# ── paths ────────────────────────────────────────────────────────────────────
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/.." && pwd)"
sketch="$root/ESP32-DIV"
libraries="$root/Libraries"
board_config="$sketch/BoardConfig.h"
core_version="2.0.10"

sketchbook="${ARDUINO_DIRECTORIES_USER:-${ARDUINO_USER_DIR:-$HOME/Arduino}}"
tftespi_dir="$sketchbook/libraries/TFT_eSPI"

# ── options ──────────────────────────────────────────────────────────────────
board=""
do_test=1
do_install=0
do_build=1
do_upload=0
do_sync_usersetup=1
dry_run=0
port=""
fqbn_override=""
output_dir=""

usage() {
  cat <<'EOF'
Build and test the ESP32-DIV firmware for the board selected in
ESP32-DIV/BoardConfig.h. This is the same sequence the CI workflow runs, so a
green run here is what CI should produce.

  tools/build-and-test.sh                 # test + build (default)
  tools/build-and-test.sh --test-only     # host-side pin map checks only
  tools/build-and-test.sh --install       # install arduino-cli core + libs first
  tools/build-and-test.sh --dry-run       # print the commands, run nothing
  tools/build-and-test.sh --upload --port /dev/ttyACM0

Options:
  --board evras3|v2|v1|cyd  Board to build (default: the one enabled in BoardConfig.h)
  --test-only               Run the host-side pin map checks and stop
  --skip-test               Skip the host-side checks
  --install                 Install the esp32 core and libraries with arduino-cli
  --skip-install            Never install (default)
  --dry-run                 Print the commands instead of running them
  --fqbn FQBN               Override the board's default Arduino FQBN
  --output DIR              Build output directory (default build/<board>)
  --upload                  Upload after a successful build
  --port PORT               Serial port for --upload
  --no-sync-usersetup       Do not copy the board's TFT_eSPI User_Setup.h into place
  -h, --help                This help

The tests need a C++ compiler (g++, or set CXX). The build needs arduino-cli.
EOF
}

# ── output helpers ───────────────────────────────────────────────────────────
step() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }
note() { printf '    %s\n' "$*"; }
fail() { printf '\n\033[31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

need_arduino_cli() {
  # In a dry run the commands are only printed, so a missing binary is fine.
  [ "$dry_run" -eq 1 ] && return 0
  command -v arduino-cli >/dev/null 2>&1 && return 0
  fail "$1"
}

run() {
  if [ "$dry_run" -eq 1 ]; then
    printf '    [dry-run] %s\n' "$*"
    return 0
  fi
  "$@"
}

while [ $# -gt 0 ]; do
  case "$1" in
    --board)            [ $# -ge 2 ] || fail "--board needs a value"; board="$2"; shift 2 ;;
    --test-only)        do_build=0; shift ;;
    --skip-test)        do_test=0; shift ;;
    --install)          do_install=1; shift ;;
    --skip-install)     do_install=0; shift ;;
    --dry-run)          dry_run=1; shift ;;
    --fqbn)             [ $# -ge 2 ] || fail "--fqbn needs a value"; fqbn_override="$2"; shift 2 ;;
    --output)           [ $# -ge 2 ] || fail "--output needs a value"; output_dir="$2"; shift 2 ;;
    --upload)           do_upload=1; shift ;;
    --port)             [ $# -ge 2 ] || fail "--port needs a value"; port="$2"; shift 2 ;;
    --no-sync-usersetup) do_sync_usersetup=0; shift ;;
    -h|--help)          usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

[ "$do_build" -eq 1 ] || [ "$do_test" -eq 1 ] || { echo "nothing to do" >&2; exit 2; }
if [ "$do_upload" -eq 1 ] && [ -z "$port" ]; then
  echo "--upload needs --port" >&2; exit 2
fi

# ── board detection ──────────────────────────────────────────────────────────
[ -f "$board_config" ] || fail "missing $board_config"

if [ -z "$board" ]; then
  # "Enabled" = an uncommented `#define BOARD_x` above the fallback block at the
  # bottom of the file. That block re-defines a default board inside
  # `#if !defined(...)`, so it has to be cut off first or it looks like a second
  # selection.
  selection_area="$(sed -n '1,/^#[[:space:]]*if[[:space:]]*!defined(BOARD_/p' "$board_config")"
  selected="$(printf '%s\n' "$selection_area" \
              | grep -E '^[[:space:]]*#[[:space:]]*define[[:space:]]+BOARD_[A-Z0-9_]+' \
              | sed -E 's/.*define[[:space:]]+(BOARD_[A-Z0-9_]+).*/\1/' | sort -u || true)"
  count="$(printf '%s\n' "$selected" | grep -c . || true)"
  if [ "$count" -gt 1 ]; then
    fail "more than one board is enabled in $board_config: $(printf '%s ' $selected)"
  fi
  if [ "$count" -eq 0 ]; then
    # Same default the fallback at the bottom of BoardConfig.h picks.
    selected="BOARD_ESP32_DIV_V2"
    note "no board enabled in BoardConfig.h — using its fallback: $selected"
  fi
  case "$selected" in
    BOARD_EVRAS3)        board="evras3" ;;
    BOARD_ESP32_DIV_V2)  board="v2" ;;
    BOARD_ESP32_DIV_V1)  board="v1" ;;
    BOARD_CYD)           board="cyd" ;;
    *) fail "unknown board macro in $board_config: $selected" ;;
  esac
  note "board from BoardConfig.h: $board ($selected)"
else
  note "board from --board: $board"
fi

case "$board" in
  evras3)
    macro="BOARD_EVRAS3"; chip="esp32s3"; bootloader_addr="0x0"
    user_setup="User_Setup evras3.h"
    default_fqbn="esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,FlashMode=dio,CDCOnBoot=cdc"
    ;;
  v2)
    macro="BOARD_ESP32_DIV_V2"; chip="esp32s3"; bootloader_addr="0x0"
    user_setup="User_Setup v2.h"
    default_fqbn="esp32:esp32:esp32s3:PSRAM=enabled,PartitionScheme=min_spiffs,FlashMode=dio"
    ;;
  v1|cyd)
    # Classic ESP32. This FQBN is a plain dev-module default — the repo has never
    # pinned one for these boards, so override with --fqbn if yours differs.
    [ "$board" = v1 ] && macro="BOARD_ESP32_DIV_V1" || macro="BOARD_CYD"
    chip="esp32"; bootloader_addr="0x1000"
    user_setup="User_Setup $board.h"
    default_fqbn="esp32:esp32:esp32:PSRAM=disabled,PartitionScheme=min_spiffs,FlashMode=dio"
    note "the $board FQBN is a generic dev-module default; use --fqbn to override"
    ;;
  *) fail "unknown board: $board (expected evras3|v2|v1|cyd)" ;;
esac

fqbn="${fqbn_override:-$default_fqbn}"
[ -n "$output_dir" ] || output_dir="$root/build/$board"
user_setup_src="$libraries/$user_setup"

step "Configuration"
note "board        $board ($macro)"
note "chip         $chip"
note "fqbn         $fqbn"
note "user setup   Libraries/$user_setup"
note "output       $output_dir"
[ "$dry_run" -eq 1 ] && note "dry run      commands are printed, not executed"

# ── 1. host-side tests ───────────────────────────────────────────────────────
if [ "$do_test" -eq 1 ]; then
  step "Test: board pin map"
  if ! command -v "${CXX:-g++}" >/dev/null 2>&1; then
    fail "no C++ compiler found (set CXX). The pin map checks need one."
  fi
  if [ "$dry_run" -eq 1 ]; then
    note "[dry-run] bash $here/board-pins/run.sh"
  else
    bash "$here/board-pins/run.sh"
  fi
fi

# ── 2. install toolchain and libraries ───────────────────────────────────────
if [ "$do_install" -eq 1 ]; then
  step "Install: esp32 core $core_version + libraries"
  need_arduino_cli "arduino-cli not found. Install it first: https://arduino.github.io/arduino-cli/latest/installation/"
  run arduino-cli config init --overwrite
  run arduino-cli config add board_manager.additional_urls \
      https://espressif.github.io/arduino-esp32/package_esp32_index.json
  run arduino-cli config set library.enable_unsafe_install true
  run arduino-cli core update-index
  run arduino-cli core install "esp32:esp32@$core_version"
  # The repo ships a patched platform.txt; the CI installs it the same way.
  core_dir="$HOME/.arduino15/packages/esp32/hardware/esp32/$core_version"
  if [ -f "$libraries/platform.txt" ]; then
    run cp "$libraries/platform.txt" "$core_dir/platform.txt"
    note "installed patched platform.txt"
  fi
  run arduino-cli lib install 'PCF8574 library@2.3.7' 'Adafruit PN532@1.3.4' \
      'ArduinoJson@6.18.2' 'TFT_eSPI@2.5.43' 'XPT2046_Touchscreen@1.4' \
      'RF24@1.5.0' 'rc-switch@2.6.4' 'NimBLE-Arduino@1.4.2' \
      'IRremoteESP8266@2.8.6' 'arduinoFFT@1.6.2'
  run arduino-cli lib install --zip-path "$libraries/SmartRC-CC1101-Driver-Lib-master.zip"
fi

# ── 3. put the right TFT_eSPI User_Setup in place ────────────────────────────
# The display controller and its pins live in the library, not the sketch. A
# stale User_Setup.h builds fine and drives the wrong panel, so check it.
if [ "$do_build" -eq 1 ] && [ "$do_sync_usersetup" -eq 1 ]; then
  step "Sync: TFT_eSPI User_Setup.h"
  [ -f "$user_setup_src" ] || fail "missing $user_setup_src"
  if [ "$dry_run" -eq 1 ]; then
    note "[dry-run] cp \"$user_setup_src\" \"$tftespi_dir/User_Setup.h\""
  elif [ ! -d "$tftespi_dir" ]; then
    note "TFT_eSPI is not installed in $sketchbook/libraries — skipping"
    note "run with --install, or copy the library there, then re-run"
  elif cmp -s "$user_setup_src" "$tftespi_dir/User_Setup.h"; then
    note "already up to date"
  else
    if [ -f "$tftespi_dir/User_Setup.h" ]; then
      cp "$tftespi_dir/User_Setup.h" "$tftespi_dir/User_Setup.h.bak"
      note "backed up the previous file to User_Setup.h.bak"
    fi
    cp "$user_setup_src" "$tftespi_dir/User_Setup.h"
    note "installed Libraries/$user_setup"
  fi
fi

# ── 4. build ─────────────────────────────────────────────────────────────────
if [ "$do_build" -eq 1 ]; then
  step "Build: $board"
  if [ "$dry_run" -eq 0 ] && ! command -v arduino-cli >/dev/null 2>&1; then
    note "the host-side tests passed; only the compile step is blocked"
    fail "arduino-cli not found, cannot build. Install it, or re-run with --install."
  fi
  run mkdir -p "$output_dir"
  run arduino-cli compile --fqbn "$fqbn" --output-dir "$output_dir" "$sketch"

  if [ "$dry_run" -eq 0 ]; then
    app="$(ls -1 "$output_dir"/*.ino.bin 2>/dev/null | head -1 || true)"
    if [ -n "$app" ]; then
      note "app image    $app ($(wc -c < "$app" | tr -d ' ') bytes)"
    fi
    step "Flash command"
    cat <<EOF
    esptool.py --chip $chip --port <PORT> write_flash \\
      $bootloader_addr     $output_dir/ESP32-DIV.ino.bootloader.bin \\
      0x8000  $output_dir/ESP32-DIV.ino.partitions.bin \\
      0xe000  \$HOME/.arduino15/packages/esp32/hardware/esp32/$core_version/tools/partitions/boot_app0.bin \\
      0x10000 $output_dir/ESP32-DIV.ino.bin
EOF
  fi
fi

# ── 5. upload ────────────────────────────────────────────────────────────────
if [ "$do_upload" -eq 1 ] && [ "$do_build" -eq 1 ]; then
  step "Upload: $port"
  need_arduino_cli "arduino-cli not found, cannot upload"
  run arduino-cli upload --fqbn "$fqbn" --port "$port" --input-dir "$output_dir" "$sketch"
fi

step "Done"
note "board $board: $([ "$do_test" -eq 1 ] && echo -n 'tests passed' || echo -n 'tests skipped')$([ "$do_build" -eq 1 ] && echo -n ', build ok' || true)$([ "$dry_run" -eq 1 ] && echo ' (dry run)' || true)"
