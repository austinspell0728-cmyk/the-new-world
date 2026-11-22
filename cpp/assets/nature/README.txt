Trees from the Stylized Nature MegaKit (Standard, free version) by
Quaternius, https://quaternius.com. License: CC0 1.0, see
License_Standard.txt (copied unchanged from the kit).

Only the trees the town uses are copied here, not the whole kit:
CommonTree_1, CommonTree_2, CommonTree_3, CommonTree_5, Pine_1, Pine_3.

Changes from the kit's glTF/ folder:
- Each .gltf keeps only its base-color textures. The normal maps are
  removed from the file, because raylib's default shader (no lighting)
  never uses them.
- The base-color PNGs are resized to 512x512 (bark from 2048, leaves from
  1024 and 2048), keeping their alpha: the leaves are cut-out cards, and
  the game discards their see-through pixels (see platform/MergedModels).
- The .bin files (geometry) are unchanged.

To add a tree: copy its .gltf and .bin from the kit's glTF/ folder, remove
the non-base-color textures in the same way (see the building kit's
README in assets/medieval/), and copy any new base-color PNG resized to
512x512.
