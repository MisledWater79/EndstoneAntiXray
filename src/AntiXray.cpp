#include "AntiXray.hpp"
#include "BinStream.hpp"

#include <fstream>
#include <filesystem>

using namespace endstone;

using json = nlohmann::json;

void AntiXray::onEnable() {
    registerEvent(&AntiXray::onSendPacket, *this);
    registerEvent(&AntiXray::onRecvPacket, *this);
    registerEvent(&AntiXray::onBreak, *this);
    registerEvent(&AntiXray::onPlace, *this);
    registerEvent(&AntiXray::blockExplode, *this);
    registerEvent(&AntiXray::actorExplode, *this);

    loadConfig();

    getLogger().info("AntiXray Enabled");
}

void AntiXray::onDisable() {
    getLogger().info("AntiXray Disabled");
}

void AntiXray::loadConfig() {
    if (std::filesystem::exists("plugins/AntiXray/config.json")) {
        if (isValidConfig()) return getLogger().info("Using loaded config");
    }

    getLogger().info("Using default config");

    config = json::parse(R"({
        "ignore_operators": false,
        "excluded_players": []
    })");

    if (!std::filesystem::exists("plugins/AntiXray/config.json")) {
        std::filesystem::create_directories("plugins/AntiXray");
        std::ofstream out("plugins/AntiXray/config.json");
        out << config.dump(4);
        out.close();
    }
}

bool AntiXray::isValidConfig() {
    std::ifstream f("plugins/AntiXray/config.json");
    if (!f.is_open()) return false;
    try {
        nlohmann::json j;
        f >> j;

        if (!j.contains("ignore_operators") || !j["ignore_operators"].is_boolean()) {
            getLogger().error("Invalid JSON Config. No \"ignore_operators\" boolean");
            return false;
        }

        if (!j.contains("excluded_players") || !j["excluded_players"].is_array()) {
            getLogger().error("Invalid JSON Config. No \"excluded_players\" array");
            return false;
        }

        for (const auto& id : j["excluded_players"]) {
            if (!id.is_string()) {
                getLogger().error("Invalid JSON Config. Non-string in excluded_players");
                return false;
            }
        }

        config = j;

        return true;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }
}

bool AntiXray::isExcluded(const std::string& name) const {
    for (const auto& entry : config["excluded_players"]) {
        if (entry.get<std::string>() == name) return true;
    }
    return false;
}


void AntiXray::onRecvPacket(PacketReceiveEvent &event) {
    switch (event.getPacketId()) {
        default:
            break;
    }
}

void AntiXray::onSendPacket(PacketSendEvent &event) {
    switch (event.getPacketId()) {
        case 21:
            //handleUpdateBlock(event);
            break;
        case 58:
            handleLevelChunk(event);
            break;
        default:
            break;
    }
}

void AntiXray::handleLevelChunk(PacketSendEvent &event) {
    if (config["ignore_operators"] && event.getPlayer()->isOp()) return;
    if (isExcluded(event.getPlayer()->getName())) return;
    BinStream bs = BinStream(event.getPayload());

    int cx = bs.readVarInt();
    int cz = bs.readVarInt();

    updateChunk(*event.getPlayer(), event.getPlayer()->getDimension(), cx, cz);
}

void AntiXray::handleUpdateBlock(PacketSendEvent &event) {
    BinStream bin = BinStream(event.getPayload());
    bin.readVarInt();
    bin.readVarInt();
    bin.readVarInt();
    getLogger().info("Update Block");
    getLogger().info("ID: {}", bin.readUVarInt());
}


/* UPDATE EVENTS */
void AntiXray::onPlace(BlockPlaceEvent &event) {
    if (config["ignore_operators"] && event.getPlayer().isOp()) return;
    if (isExcluded(event.getPlayer().getName())) return;
    checkBlocks(event.getBlock(), event.getPlayer(), true);
}

void AntiXray::onBreak(BlockBreakEvent &event) {
    if (config["ignore_operators"] && event.getPlayer().isOp()) return;
    if (isExcluded(event.getPlayer().getName())) return;
    checkBlocks(event.getBlock(), event.getPlayer(), false);
}

void AntiXray::blockExplode(BlockExplodeEvent& event) {
    for (auto player : this->getServer().getOnlinePlayers()) {
        if (config["ignore_operators"] && player->isOp()) continue;
        if (isExcluded(player->getName())) continue;
        if (player->getDimension().getName() != event.getBlock().getDimension().getName()) continue;
        if (event.getBlock().getLocation().distance(player->getLocation()) <= 64) {
            for (auto &block : event.getBlockList()) {
                checkBlocks(*block, *player, false);
            }
        }
    }
}

