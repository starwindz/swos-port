#pragma once

enum class BenchState
{
    kInitial,
    kAboutToSubstitute,
    kFormationMenu,
    kMarkingPlayers,
    kOpponentsBench,
};

void initBenchControls();
bool benchCheckControls();
BenchState getBenchState();
bool trainingTopTeam();
void setTrainingTopTeam(bool value);
int getBenchPlayerIndex();
int getBenchPlayerShirtNumber(bool topTeam, int index);
int getBenchMenuSelectedPlayer();
int getSelectedFormationEntry();
bool inBenchOrGoingTo();
bool goingToBenchDelay();
int playerToEnterGameIndex();
int playerToBeSubstitutedIndex();
int playerToBeSubstitutedPos();
bool substituteInProgress();
bool newPlayerAboutToGoIn();
void setSubstituteInProgress();
const TeamGeneralInfo *getBenchTeam();
const TeamGame *getBenchTeamData();
