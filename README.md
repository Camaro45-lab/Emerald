# Pokémon Emerald: three-pose prototype

Adds an original, crouch and recoil pose to Pokémon front/back battle-entry
and summary animations, then resumes the existing animation. The extra poses
are generated pixel deformations, not hand-redrawn artwork.

[Download the successful build artifact](https://github.com/Camaro45-lab/Emerald/actions/runs/38088323967)
and open `emerald-three-poses.gba` in a GBA emulator.

Compiled sprite tests and an mGBA boot/input smoke test passed. Full gameplay
testing is still pending. See [THREE-POSES.md](THREE-POSES.md) for details.

## Upstream project

This is a decompilation of Pokémon Emerald.

It builds the following ROM:

* [**pokeemerald.gba**](https://datomatic.no-intro.org/index.php?page=show_record&s=23&n=1961) `sha1: f3ae088181bf583e55daf962a92bb46f4f1d07b7`

To set up the repository, see [INSTALL.md](INSTALL.md).

For contacts and other pret projects, see [pret.github.io](https://pret.github.io/).
