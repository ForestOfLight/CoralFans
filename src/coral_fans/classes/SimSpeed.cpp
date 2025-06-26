#include "SimSpeed.h"
#include "ll/api/service/Bedrock.h"
#include "mc/util/Timer.h"
#include "mc/world/Minecraft.h"

float TICKS_PER_SECOND = 20.0f;

int SimSpeed::sprintTicksGoal = 0;
std::chrono::time_point<std::chrono::system_clock> SimSpeed::sprintStartDate;
CommandOutput* SimSpeed::sprintOutput = nullptr;

void SimSpeed::getRate() {
    auto mc = ll::service::getMinecraft();
    float rate = mc->getSimTimeScale() * TICKS_PER_SECOND;
    return rate;
}

void SimSpeed::setRate(float rate) {
    SimSpeed::unfreeze();
    if (rate <= 0) {
        rate = 0.0;
    }
    auto mc = ll::service::getMinecraft();
    mc->setSimTimeScale(rate / TICKS_PER_SECOND);
}

void SimSpeed::freeze() {
    auto mc = ll::service::getMinecraft();
    mc->setSimTimePause(false);
}

void SimSpeed::unfreeze() {
    auto mc = ll::service::getMinecraft();
    mc->setSimTimePause(true);
}

void SimSpeed::step(int ticks) {
    auto mc = ll::service::getMinecraft();
    if (ticks <= 0) {
        ticks = 0;
    }
    mc->mSimTimer.advanceTime(ticks);
}

void SimSpeed::sprint(CommandOutput& output, int ticks) {
    auto mc = ll::service::getMinecraft();
    if (!mc.has_value()) {
        output.error("command.tick.sprint.error.generic"_tr());
        return;
    }
    if (ticks == 0) {
        if (isSprinting()) {
            sprintOutput = &output;
            SimSpeed::finishSprint();
            output.success("command.tick.sprint.success.interrupt"_tr());
        }
        return;
    }
    if (isSprinting()) {
        output.error("command.tick.sprint.error.sprinting"_tr());
        return;
    }
    sprintStartDate = std::chrono::system_clock::now();
    mc->mSimTimer.advanceTime(ticks);
    sprintTicksGoal = ticks;
    sprintOutput = &output;
    output.success("command.tick.sprint.success.start"_tr(ticks));
}

void SimSpeed::finishSprint() {
    int completedTicks = sprintTicksGoal;
    double msToCompletion = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - sprintStartDate).count();
    if (msToCompletion == 0.0)
        msToCompletion = 1.0;
    int tps = static_cast<int>(1000.0 * completedTicks / msToCompletion);
    double mspt = (1.0 * msToCompletion) / completedTicks;
    sprintTicksGoal = 0;
    if (sprintOutput) {
        sprintOutput->success("command.tick.sprint.success.complete"_tr(tps, mspt));
    }
    sprintOutput = nullptr;
}

bool SimSpeed::isFrozen() {
    auto mc = ll::service::getMinecraft();
    return mc->getSimPaused();
}

bool SimSpeed::shouldFinishSprint() {
    auto mc = ll::service::getMinecraft();
    return mc->mSimTimer.mSteppingTick == 0 && sprintTicksGoal != 0;
}

bool SimSpeed::isSprinting() {
    auto mc = ll::service::getMinecraft();
    return mc->mSimTimer.mSteppingTick > 0;
}

bool SimSpeed::isStepping() { // This should probably be different from isSprinting, but they'll function the same for now.
    auto mc = ll::service::getMinecraft();
    return mc->mSimTimer.mSteppingTick > 0;
}

void SimSpeed::onTick() {
    if (shouldFinishSprint())
        SimSpeed::finishSprint();
}

void SimSpeed::onPlayerQuit(endstone::PlayerQuitEvent &event) {
    // If no players are online, unfreeze the tick speed. Otherwise, players won't be able to join.
}