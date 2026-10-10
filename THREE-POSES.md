# Emerald: three-pose prototype source

Status: ROM built successfully on GitHub Actions on October 10, 2026.
The compiled pixel tests and a headless mGBA boot/input test passed.
Download the emerald-three-poses artifact from:
https://github.com/Camaro45-lab/Emerald/actions/runs/38088323967
Full battle, save/load and special-form playtesting remains outstanding.

Base: https://github.com/pret/pokeemerald
Pinned source commit: c587cc9e63f57ecc688adc00513499fc7f747ac6

## Intended behavior

All 386 Emerald species receive a short sequence of three front/back poses:
original, crouch, narrow recoil, then original. This precedes the existing
battle-entry or summary-screen animation. These are simple pixel deformations,
not individually redrawn limb poses. Normal animation settings still apply.

The game generates two temporary 64x64, 4-bit frames from the already-loaded
sprite. This keeps palettes, shiny colors, Spinda spots, Unown letters and the
currently selected Castform/Deoxys form. It does not add newer generations,
new species, new gameplay, or three walking/party-menu icons.

Temporary allocation is about 4.2 KB per simultaneously animating Pokemon.
If allocation fails, the existing animation runs. Sprite destruction cancels
pending copies and releases the allocation. Heap reset clears stale owners.
Save structures are unchanged, but save compatibility has NOT been playtested.

## Build on GitHub Actions

1. Put the contents of this source folder in a GitHub repository you control.
   Include the hidden .github folder. A private repository is suitable.
2. Open the repository's **Actions** tab.
3. Select **Build three-pose Emerald**, then **Run workflow**.
4. After a successful run, download the **emerald-three-poses** artifact.
5. Extract emerald-three-poses.gba and open it in a GBA emulator such as mGBA.

The workflow runs on pushes to main or manually, has read-only repository
permissions, compiles and tests the pixel routine, builds the ROM and runs a
headless mGBA smoke test. A green build does not substitute for the gameplay
checks below.

## Build on Linux

Use the upstream INSTALL.md instructions. Required packages include
build-essential, binutils-arm-none-eabi, libpng-dev and python3-pil.
Clone pret/agbcc separately, check out
da598c1d918402c42c0c0d7128ba14567f3175e9, run ./build.sh and install it into this
repository with ./install.sh /absolute/path/to/this/repository.

Then run:

    python3 tests/test_three_poses.py
    make -j2

The result should be pokeemerald.gba. Do not use `make compare`: this modified
game intentionally differs from the original ROM checksum.

## Validation so far

- Host GCC syntax checks passed for all five new/modified C files, using
  upstream headers and a temporary map-header stand-in (these modules do not
  reference map IDs). This is not ARM compilation or linking.
- reference-audit.json records a Python reference-model asset coverage audit.
  It does not certify execution of the actual C code.
- tests/test_three_poses.py passed for 1,214 nonblank source frames across 830
  front/back sheets, checking distinct buffers, palette indices and output
  buffer canaries with the actual compiled C routine.
- mGBA executed 4,200 frames with simulated Start/A input. Captured screens
  show the title sequence and Professor Birch's introduction.
- Battle, save/load, special-form and memory-pressure playtests are pending.

Before treating this prototype as fully tested, check a new game, starter battle, double battle,
rapid summary navigation and closing it mid-animation, evolution, catching,
save/load, all Castform weather forms, Deoxys, Spinda and Unown.

## Implementation

src/three_pose_pixels.c: pure tile transformation.
src/three_pose.c: temporary frames, playback, restoration and lifetime.
src/pokemon_animation.c: battle/summary entry hooks.
src/sprite.c: cleanup and cancellation of queued copies.
src/malloc.c: clear owners when resetting the heap.

The original PNG assets remain untouched. Existing animation continues after
the extra sequence. Expanding to five poses later requires defining two more
deformations and budgeting their frame storage.
