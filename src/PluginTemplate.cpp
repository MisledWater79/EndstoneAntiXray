#include "PluginTemplate.hpp"

using namespace endstone;

void PluginTemplate::onEnable() {
    getLogger().info("Plugin Template Enabled");
}

void PluginTemplate::onDisable() {
    getLogger().info("Plugin Template Disabled");
}