#include "AntiXray.hpp"
#include "BinStream.hpp"

#include <chrono>
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

    // Process queued chunk scans every tick, limited by a time budget so a burst of
    // chunk packets (joining, teleporting, flying) can't freeze the main thread.
    getServer().getScheduler().runTaskTimer(*this, [this]() { processChunkQueue(); }, 1, 1);

    getLogger().info("AntiXray Enabled");
}

void AntiXray::onDisable() {
    getServer().getScheduler().cancelTasks(*this);
    {
        std::lock_guard lock(queueMutex);
        chunkQueue.clear();
        queuedChunks.clear();
    }
    getLogger().info("AntiXray Disabled");
}

void AntiXray::loadConfig() {
    if (std::filesystem::exists("plugins/AntiXray/config.json") && isValidConfig()) {
        getLogger().info("Using loaded config");
    } else {
        getLogger().info("Using default config");

        config = json::parse(R"({
            "ignore_operators": false,
            "excluded_players": [],
            "max_ms_per_tick": 5
        })");

        if (!std::filesystem::exists("plugins/AntiXray/config.json")) {
            std::filesystem::create_directories("plugins/AntiXray");
            std::ofstream out("plugins/AntiXray/config.json");
            out << config.dump(4);
            out.close();
        }
    }

    // Cache values so hot paths don't touch json
    ignoreOperators = config["ignore_operators"].get<bool>();
    excludedPlayers.clear();
    for (const auto& entry : config["excluded_players"]) {
        excludedPlayers.insert(entry.get<std::string>());
    }
    maxMsPerTick = config.value("max_ms_per_tick", 5.0);
    if (maxMsPerTick <= 0) maxMsPerTick = 5.0;
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

        // Optional
        if (j.contains("max_ms_per_tick") && !j["max_ms_per_tick"].is_number()) {
            getLogger().error("Invalid JSON Config. \"max_ms_per_tick\" must be a number");
            return false;
        }

        config = j;

        return true;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }
}

bool AntiXray::isExcluded(const std::string& name) const {
    return excludedPlayers.contains(name);
}

bool AntiXray::shouldIgnore(Player& p) const {
    if (ignoreOperators && p.isOp()) return true;
    return isExcluded(p.getName());
}

bool AntiXray::isOre(const std::string& type) {
    return type.find("ore") != std::string::npos || type.find("debris") != std::string::npos;
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
    if (shouldIgnore(*event.getPlayer())) return;
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
    if (shouldIgnore(event.getPlayer())) return;
    checkBlocks(event.getBlock(), event.getPlayer(), true);
}

void AntiXray::onBreak(BlockBreakEvent &event) {
    if (shouldIgnore(event.getPlayer())) return;
    checkBlocks(event.getBlock(), event.getPlayer(), false);
}

void AntiXray::blockExplode(BlockExplodeEvent& event) {
    // Re-revealing blocks (wasPlaced = false) refreshes them for every client at once,
    // so it only needs to happen once if any relevant player is nearby.
    const auto dimType = event.getBlock().getDimension().getType();
    const auto loc = event.getBlock().getLocation();
    Player* target = nullptr;
    for (auto player : getServer().getOnlinePlayers()) {
        if (player->getDimension().getType() != dimType) continue;
        if (shouldIgnore(*player)) continue;
        if (loc.distance(player->getLocation()) <= 64) {
            target = player;
            break;
        }
    }
    if (!target) return;
    for (auto &block : event.getBlockList()) {
        checkBlocks(*block, *target, false);
    }
}

