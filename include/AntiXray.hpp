#pragma once

#include <endstone/endstone.hpp>
#include <nlohmann/json.hpp>

class AntiXray : public endstone::Plugin {
public:
    void onEnable() override;

    void onDisable() override;

    void loadConfig();

    bool isValidConfig();
    bool isExcluded(const std::string& name) const;


    /* ON RECIEVE PACKET METHODS */
    void onRecvPacket(endstone::PacketReceiveEvent& event);


    /* ON SEND PACKET METHODS */
    void onSendPacket(endstone::PacketSendEvent &event);

    void handleLevelChunk(endstone::PacketSendEvent &event);

    void handleUpdateBlock(endstone::PacketSendEvent &event);


    /* UPDATE EVENTS */
    void onPlace(endstone::BlockPlaceEvent& event);

    void onBreak(endstone::BlockBreakEvent& event);

    void blockExplode(endstone::BlockExplodeEvent& event);

    void actorExplode(endstone::ActorExplodeEvent& event);


    /* UPDATE BLOCKS & CHUNKS */
    void checkBlocks(endstone::Block& b, endstone::Player& p, bool wasPlaced);

    void checkBlock(endstone::Block& b, int offX, int offY, int offZ, endstone::UUID uuid, bool wasPlaced);

    void hideBlock(endstone::Block& b, endstone::Player& p);

    void updateChunk(endstone::Player& p, endstone::Dimension& d, int cx, int cz);

private:
    const unsigned int DEEPSLATE_ID = 0x3B4268E2;
    const unsigned int STONE_ID = 0x80310E21;
    const unsigned int NETHERRACK_ID = 0x803389C0;

    nlohmann::json config;
};