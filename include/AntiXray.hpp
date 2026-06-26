#pragma once

#include <endstone/plugin/plugin.h>

class AntiXray : public endstone::Plugin {
public:
    void onEnable() override;

    void onDisable() override;
};