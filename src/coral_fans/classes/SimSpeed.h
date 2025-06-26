#ifndef TICKSPEED_H
#define TICKSPEED_H

#include <string>
#include <chrono>
#include "ll/api/i18n/I18n.h"
#include "mc/server/commands/CommandOutput.h"

using ll::i18n_literals::operator""_tr;

class SimSpeed {
public:
    static int sprintTicksGoal;
    static std::chrono::time_point<std::chrono::system_clock> sprintStartDate;
    static CommandOutput* sprintOutput;

    static void getRate();
    static void setRate(float rate);
    static void freeze();
    static void unfreeze();
    static void step(int ticks);
    static void sprint(CommandOutput& output, int ticks);
    static void finishSprint();
    static bool shouldFinishSprint();
    static bool isFrozen();
    static bool isSprinting();
    static bool isStepping();
    static void onTick();
    static void onPlayerQuit(endstone::PlayerQuitEvent &event);
};

#endif //TICKSPEED_H
