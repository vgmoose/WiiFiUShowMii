#include <sstream>
#include <iostream>

#if defined(SWITCH)
#include <switch.h>
#elif defined(__WIIU__)
#include <nsysnet/netconfig.h>
#endif

#include "libs/chesto/src/Container.hpp"
#include "libs/chesto/src/TextElement.hpp"
#include "libs/chesto/src/Button.hpp"

int monoOffset = 10;

class WifiInfo {
public:
    std::string ssid = "N/A";
    std::string auth = "N/A";
    bool unsupported = false;
};

bool readWifiInfo(WifiInfo* wifiInfo) {
#if defined(SWITCH)
    nifmInitialize(NifmServiceType_System);
    NifmNetworkProfileData profileData;
    Result res = nifmGetCurrentNetworkProfile(&profileData);
    nifmExit();

    if (R_SUCCEEDED(res)) {
        auto data = profileData.wireless_setting_data;
        char* ssid = data.ssid;
        ssid[data.ssid_len] = '\0';
        if (data.ssid_len > 0) {
            wifiInfo->ssid = std::string(ssid);
        }
        char* pass = (char*)(data.passphrase) + 1; // TODO: unicode support?
        // TODO: passphrase length may not be part of this field in the future
        int pass_len = int(data.passphrase[0]);
        pass[pass_len] = '\0';
        if (pass_len > 0) {
            wifiInfo->auth = std::string(pass);
        }
        return true;
    }
#elif defined(__WIIU__)
    NetConfWifiConfig wifiConfig;
    netconf_init();
    int success = netconf_get_wifi_cfg(&wifiConfig);
    netconf_close();

    monoOffset = 0;

    if (success >= 0) {
        auto config = wifiConfig.config;
        auto ssid_len = config.ssidlength;
        char* ssid = (char*)config.ssid;
        ssid[ssid_len] = '\0';
        if (ssid_len > 0) {
            wifiInfo->ssid = std::string(ssid);
        }
        auto privacy = config.privacy;
        auto pass_len = privacy.aes_key_len;
        char* pass = (char*)privacy.aes_key;
        pass[pass_len] = '\0';
        if (pass_len > 0) {
            wifiInfo->auth = std::string(pass);
        }
        return true;
    }
#else
    // we aren't switch or wiiu, so we don't have wifi info
    // TODO: add 3ds and Wii support here
    wifiInfo->unsupported = true;
#endif
    return false;
}

int main(int argc, char* argv[])
{
	RootDisplay* display = new RootDisplay();
    display->backgroundColor = fromRGB(0x00, 0x88, 0xA3);
    Container* con = new Container(COL_LAYOUT, 8);

    WifiInfo wifiInfo;
    bool success = readWifiInfo(&wifiInfo);

    // can be used to test on PC
    // success = true;

    if (success)
    {        
        Container* row1 = new Container(ROW_LAYOUT, 6);
        row1->add(new TextElement("SSID:", 30));
        row1->add(new TextElement(wifiInfo.ssid.c_str(), 30, 0, MONOSPACED))->y += monoOffset;

        Container* row2 = new Container(ROW_LAYOUT, 6);
        row2->add(new TextElement("Auth:", 30));
        row2->add(new Button("Show", X_BUTTON))->setAction([row1, row2, wifiInfo]() {
            // replace button with the password
            auto btn = row2->elements.back();
            auto x = btn->x;
            auto y = btn->y + monoOffset;
            row2->elements.pop_back();
            row2->elements.push_back((new TextElement(wifiInfo.auth.c_str(), 30, 0, MONOSPACED))->setPosition(x, y));
        });

        con->add(row1);
        con->add(row2);
    } else
    {
        if (wifiInfo.unsupported) {
            con->add(new TextElement("Unsupported platform!", 36));
            con->add(new TextElement("This build will never return any WiFi info.", 30));
        } else {
            con->add(new TextElement("There was an issue trying to read WiFi info!", 36));
            con->add(new TextElement("This app can only display info about the current network.", 24))->centerHorizontallyIn(con);
            con->add(new TextElement("Please connect to a wireless hotspot in settings and retry.", 24))->centerHorizontallyIn(con);
        }
    }

    con->height += 10;

    con->add(new Button("Close App", B_BUTTON))->centerHorizontallyIn(con)->setAction([display](){
		display->exitRequested = true;
		display->isRunning = false;
	});

    con->centerIn(display);
    display->child(con);

    display->canUseSelectToExit = true;
	return display->mainLoop();
}
