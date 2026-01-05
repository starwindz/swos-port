#pragma once

void stopAllPlayers();
void initPlayerShotChanceTables();
void updatePlayerShotChanceTable(TeamGeneralInfo& team, const Sprite& player);

#ifdef SWOS_TEST
const int16_t * getPlayerShotChanceTable();
int getGoalieShotChanceTableIndex(const int16_t *ptr);
#endif
