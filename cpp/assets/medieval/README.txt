Building pieces from the Medieval Village MegaKit (Standard, free version)
by Quaternius, https://quaternius.com. License: CC0 1.0, see
License_Standard.txt (copied unchanged from the kit).

Only the pieces the town uses are copied here, not the whole kit.

Changes from the kit's glTF/ folder:
- Each .gltf keeps only its base-color texture. The normal, roughness and
  ORM maps are removed from the file, because raylib's default shader (no
  lighting) never uses them and they would only take up space and memory.
- The base-color PNGs are resized from 2048x2048 to 512x512. From the game
  camera the difference is not visible.
- The .bin files (geometry) are unchanged.

To add a piece: copy its .gltf and .bin from the kit's glTF/ folder, remove
the non-base-color textures in the same way, and copy any new base-color
PNG resized to 512x512.
