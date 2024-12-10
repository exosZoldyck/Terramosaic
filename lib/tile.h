#ifndef TILE_H
#define TILE_H

#pragma once

struct Tile{
    int pixelId;
    int palletId;
    double closestDeltaE = 9007199254740991;
};

#endif