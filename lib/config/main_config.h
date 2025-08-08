#pragma once
#ifndef _MAIN_CONFIG_H_
#define _MAIN_CONFIG_H_

#include "Arduino.h"

class Config {
private:
  String ssid = "";
  String pass = "";
public:
  bool update_parametres(const char* _ssid, const char* _pass);
  const char* get_ssid()const { return ssid.c_str(); }
  const char* get_pass()const { return pass.c_str(); }
};

//? Main configuration definitions
extern Config cnfg;
void load_config();
void update_config(Config _cfg);
#endif