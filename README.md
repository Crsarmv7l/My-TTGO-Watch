Fork of Sharandac's firmware.

- BLE weather widget working with Weather Notification (src icons still may need some tweaks, I am fixing them as I see them). Mitigated a previous issue of multiple weather reports stacking up and changing the widget to a previous weather state. Weather message will display, upon close, or screen timeout, it is deleted. Weather widget persists.

- Beacon spam implemented.
  - turn on wifi in settings with the right switch. Go into the subsettings and turn on the beacon spam switch. Beacon spam continues until turned off. This is     a straight integration of the Beacon Spam found here: https://github.com/justcallmekoko/ESP32Marauder/ with minimal enhancements.
    Unlike deauth, beacon spam isn't triggered by a specific packet. Maybe I will dig more into triggering on a probe request, but the packet building will need 
    to speed up substantially. It simply builds and sends beacon packets constantly.

- Deauth implemented
  - Enable the right switch (turns on normal wifi), then turn on left switch. Then select AP target. Deauth continues until the left switch is turned off.
  - Deauth Notes: Deauth gets all information from the selected target AP's beacon packet and is triggered by said packets. (This deauth is broadcast, not
    client specific.
  - Normal wifi usage is not affected (just use right switch alone). I haven't really tested deauth/beacon spam with a saved network, but I wanted to keep   normal usage incase ftp access is needed. 
  - Deauth based on the works of many people, but in particular the ESP32 Marauder Project found here: https://github.com/justcallmekoko/ESP32Marauder/
  - Enhancements and trigger is my own work

This is an experimental branch aimed at incorporating wifi deauth, becaon spam, and BLE weather widget in Sharandac's newly refactored repo 
