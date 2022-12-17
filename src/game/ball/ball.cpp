#include "ball.h"

// TODO: remove |stateGoal|
void checkIfBallOutOfPlay()
{
     swos.stateGoal = 0;
     bool playRefereeWhistle = false;

     if (swos.gameStatePl != GameState::kStopped) {
         ;
     }

     if (playRefereeWhistle)
         SWOS::PlayRefereeWhistleSample();
}
