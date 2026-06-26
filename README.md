# Endstone AntiXray
![GitHub Downloads (specific asset, all releases)](https://img.shields.io/github/downloads/MisledWater79/EndstoneAntiXray/total?label=Downloads)
![](https://img.shields.io/badge/language-c++-blue.svg?label=Language)
![GitHub License](https://img.shields.io/github/license/MisledWater79/EndstoneAntiXray?label=License)

An Endstone plugin to prevent xraying on servers.

## ⚠️Warning⚠️
It is recommended you turn off `client-side-chunk-generation` in your `server.properties`.
Without this turned off this plugin will not work.

### Installation
**Supported Endstone versions:**

| Endstone Version | Full Support | Partial Support | No Support |
|:----------------:|:------------:|:---------------:|:----------:|
|    <= 0.11.0     |      ❌       |        ❌        |     ✅      |
|      0.11.4      |      ✅       |        ❌        |     ❌      |

Make your way to the [Releases](https://github.com/MisledWater79/EndstoneAntiXray/releases) page and download the respective version, and file format.
Use `.dll` for windows servers and `.so` for linux servers.

Once downloaded place the file into your `/plugins/` folder in your server.

Now start the server (or use `/reload`).

### Config

After installing the plugin, a `config.json` file will appear in the `/plugins/EndstoneAntiXray/` folder.
Here you can manage a couple configurable settings.

The format for this file is as such:
```json
{
  "ignore_operators": false,
  "excluded_players": [
    "MisledWater79"
  ]
}
```

---

## Support & Contribution
I don't like asking for money; and all my projects will always be free, so for those who like my work and would like to support you can:
* Buy me a [Coffee](https://ko-fi.com/misledwater79).

### Contribution
If you would like to help out with this or any of my projects
* Create an [Issue](https://github.com/MisledWater79/EndstoneAntiXray/issues) for any bugs.
* Submit a [Pull Request](https://github.com/MisledWater79/EndstoneAntiXray/pulls) for any contributions.
* Join the [Discord]() for miscellaneous stuff & chatting.