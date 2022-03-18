# Work in Progress:
I am torn weather to publish my latest deauth code. The currently posted deauth code is flawed for two reasons: 

1. It simply takes a user selected target's bssid and throws that in a packet as both the MAC and BSSID in the simplest way possible, then sends the packets. Despite what people say, while the BSSID *can* be the MAC, it isn't always. For instance this works fine on my main router, but doesn't work at all on my secondary router that uses a powerline adaptor. My secondary router is essentially an adhoc network.
2. The second reason it is flawed is that the 1.0.0 WiFi library, and even esp_wifi_scan for some reason I don't know, doesn't return the correct channel number. This is true even from Sharandac's unmodified firmware

My current (unpublished) deauth gets around those with the following:
1. Gets the user selected AP target's BSSID
2. Drops into promiscuous and iterates through channels once, sorting for beacon packets that match the user selected BSSID.
3. Once a matching beacon packet is obtained, pull the primary channel info from the packet as well as the AP's MAC.
4. Set the deauth frame's BSSID to the user selected target AP, and the deauth frame's src to the AP's MAC.
5. Set the promiscuous channel to the primary channel pulled from the AP beacon.
6. Wait for client traffic to the user selected router.
7. Once traffic is found, it pulls the src address from the client packet, and sets that as the destination address in the deauth frame.
8. Sends the packet(s) to the client

This is HIGHLY effective. With multiple devices on my networks I was able to deauth EVERYTHING on both my primary network and on my secondary adhoc network. My secondary network previously took evasive action, changing channels and the BSSID. Essentially this works because the packets are exactly copying the AP, and since I am not using broadcast deauth but instead targeting and deauthing each individual client. By sending the raw packets this is also very fast and doesn't seem to fall behind.

The reason I am torn on publishing is because the only way to protect against my implementation is 802.11w. Nothing else will work. Routers don't know the attack is happening because the packets are always sent to individual clients. Clients don't know the attack is happening because the packet copied the BSSID and AP MAC exactly. The only thing needed to preform the attack is my code, the watch, two button flips and touching the target network. Aircrack which is a lot more complex comparatively in ease of use has issues with rampant abuse and this is so easy once the code is loaded. 

I will think on it some more.


A fork of Sharandac's excellent firmware. 

This port is adapted to work with Adam Piggz's Amazfish companion app on linux phone distros via the Bangle.js device type. Specifically by utilizing UART UUID's, the Bangle.js name, and expected services. On the Amazfish side the Bangle.js name along with service discovery returns pinetimejfdevice (as it should for Bangle.js), and it works through devicefactory.cpp.

Amazfish:
https://github.com/piggz/harbour-amazfish