void AntiXray::actorExplode(ActorExplodeEvent& event) {
    for (auto player : this->getServer().getOnlinePlayers()) {
        if (config["ignore_operators"] && player->isOp()) continue;
        if (isExcluded(player->getName())) continue;
        if (player->getDimension().getName() != event.getActor().getDimension().getName()) continue;
        if (event.getLocation().distance(player->getLocation()) <= 64) {
            for (auto &block : event.getBlockList()) {
                checkBlocks(*block, *player, false);
            }
        }
    }
}


/* UPDATE BLOCKS & CHUNKS */
void AntiXray::checkBlocks(Block &b, Player &p, bool wasPlaced) {
    auto uuid = p.getUniqueId();

    checkBlock(b, 1, 0, 0, uuid, wasPlaced);
    checkBlock(b, -1, 0, 0, uuid, wasPlaced);
    checkBlock(b, 0, 1, 0, uuid, wasPlaced);
    checkBlock(b, 0, -1, 0, uuid, wasPlaced);
    checkBlock(b, 0, 0, 1, uuid, wasPlaced);
    checkBlock(b, 0, 0, -1, uuid, wasPlaced);
}

void AntiXray::checkBlock(Block& b, int offX, int offY, int offZ, UUID uuid, bool wasPlaced) {
    const int x = b.getX() + offX;
    const int y = b.getY() + offY;
    const int z = b.getZ() + offZ;

    const Dimension& dim = b.getDimension();

    auto id = dim.getBlockAt(x, y, z)->getType();
    if (id.find("ore") != std::string::npos || id.find("debris") != std::string::npos) {
        if (wasPlaced) {
            getServer().getScheduler().runTaskLater(*this, [uuid, dd = &dim, x, y, z, this]() {
                auto* pp = getServer().getPlayer(uuid);
                if (!pp) return;
                hideBlock(*dd->getBlockAt(x, y, z), *pp);
            }, 1);
        } else {
            auto data = dim.getBlockAt(x, y, z)->getData();
            b.getRelative(offX, offY, offZ)->setType("minecraft:air");
            b.getRelative(offX, offY, offZ)->setType(id);
            b.getRelative(offX, offY, offZ)->setData(*data);
        }
    }
}

void AntiXray::hideBlock(Block &b, Player &p) {
    unsigned int id = b.getType().find("deepslate") == std::string::npos ? STONE_ID : DEEPSLATE_ID;
    if (b.getType().find("nether") != std::string::npos || b.getType().find("quartz") != std::string::npos || b.
        getType().find("debris") != std::string::npos) id = NETHERRACK_ID;

    const int x = b.getX();
    const int y = b.getY();
    const int z = b.getZ();

    Dimension &d = b.getDimension();
    std::unique_ptr<Block> b1, b2, b3, b4, b5, b6;

    b1 = d.getBlockAt(x + 1, y, z);
    b2 = d.getBlockAt(x - 1, y, z);
    b3 = d.getBlockAt(x, y + 1, z);
    b4 = d.getBlockAt(x, y - 1, z);
    b5 = d.getBlockAt(x, y, z + 1);
    b6 = d.getBlockAt(x, y, z - 1);

    if (b1->getType() == "minecraft:air") return;
    if (b2->getType() == "minecraft:air") return;
    if (b3->getType() == "minecraft:air") return;
    if (b4->getType() == "minecraft:air") return;
    if (b5->getType() == "minecraft:air") return;
    if (b6->getType() == "minecraft:air") return;

    BinStream bs2 = BinStream(96 + 7);
    bs2.writeVarInt(x);
    bs2.writeVarInt(y);
    bs2.writeVarInt(z);
    bs2.writeUVarInt(id);
    bs2.writeUVarInt(3);
    bs2.writeUVarInt(0);

    p.sendPacket(21, std::string_view(reinterpret_cast<char *>(bs2.getData()), bs2.getPosition()));
}

void AntiXray::updateChunk(Player &p, Dimension &d, int cx, int cz) {
    auto uuid = p.getUniqueId();
    getServer().getScheduler().runTaskLater(*this, [uuid, dd = &d, cx, cz, this]() {
        auto* pp = getServer().getPlayer(uuid);
        if (!pp) return;
        for (int x = (cx << 4) - 1; x < (cx << 4) + 18; x++) {
            for (int z = (cz << 4) - 1; z < (cz << 4) + 18; z++) {
                int my = dd->getHighestBlockYAt(x, z);
                for (int y = -64; y <= my; y++) {
                    std::unique_ptr<Block> b = dd->getBlockAt(x, y, z);
                    if (b->getType().find("ore") == std::string::npos && b->getType().find("debris") ==
                        std::string::npos) continue;
                    hideBlock(*b, *pp);
                }
            }
        }
    }, 1);
}