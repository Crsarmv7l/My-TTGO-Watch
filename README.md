A fork of Sharandac's excellent firmware. 

Attempt to get my original branch and modifications working with android and Gadgetbridge.

There are two problems with gadgetbridge that make things more difficult:
1. Gadgetbridge does not cache notifications and send on connect, it assumes BLE is always on. Hence in The Bluetooth setting you MUST have "Stay on" enabled (maybe "Always on" as well).
2. Call notification is broken in Gadgetbridge for the Bangle, which we are emulating. I can see from serial logging that a message is sent. I will try and make it work with that.

Should be supported once I fix it:
- Call notification
- Txt notification
- Email notification
- Chat app notification
- Weather notification, Works fine with the Weather Notification add on for gadgetbridge. Just had to tweak names a bit to resolve the correct icons.
- BLE Weather widget 
- Experimental deauth attack in wifi settings. Enable the right switch (turns on normal wifi), then turn on left switch. Then select AP target. Deauth continues until the left switch is turned off.
     - Deauth Notes: Deauth gets all information from the selected target AP's beacon packet and is triggered by said packets. (This deauth is broadcast, not        client specific, it should be easy to look at the code and add another if statement in the promiscuous filt that kicks in after first_run, filtering by          destination BSSID/MAC, if to the AP, pull the sender address and insert into the Dest address on the deauth packet and send).
     - Normal wifi usage is not affected (just use right switch alone). I haven't really tested deauth/beacon spam with a saved network, but I wanted to keep        normal usage incase ftp access is needed. I recommend deleting the network after ftp usage to ensure there are no issues.
     - Deauth based on the works of many people, but in particular the ESP32 Marauder Project found here: https://github.com/justcallmekoko/ESP32Marauder/
     - Enhancements and trigger is my own work
- AP Beacon Spam. Turn on wifi in settings with the right switch. Go into the subsettings and turn on the beacon spam switch. Beacon spam continues until turned off. This is a straight integration of the Beacon Spam found here: https://github.com/justcallmekoko/ESP32Marauder/ with minimal enhancements. 
     - Unlike deauth, beacon spam isn't triggered by a specific packet. Maybe I will dig more into triggering on a probe request, but the packet building will        need to speed up substantially. It simply builds and sends beacon packets constantly.

Both Step sync and Battery level were removed

# DISCLAIMER: 
This is very much a work in progress and my coding isn't fantastic or elegant. I take no responsiility for any damage caused by you running this firmware. It shouldn't damage anything, but if you are concerned feel free to go through the code. 

