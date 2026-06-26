#include "AntiXray.hpp"

using namespace endstone;

void AntiXray::onEnable() {
    getLogger().info("EndstoneAntiXray Enabled");
}

void AntiXray::onDisable() {
    getLogger().info("EndstoneAntiXray Disabled");
}