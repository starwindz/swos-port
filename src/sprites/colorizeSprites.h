#pragma once

void initSpriteColorizer(int res);
void colorizeGameSprites(int res, const TeamGame *topTeam, const TeamGame *bottomTeam);
int getGoalkeeperIndexFromFace(bool topTeam, int face);
