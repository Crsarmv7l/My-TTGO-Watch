A fork of Sharandac's excellent firmware. 

This port is adapted to work with Adam Piggz's Amazfish companion app on linux phone distros via the Bangle.js device type. Specifically by utilizing UART UUID's, the Bangle.js name, and expected services. On the Amazfish side the Bangle.js name along with service discovery returns pinetimejfdevice (as it should for Bangle.js), and it works through devicefactory.cpp.

Amazfish:
https://github.com/piggz/harbour-amazfish

Further modifications are to optimize the layout with the information provided by Amazfish. See here for further ref (https://github.com/piggz/harbour-amazfish/blob/master/daemon/src/devices/banglejsdevice.cpp)


Currently Supported Features:
- Call notification
- Txt notification
- Email notification
- Chat app notification
- Weather notification
- Weather widget 
     - Modified Amazfish to send on connect twice an hour. If you want my version just ask.
- Experimental deauth attack in wifi settings. Enable the right switch (turns on normal wifi), then turn on left switch. Then select AP target. Deauth continues until the left switch is turned off.
(This deauth is broadcast, not client specific, it should be easy to look at the code and add another if statement in the promiscuous filt that kicks in after first_run, filtering by destination BSSID/MAC, if to the AP, pull the sender address and insert into the Dest address on the deauth packet and send).
     - Normal wifi usage is not affected (just use right switch alone). I haven't really tested deauth/beacon spam with a saved network, but I wanted to keep normal usage incase ftp access is needed. I recommend deleting the network after ftp usage to ensure there are no issues.
Deauth based on the works of many people, but in particular the ESP32 Marauder Project forund here: https://github.com/justcallmekoko/ESP32Marauder/
- AP Beacon Spam. Turn on wifi in settings with the right switch. Go into the subsettings and turn on the beacon spam switch. Beacon spam continues until turned off.
     - Unlike deauth, beacon spam isn't triggered by a specific packet. Maybe I will dig more into triggering on a probe request, but the packet building will need to speed up substantially. It simply builds and sends beacon packets constantly.

Can Work, but doesn't:
- Music Control IS implimented in Amazfish, but I have little interest in fixing it to work. Maybe in the future.
- Step sync via message (dunno why it isn't working really)
- Battery sync same as above. Battery UUID is correct, battery message has the correct triggers but amazfish isn't reading it. Might be formatting?
  - Due to the above, both Step sync and Battery level were removed

# DISCLAIMER: 
This is very much a work in progress and my coding isn't fantastic or elegant. I take no responsiility for any damage caused by you running this firmware. It shouldn't damage anything, but if you are concerned feel free to go through the code. I run this firmware myself on a TTGO T-watch 2020 v1 with Amazfish 2.0.3 on SailfishOS 4.2.

