#pragma once

enum class GoalType {
    kRegular,
    kPenalty,
    kOwnGoal,
};

void resetResult(const char *team1Name, const char *team2Name);
void showAndPositionResult();
void registerScorer(const Sprite& scorer, int teamNum, GoalType goalType);
void drawScorers(int teamNum);
void drawTeamName(int teamNum);
