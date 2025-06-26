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
#include "coral_fans/classes/TickSpeed.h"

namespace coral_fans::commands {
void registerTickCommand(CommandPermissionLevel permission) {
    using ll::i18n_literals::operator""_tr;

    auto& tickCommand = ll::command::CommandRegistrar::getInstance()
                            .getOrCreateCommand("tick", "command.tick.description"_tr(), permission);

    tickCommand.runtimeOverload()
        .text("query")
        .execute([](CommandOrigin const&, CommandOutput& output, ll::command::RuntimeCommand const&) {
            std::string message;
            if (SimSpeed::isSprinting())
                message += "command.tick.sprint"_tr();
            else if (SimSpeed::isFrozen())
                message += "command.tick.freeze"_tr();
            else
                message += "command.tick.unfreeze"_tr();
            message += "\n" + "command.tick.query.rate"_tr(SimSpeed::getRate());
            if (SimSpeed::isSprinting() || SimSpeed::isFrozen())
                message += " " + "command.tick.query.ratenotapplicable"_tr();
            message += "\n";
            message += "command.tick.query.mspt"_tr(ll::service::getBedrock().getMspt()); // does this service exist lol
            if (!SimSpeed::isSprinting())
                message += " " + "command.tick.query.targetmspt"_tr(1000.0f / SimSpeed::getRate());
            message += "\n";
            output.success(message);
        });

    // tick freeze|unfreeze
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
            bool pause = false;
            const auto val = self["tickFreezeType"].get<ll::command::ParamKind::Enum>();
            switch (val.index) {
                case 0:
                    SimSpeed::unfreeze();
                    output.success("command.tick.unfreeze"_tr());
                    break;
                case 1:
                    SimSpeed::freeze();
                    output.success("command.tick.freeze"_tr());
                    break;
            }
            // LevelEventPacket{LevelEvent::SimTimeStep, origin.getWorldPosition(), pause}.sendToClients();

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