Further modifications are to optimize the layout with the information provided by Amazfish. See here for further ref (https://github.com/piggz/harbour-amazfish/blob/master/daemon/src/devices/banglejsdevice.cpp)

While the Weather App provded by Sharandac's firmware is nice, it relies on wifi to update. I have completed a weather widget that utilizes Amazfish's built in weather service messages. Visually it looks the same as the Weather app widget (maybe not as customizable). It should also work well with Gadgetbridge. Probably still needs some work, but its functional.

Temperature units can be changed from F to C in src/gui/mainbar/setup_tile/bluetooth_settings/bluetooth_message.cpp
change line 816 "int conversion" to "temperature - 273", and change the unit sign in the strings for Celcius.

Features are limited due to a limited feature set supported in Amazfish and my limited ability.

Currently Supported Features:
- Call notification
- Txt notification
- Email notification
- Chat app notification
- Weather notification
- Weather widget 
     - Modified Amazfish to send on connect twice an hour. If you want my version just ask.
- Experimental deauth attack in wifi settings. It is broadcast deauth so ymmv.
Deauth based on the works of many people, but in particular the ESP32 Marauder Project forund here: https://github.com/justcallmekoko/ESP32Marauder/

Can Work, but doesn't:
- Music Control IS implimented in Amazfish, but I have little interest in fixing it to work. Maybe in the future.
- Step sync via message (dunno why it isn't working really)
- Battery sync same as above. Battery UUID is correct,battery message has the correct triggers but amazfish isn't reading it. Might be formatting?
  - Due to the above, both Step sync and Battery level were removed

# Other:
All other features on the watch side are untouched except the removal of some apps that I dont use or wont work with Amazfish, but they can be added back in directly (eg calc, IRremote, etc. Anything not wanted can be commented out/removed on compile)

DISCLAIMER: This is very much a work in progress and my coding isn't fantastic or elegant. I take no responsiility for any damage caused by you running this firmware. It shouldn't damage anything, but if you are concerned feel free to go through the code. I run this firmware myself on a TTGO T-watch 2020 v1 with Amazfish 2.0.3 on SailfishOS 4.2.

# My-TTGO-Watch

A GUI named hedge for smartwatch like devices based on ESP32. Currently support for T-Watch2020 (V1,V2,V3), T-Watch2021, M5Paper, M5Core2 and native Linux support for testing.

## Features

* BLE communication
* Time synchronization via BLE
* Notification via BLE
* Step counting
* Wake-up on wrist rotation
* Quick actions:

  * WiFi
  * Bluetooth
  * GPS
  * Luminosity
  * Sound volume

* Multiple watch faces:

  * Embedded (digital)
  * [Community based watchfaces](https://sharandac.github.io/My-TTGO-Watchfaces/)

* Multiple 'apps':

  * Music (control the playback of the music on your phone)
  * Navigation (displays navigation instructions coming from the companion app)
  * Map (displays a map)
  * Notification (displays the last notification received)
  * Stopwatch (with all the necessary functions such as play, pause, stop)
  * Alarm
  * Step counter (displays the number of steps and daily objective)
  * Weather
  * Calendar
  * IR remote
  * ...

* Companion apps: Gadgetbridge

## Install

Clone this repository and open it with platformIO. Select the right env and then build and upload.
Or follow the great step by step [tutorial](https://www.youtube.com/watch?v=wUGADCnerCs) from [ShotokuTech](https://github.com/ShotokuTech).
If you are interested in native Linux support, please install sdl2, curl and mosquitto dev lib and change the env to emulator_* in platformIO.

```bash
sudo apt-get install libsdl2-dev libcurl4-gnutls-dev libmosquitto-dev build-essential
```

# Telegram chatgroup

Telegram chatgroup is here:
https://t.me/TTGO_Watch

# known issues

* the webserver crashes the ESP32 really often
* the battery indicator is not accurate, rather a problem with the power management unit ( axp202 )

# how to use

Cf. [Usage](USAGE.md)


# Contributors

Special thanks to the following people for their help:

[5tormChild](https://github.com/5tormChild)<br>
[bwagstaff](https://github.com/bwagstaff)<br>
[chrismcna](https://github.com/chrismcna)<br>
[datacute](https://github.com/datacute)<br>
[fliuzzi02](https://github.com/fliuzzi02)<br>
[guyou](https://github.com/guyou)<br>
[jakub-vesely](https://github.com/jakub-vesely)<br>
[joshvito](https://github.com/joshvito)<br>
[JoanMCD](https://github.com/JoanMCD)<br>
[NorthernDIY](https://github.com/NorthernDIY)<br>
[Neuroplant](https://github.com/Neuroplant)<br>
[rnisthal](https://github.com/rnisthal)<br>
[paulstueber](https://github.com/paulstueber)<br>
[ssspeq](https://github.com/ssspeq)<br>

and the following projects:

[ArduinoJson](https://github.com/bblanchon/ArduinoJson)<br>
[AsyncTCP](https://github.com/me-no-dev/AsyncTCP)<br>
[ESP32SSDP](https://github.com/luc-github/ESP32SSDP)<br>
[ESP32-targz](https://github.com/tobozo/ESP32-targz)<br>
[ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)<br>
[ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)<br>
[LVGL](https://github.com/lvgl)<br>
[pubsubclient](https://github.com/knolleary/pubsubclient)<br>
[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)<br>
[TTGO_TWatch_Library](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library)<br>

Every Contribution to this repository is highly welcome! Don't fear to create pull requests which enhance or fix the project, you are going to help everybody.