void AntiXray::actorExplode(ActorExplodeEvent& event) {
    const auto dimType = event.getActor().getDimension().getType();
    const auto loc = event.getLocation();
    Player* target = nullptr;
    for (auto player : getServer().getOnlinePlayers()) {
        if (player->getDimension().getType() != dimType) continue;
        if (shouldIgnore(*player)) continue;
        if (loc.distance(player->getLocation()) <= 64) {
            target = player;
            break;
        }
    }
    if (!target) return;
    for (auto &block : event.getBlockList()) {
        checkBlocks(*block, *target, false);
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
    std::unique_ptr<Block> rel = b.getRelative(offX, offY, offZ);
    const std::string id = rel->getType();
    if (!isOre(id)) return;

    if (wasPlaced) {
        const int x = rel->getX();
        const int y = rel->getY();
        const int z = rel->getZ();
        Dimension* dd = &b.getDimension();
        getServer().getScheduler().runTaskLater(*this, [uuid, dd, x, y, z, this]() {
            auto* pp = getServer().getPlayer(uuid);
            if (!pp) return;
            hideBlock(*dd->getBlockAt(x, y, z), *pp);
        }, 1);
    } else {
        auto data = rel->getData();
        rel->setType("minecraft:air");
        rel->setType(id);
        rel->setData(*data);
    }
}

void AntiXray::hideBlock(Block &b, Player &p) {
    hideBlock(b, p, b.getType());
}

void AntiXray::hideBlock(Block &b, Player &p, const std::string& type) {
    const int x = b.getX();
    const int y = b.getY();
    const int z = b.getZ();

    // Bail out at the first exposed face instead of fetching all 6 neighbours up front
    const Dimension &d = b.getDimension();
    auto isAir = [&d](int bx, int by, int bz) {
        return d.getBlockAt(bx, by, bz)->getType() == "minecraft:air";
    };
    if (isAir(x, y + 1, z) || isAir(x, y - 1, z) || isAir(x + 1, y, z) ||
        isAir(x - 1, y, z) || isAir(x, y, z + 1) || isAir(x, y, z - 1)) {
        return;
    }

    unsigned int id = type.find("deepslate") == std::string::npos ? STONE_ID : DEEPSLATE_ID;
    if (type.find("nether") != std::string::npos || type.find("quartz") != std::string::npos ||
        type.find("debris") != std::string::npos) id = NETHERRACK_ID;

    BinStream bs2 = BinStream(96 + 7);
    bs2.writeVarInt(x);
    bs2.writeVarInt(y);
    bs2.writeVarInt(z);
    bs2.writeUVarInt(id);
    bs2.writeUVarInt(3);
    bs2.writeUVarInt(0);

    p.sendPacket(21, std::string_view(reinterpret_cast<char *>(bs2.getData()), bs2.getPosition()));
}

std::string AntiXray::jobKey(const UUID& uuid, Dimension::Type dim, int cx, int cz) {
    return uuid.str() + ':' + std::to_string(static_cast<int>(dim)) + ':' + std::to_string(cx) + ':' +
           std::to_string(cz);
}

void AntiXray::updateChunk(Player &p, Dimension &d, int cx, int cz) {
    const auto dimType = d.getType();
    if (dimType == Dimension::Type::TheEnd) return; // no ores in the End

    const auto uuid = p.getUniqueId();
    const std::string key = jobKey(uuid, dimType, cx, cz);

    std::lock_guard lock(queueMutex);
    if (auto it = queuedChunks.find(key); it != queuedChunks.end()) {
        // Chunk was re-sent before we finished it: rescan from the start
        it->second->column = 0;
        return;
    }
    chunkQueue.push_back({uuid, dimType, cx, cz, 0});
    queuedChunks.emplace(key, std::prev(chunkQueue.end()));
}

void AntiXray::processChunkQueue() {
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::duration_cast<clock::duration>(
                              std::chrono::duration<double, std::milli>(maxMsPerTick));

    std::lock_guard lock(queueMutex);
    while (!chunkQueue.empty() && clock::now() < deadline) {
        ChunkJob &job = chunkQueue.front();

        Player* pp = getServer().getPlayer(job.player);
        // Drop jobs for players who left or changed dimension (chunks get resent anyway)
        if (!pp || pp->getDimension().getType() != job.dimension) {
            queuedChunks.erase(jobKey(job.player, job.dimension, job.cx, job.cz));
            chunkQueue.pop_front();
            continue;
        }

        Dimension &dim = pp->getDimension();
        const int minY = job.dimension == Dimension::Type::Nether ? 0 : -64;
        const int baseX = job.cx << 4;
        const int baseZ = job.cz << 4;

        // Scan one column at a time so we can stop mid-chunk when out of time
        while (job.column < 256 && clock::now() < deadline) {
            const int x = baseX + (job.column >> 4);
            const int z = baseZ + (job.column & 15);
            job.column++;

            const int maxY = dim.getHighestBlockYAt(x, z);
            for (int y = minY; y <= maxY; y++) {
                std::unique_ptr<Block> b = dim.getBlockAt(x, y, z);
                const std::string type = b->getType();
                if (!isOre(type)) continue;
                hideBlock(*b, *pp, type);
            }
        }

        if (job.column >= 256) {
            queuedChunks.erase(jobKey(job.player, job.dimension, job.cx, job.cz));
            chunkQueue.pop_front();
        }
    }
}
