// NpcData.h
//
// Reads NPCs from a small text format, so they can be edited without
// recompiling. Example (see data/npcs.txt for the real file):
//
//     # comment lines start with '#'
//     [npc]
//     name   = Mara
//     tile   = 20, 13        # x, z of the tile they stand on
//     facing = 90            # optional, degrees: 0 = south (+Z), 90 = east (+X)
//     color  = #c0504d       # body color, #RRGGBB
//     line   = Fresh bread, still warm!
//
// Every [npc] block needs name, tile, color and line; facing is optional.
// Unknown keys, repeated keys and bad values are errors, reported with the
// file name and line number, so typos never fail silently.
//
// Comments must be on their own line, because '#' is also used in colors.

#pragma once

#include <istream>
#include <string>
#include <vector>

#include "Npc.h"

namespace game {

// Parses NPCs from `in`. `sourceName` is only used in error messages.
// Throws std::runtime_error ("source:line: problem") on bad input.
std::vector<Npc> parseNpcs(std::istream& in, const std::string& sourceName);

// Opens `path` and parses it. Throws std::runtime_error if the file cannot
// be opened or is invalid.
std::vector<Npc> loadNpcFile(const std::string& path);

} // namespace game
