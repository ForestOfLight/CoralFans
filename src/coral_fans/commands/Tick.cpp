#include "coral_fans/base/MySchedule.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/runtime/ParamKind.h"
#include "ll/api/command/runtime/RuntimeCommand.h"
#include "ll/api/command/runtime/RuntimeOverload.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/service/Bedrock.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandRegistry.h"
#include "mc/util/Timer.h"
#include "mc/world/Minecraft.h"

namespace coral_fans::commands {
class TickSpeed {
public:
    static int sprintTicksGoal = 0;
    static bool shouldInterruptSprint = false;
    static std::chrono::time_point<std::chrono::system_clock> sprintStartDate;

    static void setRate(float rate) {
        unfreeze();
        if (rate <= 0) {
            rate = 0.0;
        }
        auto mc = ll::service::getMinecraft();
        mc->setSimTimeScale(rate / 20.0f);
    }

    static void freeze() {
        auto mc = ll::service::getMinecraft();
        mc->setSimTimePause(false);
    }

    static void unfreeze() {
        auto mc = ll::service::getMinecraft();
        mc->setSimTimePause(true);
    }

    static void step(int ticks) {
        auto mc = ll::service::getMinecraft();
        if (ticks <= 0) {
            ticks = 0;
        }
        mc->mSimTimer.advanceTime(ticks);
    }

    static std::string sprint(CommandOutput& output, int ticks) {
        auto mc = ll::service::getMinecraft();
        if (!mc.has_value()) {
            output.error("command.tick.sprint.error.generic"_tr());
            return;
        }
        if (ticks == 0) {
            if (isSprinting()) {
                finishSprint(CommandOutput& output);
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
        output.success("command.tick.sprint.success.start"_tr(ticks));
    }

    static void finishSprint(CommandOutput& output) {
        int completedTicks = sprintTicksGoal;
        double msToCompletion = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - sprintStartDate).count();
        if (msToCompletion == 0.0)
            msToCompletion = 1.0;
        int tps = static_cast<int>(1000.0 * completedTicks / msToCompletion);
        double mspt = (1.0 * msToCompletion) / completedTicks;
        sprintTicksGoal = 0;
        output.success("command.tick.sprint.success.complete"_tr(tps, mspt));
    }

    static bool isFrozen() {
        auto mc = ll::service::getMinecraft();
        return mc->getSimPaused();
    }

    static bool shouldFinishSprint() {
        auto mc = ll::service::getMinecraft();
        return mc->mSimTimer.mSteppingTick == 0 && sprintTicksGoal != 0;
    }

    static bool isSprinting() {
        auto mc = ll::service::getMinecraft();
        return mc->mSimTimer.mSteppingTick > 0;
    }

    static bool isStepping() { // This should probably be different from isSprinting, but they'll function the same for now.
        auto mc = ll::service::getMinecraft();
        return mc->mSimTimer.mSteppingTick > 0;
    }

    static bool onTick() {
        if (shouldFinishSprint()) {
            finishSprint(CommandOutput& output);
        }
    }

    static void onPlayerQuit(endstone::PlayerQuitEvent &event) {
        // If no players are online, unfreeze the tick speed. Otherwise, players won't be able to join.
    }
};

void registerTickCommand(CommandPermissionLevel permission) {
    using ll::i18n_literals::operator""_tr;

    // reg cmd
    auto& tickCommand = ll::command::CommandRegistrar::getInstance()
                            .getOrCreateCommand("tick", "command.tick.description"_tr(), permission);

    // tick freeze|reset
    ll::command::CommandRegistrar::getInstance().tryRegisterRuntimeEnum(
        "tickFreezeType",
        {
            {"unfreeze",  0},
            {"freeze", 1}
        }
    );
    tickCommand.runtimeOverload()
        .required("tickFreezeType", ll::command::ParamKind::Enum, "tickFreezeType")
        .execute([&](CommandOrigin const&, CommandOutput& output, ll::command::RuntimeCommand const& self) {
            bool       pause = false;
            const auto val   = self["tickFreezeType"].get<ll::command::ParamKind::Enum>();
            switch (val.index) {
            case 1:
                pause = true;
                break;
            case 0:
                pause = false;
                break;
            }
            // LevelEventPacket{LevelEvent::SimTimeStep, origin.getWorldPosition(), pause}.sendToClients();
            auto mc = ll::service::getMinecraft();
            if (mc.has_value()) mc->setSimTimePause(pause);
            output.success("command.tick.set.output"_tr(val.name));
        });

    // tick rate <float>
    tickCommand.runtimeOverload()
        .text("rate")
        .required("rate", ll::command::ParamKind::Float)
        .execute([](CommandOrigin const&, CommandOutput& output, ll::command::RuntimeCommand const& self) {
            float minRate = 0f;
            float rate = self["rate"].get<ll::command::ParamKind::Float>();
            if (rate <= minRate)
                output.error("command.tick.rate.error.outofrange"_tr(minRate, rate));
            // LevelEventPacket{LevelEvent::SimTimeScale, {rate / 20}, rate > 0}.sendToClients();

            auto mc = ll::service::getMinecraft();
            if (mc.has_value()) {
                mc->setSimTimePause(false);
                mc->setSimTimeScale(rate / 20.0f);
                output.success("command.tick.rate.success"_tr(rate));
            } else {
                output.error("command.tick.rate.error.generic"_tr());
            }
        });

    // tick step <int>
    tickCommand.runtimeOverload()
        .text("step")
        .required("time", ll::command::ParamKind::Int)
        .execute([&](CommandOrigin const&, CommandOutput& output, ll::command::RuntimeCommand const& self) {
            int tick = self["time"].get<ll::command::ParamKind::Int>();
            if (!::Command::validRange(tick, 0, INT_MAX, output)) {
                return;
            }
            auto mc = ll::service::getMinecraft();
            if (mc.has_value()) mc->mSimTimer.mSteppingTick = (float)tick;
            output.success("command.tick.step.output"_tr(tick));
        });
}
} // namespace coral_fans::commands
