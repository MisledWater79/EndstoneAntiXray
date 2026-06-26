#pragma once

#include <endstone/plugin/plugin.h>

class PluginTemplate : public endstone::Plugin {
public:
    void onEnable() override;

    void onDisable() override;
};