// 问爻 + 黄历 · M5StickS3 合并固件
// 主菜单负责选择功能；两个原固件逻辑保持独立，降低合并风险。
#include <M5Unified.h>
#include <Preferences.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <WiFi.h>
#include <WebServer.h>
#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include "../modules/common/license_public.h"
#include "../modules/common/utf8.h"
#include "../modules/common/text_layout.h"
#include "../modules/common/text_fonts.h"
#include "../modules/common/input.h"
#include "../modules/common/power_monitor.h"
Navigation::Reader navigationReader;
Navigation::Event navigationEvent=Navigation::NONE;

static inline bool useStickCPlusKeySwap(){
#if defined(ARDUINO_M5STACK_STICKC_PLUS)
  return true;
#else
  return false;
#endif
}

static inline m5::Button_Class& btnA(){
  return useStickCPlusKeySwap() ? M5.BtnB : M5.BtnA;
}

static inline m5::Button_Class& btnB(){
  return useStickCPlusKeySwap() ? M5.BtnA : M5.BtnB;
}

static inline bool powerButtonPressed(){
  return M5.BtnPWR.isPressed()
      || M5.BtnPWR.wasClicked()
      || M5.BtnPWR.wasSingleClicked()
      || M5.BtnPWR.wasDoubleClicked()
      || M5.BtnPWR.wasHold();
}

namespace LayoutLab {
struct Settings {
  int wx = 0;
  int wy = 0;
  int ww = 0;
  int wh = 0;
  int wbtop = 0;
  int wbbot = 0;
  int wflow = 0;
  int hx = 0;
  int hy = 0;
  int hw = 0;
};

Settings cfg;
}

namespace HuangLi {
#define setup huangli_setup
#define loop huangli_loop
#include "../modules/huangli/huangli.ino"
#undef setup
#undef loop
}

namespace WenYao {
#define setup wenyao_setup
#define loop wenyao_loop
#include "../modules/wenyao/wenyao.ino"
#undef setup
#undef loop
}

static constexpr int W = 135;
static constexpr int H = 240;
static constexpr const char* COMBO_VERSION = "3.2.9";
static constexpr const char* COMBO_VER = "版｜叁.贰.玖";

static constexpr uint16_t C_PAPER = lgfx::color565(231,220,194);
static constexpr uint16_t C_PAPER2 = lgfx::color565(239,231,210);
static constexpr uint16_t C_INK = lgfx::color565(38,33,26);
static constexpr uint16_t C_INKL = lgfx::color565(156,144,120);
static constexpr uint16_t C_CIN = lgfx::color565(178,58,46);
static constexpr uint16_t C_LINE = lgfx::color565(189,151,88);
static constexpr uint16_t C_BAT_GREEN = lgfx::color565(112,154,104);
static constexpr uint16_t C_BAT_YELLOW = lgfx::color565(183,137,44);
static constexpr uint16_t C_BAT_RED = lgfx::color565(178,58,46);

M5Canvas menuCv(&M5.Display);
Preferences layoutPrefs;
Preferences securityPrefs;

enum MainApp { APP_MENU, APP_MENU_HELP, APP_WENYAO, APP_HUANGLI };
MainApp activeApp = APP_MENU;
int menuSel = 0; // 0 = 问爻, 1 = 黄历
char serialLine[256];
int serialLineLen = 0;
static constexpr uint32_t INPUT_RELEASE_GUARD_MS = 180;
static constexpr uint32_t MAIN_MENU_LONG_PRESS_MS = 800;
static constexpr uint32_t WENYAO_HOME_LONG_PRESS_MS = 800;
static constexpr uint32_t CHARGE_STANDBY_MESSAGE_MS = 6000;
static constexpr uint32_t CHARGE_BOOT_BYPASS_MAGIC = 0xC1A9E001;
static constexpr uint32_t CHARGE_BOOT_BYPASS_CLEAR_MS = 2500;
RTC_DATA_ATTR uint32_t chargeBootBypass = 0;
uint32_t inputLockUntil = 0;
bool suppressNextBRelease = false;
bool menuBLongHandled = false;
bool wenYaoHomeBLongHandled = false;
bool waitButtonsReleased = false;
int menuHelpPage = 0;
static constexpr int MENU_HELP_PAGE_COUNT = 11;
bool productionAuthorized = false;
bool chargeStandby = false;
bool chargeStandbyScreenOn = false;
uint32_t chargeStandbyHideAt = 0;
uint32_t chargeBootBypassClearAt = 0;
Battery::Monitor batteryMonitor;

bool samplePower(){
  if(!batteryMonitor.due(millis())) return false;
  batteryMonitor.update(millis(),Battery::read(M5.Power.M5pm1));
  return true;
}

static constexpr bool ENABLE_LAYOUT_SERIAL = false;
static constexpr bool PROD_LOCK_ENABLED = false; // Community builds do not require activation.

bool inputLocked(){
  return (int32_t)(millis() - inputLockUntil) < 0;
}

void armInputLock(){
  inputLockUntil = millis() + INPUT_RELEASE_GUARD_MS;
}

bool takeAppARelease(){
  if(inputLocked()) return false;
  if(btnA().wasReleased()){
    armInputLock();
    return true;
  }
  return false;
}

bool takeAppBRelease(){
  bool released = btnB().wasReleased();
  if(released && suppressNextBRelease){
    suppressNextBRelease = false;
    armInputLock();
    return false;
  }
  if(inputLocked()) return false;
  if(released){
    armInputLock();
    return true;
  }
  return false;
}

void menuSetF(int n){
  switch(n){
    case 10: menuCv.setFont(&::TextFonts::cn10); break;
    case 12: menuCv.setFont(&::TextFonts::cn12); break;
    case 14: menuCv.setFont(&::TextFonts::cn14); break;
    case 16: menuCv.setFont(&::TextFonts::cn16); break;
    default: menuCv.setFont(&::TextFonts::cn24); break;
  }
}

void menuTc(const char* s,int x,int y,int n,uint16_t col){
  menuSetF(n);
  menuCv.setTextDatum(middle_center);
  menuCv.setTextColor(col);
  menuCv.drawString(s,x,y);
}

void menuTl(const char* s,int x,int y,int n,uint16_t col){
  menuSetF(n);
  menuCv.setTextDatum(middle_left);
  menuCv.setTextColor(col);
  menuCv.drawString(s,x,y);
}

void deviceMacHex(char* out,size_t len){
  uint64_t mac = ESP.getEfuseMac();
  snprintf(out,len,"%04X%08X",(uint16_t)(mac >> 32),(uint32_t)mac);
}

void deviceShortId(char* out,size_t len){
  char mac[13];
  deviceMacHex(mac,sizeof(mac));
  snprintf(out,len,"S3-%.6s",mac);
}

bool verifyProductionLicense(const char* license){
  if(!license || strncmp(license,"V2-",3)!=0) return false;
  const char* hex=license+3;
  size_t n=strlen(hex);
  if(n<16 || n>144 || (n&1)) return false;
  unsigned char signature[72], digest[32];
  for(size_t i=0;i<n;i+=2){
    auto nibble=[](char c)->int{
      if(c>='0' && c<='9') return c-'0';
      if(c>='A' && c<='F') return c-'A'+10;
      if(c>='a' && c<='f') return c-'a'+10;
      return -1;
    };
    int hi=nibble(hex[i]),lo=nibble(hex[i+1]);
    if(hi<0 || lo<0) return false;
    signature[i/2]=(hi<<4)|lo;
  }
  char mac[13],payload[64];
  deviceMacHex(mac,sizeof(mac));
  snprintf(payload,sizeof(payload),"wenyao-huangli-s3|v2|%s",mac);
  if(mbedtls_sha256((const unsigned char*)payload,strlen(payload),digest,0)!=0) return false;
  mbedtls_pk_context key;
  mbedtls_pk_init(&key);
  int err=mbedtls_pk_parse_public_key(&key,(const unsigned char*)LICENSE_PUBLIC_KEY,sizeof(LICENSE_PUBLIC_KEY));
  if(!err) err=mbedtls_pk_verify(&key,MBEDTLS_MD_SHA256,digest,sizeof(digest),signature,n/2);
  mbedtls_pk_free(&key);
  return err==0;
}

bool loadProductionAuthorization(){
  if(!PROD_LOCK_ENABLED) return true;
  if(!securityPrefs.begin("prodsec",true)) return false;
  String stored=securityPrefs.getString("lic","");
  securityPrefs.end();
  return verifyProductionLicense(stored.c_str());
}

bool saveProductionAuthorization(const char* license){
  if(!verifyProductionLicense(license)) return false;
  if(!securityPrefs.begin("prodsec",false)) return false;
  size_t written=securityPrefs.putString("lic",license);
  securityPrefs.end();
  return written==strlen(license) && loadProductionAuthorization();
}

void drawProductionLocked(){
  activeApp = APP_MENU;
  char sid[12];
  deviceShortId(sid,sizeof(sid));
  menuPaperBg();
  menuFrame();
  menuTc("未授权",W/2,62,20,C_CIN);
  menuCv.drawFastHLine(28,88,W-56,C_INKL);
  menuTc("设备未授权",W/2,112,14,C_INK);
  menuTc(sid,W/2,138,14,C_INKL);
  menuTc("请联系卖家授权",W/2,170,12,C_INK);
  menuTc(COMBO_VER,W/2,214,12,C_INKL);
  menuCv.pushSprite(0,0);
}

void menuPaperBg(){
  menuCv.fillScreen(C_PAPER);
  randomSeed(9);
  for(int i=0;i<240;i++) menuCv.drawPixel(random(W),random(H), lgfx::color565(214,202,174));
  for(int i=0;i<36;i++) menuCv.drawPixel(random(W),random(H), lgfx::color565(198,184,154));
}

void menuFrame(){
  menuCv.drawRect(5,5,W-10,H-10,C_LINE);
  menuCv.drawRect(8,8,W-16,H-16,C_INKL);
}

void drawMenuSeal(int cx,int cy,const char* ch,bool selected){
  int sz = 40;
  menuCv.fillRect(cx-sz/2,cy-sz/2,sz,sz,C_CIN);
  menuCv.drawRect(cx-sz/2+3,cy-sz/2+3,sz-6,sz-6,C_PAPER);
  menuTc(ch,cx,cy,24,C_PAPER);
}

void drawMenuChoice(int top,int bottom,const char* sealCh,const char* title,bool selected){
  int x=11, y=top+5, w=W-22, h=bottom-top-10;
  if(selected){
    menuCv.fillRect(x,y,w,h,C_PAPER2);
    menuCv.drawRect(x,y,w,h,C_CIN);
    menuCv.drawRect(x+3,y+3,w-6,h-6,C_CIN);
  } else {
    menuCv.drawRect(x,y,w,h,C_INKL);
  }
  int cy = top + (bottom-top) / 2;
  drawMenuSeal(42,cy,sealCh,selected);
  menuTl(title,66,cy,24,C_INK);
}

int menuBatteryPercent(){
  return batteryMonitor.percent;
}

uint16_t menuBatteryColor(int pct){
  if(pct < 0) return C_INKL;
  if(pct >= 50) return C_BAT_GREEN;
  if(pct >= 20) return C_BAT_YELLOW;
  return C_BAT_RED;
}

void drawMenuBattery(int x,int y){
  int pct = menuBatteryPercent();
  uint16_t col = menuBatteryColor(pct);
  const int bw = 26;
  const int bh = 10;
  const int capW = 2;
  int fillW = pct < 0 ? 0 : (bw-4)*pct/100;

  menuCv.fillRect(x-1,y-1,bw+capW+2,bh+2,C_PAPER);
  menuCv.drawRoundRect(x,y,bw,bh,2,C_INKL);
  if(fillW > 0) menuCv.fillRect(x+2,y+2,fillW,bh-4,col);
  if(pct < 0) menuCv.drawFastHLine(x+10,y+5,6,C_INKL);
  menuCv.fillRect(x+bw,y+4,capW,bh-8,col);
  menuCv.drawRect(x+bw,y+4,capW,bh-8,C_INKL);
}

void drawChargeBattery(int pct){
  uint16_t col = menuBatteryColor(pct);
  const int bw = 58;
  const int bh = 20;
  const int x = (W - bw) / 2 - 2;
  const int y = 126;
  const int capW = 5;
  int fillW = pct < 0 ? 0 : (bw-6)*pct/100;
  menuCv.drawRoundRect(x,y,bw,bh,3,C_INKL);
  if(fillW > 0) menuCv.fillRect(x+3,y+3,fillW,bh-6,col);
  if(pct < 0) menuCv.drawFastHLine(x+23,y+10,12,C_INKL);
  menuCv.fillRect(x+bw,y+7,capW,bh-14,col);
  menuCv.drawRect(x+bw,y+7,capW,bh-14,C_INKL);
}

void formatChargePercent(char* out,size_t len,int pct){
  if(pct < 0){
    snprintf(out,len,"读数异常");
    return;
  }
  int rounded = constrain(((pct + 2) / 5) * 5,0,100);
  snprintf(out,len,"约 %d%%",rounded);
}

void drawMainMenu(){
  activeApp = APP_MENU;
  menuBLongHandled = false;
  menuPaperBg();
  menuFrame();
  drawMenuChoice(12,84,"爻","问爻",menuSel==0);
  drawMenuChoice(84,156,"历","黄历",menuSel==1);
  if(menuSel==2) menuCv.drawRect(18,158,99,25,C_CIN);
  menuTc("使用说明",W/2,170,14,menuSel==2 ? C_CIN : C_INK);
  menuTc("B切换 A选定",W/2,194,12,C_INK);
  drawMenuBattery(13,210);
  menuTc(COMBO_VER,87,214,12,C_INKL);
  menuCv.pushSprite(0,0);
}

bool shouldEnterChargeStandby(){
  M5.update();
  int vbus = batteryMonitor.raw.inputMv;
  if(vbus <= 4400){
    chargeBootBypass = 0;
    return false;
  }
  if(chargeBootBypass == CHARGE_BOOT_BYPASS_MAGIC){
    chargeBootBypass = 0;
    return false;
  }
  return vbus > 4400 && !btnA().isPressed() && !btnB().isPressed() && !powerButtonPressed();
}

void drawChargeStandbyMessage(){
  menuCv.fillScreen(TFT_BLACK);
  int pct = menuBatteryPercent();
  char pctText[24];
  formatChargePercent(pctText,sizeof(pctText),pct);
  menuTc(Battery::label(batteryMonitor.state),W/2,78,20,C_PAPER2);
  menuCv.drawFastHLine(36,104,W-72,C_INKL);
  drawChargeBattery(pct);
  menuTc(pctText,W/2,164,16,C_PAPER2);
  menuTc("长按A使用",W/2,194,12,C_INKL);
  menuCv.pushSprite(0,0);
}

void showChargeStandbyMessage(){
  chargeStandbyScreenOn = true;
  chargeStandbyHideAt = millis() + CHARGE_STANDBY_MESSAGE_MS;
  M5.Display.setBrightness(180);
  drawChargeStandbyMessage();
}

void hideChargeStandbyMessage(){
  chargeStandbyScreenOn = false;
  menuCv.fillScreen(TFT_BLACK);
  menuCv.pushSprite(0,0);
  M5.Display.setBrightness(0);
}

void enterChargeStandby(){
  chargeStandby = true;
  activeApp = APP_MENU;
  showChargeStandbyMessage();
}

void exitChargeStandby(){
  chargeStandby = false;
  chargeStandbyScreenOn = false;
  chargeBootBypass = CHARGE_BOOT_BYPASS_MAGIC;
  chargeBootBypassClearAt = millis() + CHARGE_BOOT_BYPASS_CLEAR_MS;
  M5.Display.setBrightness(180);
  waitButtonsReleased = true;
  navigationReader.suppressUntilReleased();
  armInputLock();
  if(productionAuthorized) drawMainMenu();
  else drawProductionLocked();
}

void manualRow(int y,const char* key,const char* val){
  const int keyX = 18;
  const int valBaseX = 50;
  const int gap = 10;
  int valX = valBaseX;
  if(key && key[0]){
    menuTl(key,keyX,y,12,C_CIN);
    menuSetF(12);
    valX = max(valBaseX, keyX + (int)menuCv.textWidth(key) + gap);
  }
  menuTl(val,valX,y,12,C_INK);
}

void manualFooter(){
  int page = constrain(menuHelpPage,0,MENU_HELP_PAGE_COUNT-1);
  char label[16];
  snprintf(label,sizeof(label),"%d/%d",page+1,MENU_HELP_PAGE_COUNT);
  menuCv.drawFastHLine(32,204,W-64,C_INKL);
  menuTc(label,W/2,222,16,C_INK);
}

void drawMainMenuHelp(){
  struct HelpPage { const char* title; const char* lines[7]; };
  static const HelpPage pages[] = {
    {"先认两个键",{
      "中间是A键，","右侧是B键。","在菜单里，","用B换一个，",
      "按A就选好了。","长按B退一步。",nullptr}},
    {"慢慢看就好",{
      "读内容时，","按A往后翻，","按B往前翻。","想回主菜单，",
      "按住B一秒，","不用等到松手。",nullptr}},
    {"用完歇一会",{
      "左侧按键，","按一下开机，","连按两下关机。","用完记得关机。",
      "再开机时，","记得核对日期。",nullptr}},
    {"充电不用等",{
      "充电时会灭屏，","按A或B看看，","会亮起充电页。","想边充边用，",
      "长按A就好。","不用拔掉电线。",nullptr}},
    {"看看充上没",{
      "显示正在充电，","才是充电信号。","百分比是估算。","显示未在充电，",
      "不一定是满了。","若电量一直低，","请联系我检查。"}},
    {"给自己点启发",{
      "先想好一件事，","在问爻选摇卦。","按A摇一次，","一共摇六次。",
      "读读卦里的话，","换个角度想想。",nullptr}},
    {"想简单一点",{
      "在问爻首页，","长按B换模式。","简单只看卦象，","也能看卦义。",
      "专业能看动爻。","选择会记住，","短按B仍返回。"}},
    {"还可以回头看",{
      "看完最后一页，","再按A，还能","重看这次结果。","上一卦只留在",
      "本次开机期间，","关机后就没了。",nullptr}},
    {"也能自己来",{
      "想看不同组合，","就到选卦看看。","先上卦后下卦。","有自己的结果，",
      "选自定六爻，","从初爻选起。",nullptr}},
    {"翻翻别的日子",{
      "想换一天看看，","在黄历长按A，","打开日期功能。","翻看不改今天。",
      "选设为今日后，","再确认才会改。",nullptr}},
    {"把日子对准",{
      "到日期功能里，","选手机校时。","手机连上","HuangLi，再用",
      "浏览器打开","192.168.8.8","按提示同步。"}},
  };
  static_assert(sizeof(pages)/sizeof(pages[0])==MENU_HELP_PAGE_COUNT,"Help page count mismatch");
  activeApp=APP_MENU_HELP;
  menuHelpPage=constrain(menuHelpPage,0,MENU_HELP_PAGE_COUNT-1);
  menuPaperBg(); menuFrame();
  menuTc("说明书",W/2,26,16,C_INK);
  menuCv.drawFastHLine(23,43,W-46,C_INKL);
  const HelpPage& page=pages[menuHelpPage];
  menuTc(page.title,W/2,61,14,C_CIN);
  for(int i=0;i<7;i++){
    if(page.lines[i]) menuTl(page.lines[i],18,82+i*18,14,C_INK);
  }
  manualFooter();
  menuCv.pushSprite(0,0);
}

int clampLayoutValue(const char* key,int value){
  if(strcmp(key,"ww")==0 || strcmp(key,"hw")==0) return constrain(value,-24,14);
  if(strcmp(key,"wbtop")==0 || strcmp(key,"wbbot")==0) return constrain(value,-32,32);
  if(strcmp(key,"wh")==0 || strcmp(key,"wflow")==0) return constrain(value,-36,36);
  return constrain(value,-30,30);
}

bool setLayoutKey(const char* key,int value){
  value = clampLayoutValue(key,value);
  if(strcmp(key,"wx")==0) LayoutLab::cfg.wx=value;
  else if(strcmp(key,"wy")==0) LayoutLab::cfg.wy=value;
  else if(strcmp(key,"ww")==0) LayoutLab::cfg.ww=value;
  else if(strcmp(key,"wh")==0) LayoutLab::cfg.wh=value;
  else if(strcmp(key,"wbtop")==0) LayoutLab::cfg.wbtop=value;
  else if(strcmp(key,"wbbot")==0) LayoutLab::cfg.wbbot=value;
  else if(strcmp(key,"wflow")==0) LayoutLab::cfg.wflow=value;
  else if(strcmp(key,"hx")==0) LayoutLab::cfg.hx=value;
  else if(strcmp(key,"hy")==0) LayoutLab::cfg.hy=value;
  else if(strcmp(key,"hw")==0) LayoutLab::cfg.hw=value;
  else return false;
  return true;
}

void saveLayout(){
  layoutPrefs.begin("layoutlab",false);
  layoutPrefs.putInt("wx",LayoutLab::cfg.wx);
  layoutPrefs.putInt("wy",LayoutLab::cfg.wy);
  layoutPrefs.putInt("ww",LayoutLab::cfg.ww);
  layoutPrefs.putInt("wh",LayoutLab::cfg.wh);
  layoutPrefs.putInt("wbtop",LayoutLab::cfg.wbtop);
  layoutPrefs.putInt("wbbot",LayoutLab::cfg.wbbot);
  layoutPrefs.putInt("wflow",LayoutLab::cfg.wflow);
  layoutPrefs.putInt("hx",LayoutLab::cfg.hx);
  layoutPrefs.putInt("hy",LayoutLab::cfg.hy);
  layoutPrefs.putInt("hw",LayoutLab::cfg.hw);
  layoutPrefs.end();
}

void loadLayout(){
  layoutPrefs.begin("layoutlab",true);
  LayoutLab::cfg.wx=layoutPrefs.getInt("wx",0);
  LayoutLab::cfg.wy=layoutPrefs.getInt("wy",0);
  LayoutLab::cfg.ww=layoutPrefs.getInt("ww",0);
  LayoutLab::cfg.wh=layoutPrefs.getInt("wh",0);
  LayoutLab::cfg.wbtop=layoutPrefs.getInt("wbtop",0);
  LayoutLab::cfg.wbbot=layoutPrefs.getInt("wbbot",0);
  LayoutLab::cfg.wflow=layoutPrefs.getInt("wflow",0);
  LayoutLab::cfg.hx=layoutPrefs.getInt("hx",0);
  LayoutLab::cfg.hy=layoutPrefs.getInt("hy",0);
  LayoutLab::cfg.hw=layoutPrefs.getInt("hw",0);
  layoutPrefs.end();
}

void resetLayout(){
  LayoutLab::cfg = LayoutLab::Settings();
  saveLayout();
}

void printLayout(){
  Serial.printf(
    "{\"type\":\"layout\",\"version\":\"%s\",\"wx\":%d,\"wy\":%d,\"ww\":%d,\"wh\":%d,\"wbtop\":%d,\"wbbot\":%d,\"wflow\":%d,\"hx\":%d,\"hy\":%d,\"hw\":%d}\n",
    COMBO_VERSION,
    LayoutLab::cfg.wx,LayoutLab::cfg.wy,LayoutLab::cfg.ww,LayoutLab::cfg.wh,
    LayoutLab::cfg.wbtop,LayoutLab::cfg.wbbot,LayoutLab::cfg.wflow,
    LayoutLab::cfg.hx,LayoutLab::cfg.hy,LayoutLab::cfg.hw
  );
}

void trimRight(char* s){
  int n = strlen(s);
  while(n>0 && (s[n-1]==' ' || s[n-1]=='\t')){
    s[--n] = 0;
  }
}

bool handleProductionCommand(char* line){
  if(strcmp(line,"PROD ID")==0 || strcmp(line,"PROD STATUS")==0){
    productionAuthorized=loadProductionAuthorization();
    char mac[13];
    char sid[12];
    deviceMacHex(mac,sizeof(mac));
    deviceShortId(sid,sizeof(sid));
    Serial.printf("PROD %s %s %s %s\n",sid,mac,productionAuthorized ? "AUTHORIZED" : "LOCKED",COMBO_VERSION);
    return true;
  }
  if(strncmp(line,"PROD AUTH ",10)==0){
    char* token = line + 10;
    while(*token==' ' || *token=='\t') token++;
    trimRight(token);
    if(saveProductionAuthorization(token)){
      productionAuthorized = true;
      Serial.println("OK PROD AUTH");
      drawMainMenu();
    } else {
      Serial.println("ERR PROD AUTH");
      productionAuthorized=loadProductionAuthorization();
      if(!productionAuthorized) drawProductionLocked();
    }
    return true;
  }
  if(strcmp(line,"TIME STATUS")==0){
    char mac[13];
    deviceMacHex(mac,sizeof(mac));
    Serial.printf("TIME %s %lld %s %d\n",mac,(long long)time(nullptr),
                  HuangLi::clockTrusted ? "TRUSTED" : "UNTRUSTED",M5.Rtc.isEnabled()?1:0);
    return true;
  }
  if(strncmp(line,"TIME SET ",9)==0){
    char* end=nullptr;
    long long epoch=strtoll(line+9,&end,10);
    if(productionAuthorized && end && *end==0 && epoch>=1704067200LL && epoch<=4102444799LL){
      HuangLi::applyPhoneEpoch((time_t)epoch);
      Serial.println("OK TIME SET");
      if(activeApp==APP_HUANGLI) redrawForLayout();
    } else Serial.println("ERR TIME SET");
    return true;
  }
  return strncmp(line,"PROD",4)==0 || strncmp(line,"TIME",4)==0;
}

void drawHuangLiHome(){
  HuangLi::screen = HuangLi::HOME;
  HuangLi::paperBg();
  HuangLi::frame(false);
  char clockLine[24];
  HuangLi::formatHomeClock(clockLine,sizeof(clockLine));
  HuangLi::tcFit(clockLine,W/2,30,16,HuangLi::C_INK,W-10);
  HuangLi::seal(51,44);
  HuangLi::tc("黄历",W/2,96,24,HuangLi::C_INK);
  HuangLi::tc("通",43,120,16,HuangLi::C_CIN);
  HuangLi::tc("书",59,120,16,HuangLi::C_CIN);
  HuangLi::tc("择",75,120,16,HuangLi::C_CIN);
  HuangLi::tc("日",91,120,16,HuangLi::C_CIN);
  HuangLi::cv.drawFastHLine(26,139,W-52,HuangLi::C_INKL);
  HuangLi::tc("短A：今日",W/2,162,14,HuangLi::C_INK);
  HuangLi::tc("长A：日期功能",W/2,184,14,HuangLi::C_INK);
  HuangLi::tc(HuangLi::VER,W/2,222,14,HuangLi::C_INKL);
  HuangLi::cv.pushSprite(0,0);
}

void drawWenYaoHome(){
  wenYaoHomeBLongHandled = false;
  WenYao::state = WenYao::HOME;
  WenYao::drawHome();
}

void redrawForLayout(){
  if(activeApp == APP_MENU){
    drawMainMenu();
  } else if(activeApp == APP_MENU_HELP){
    drawMainMenuHelp();
  } else if(activeApp == APP_HUANGLI){
    if(HuangLi::screen == HuangLi::HOME) drawHuangLiHome();
    else HuangLi::redrawCurrentScreen();
  } else {
    if(WenYao::state == WenYao::HOME) drawWenYaoHome();
    else WenYao::redrawCurrentScreen();
  }
}

bool showLayoutPage(const char* page){
  if(strcmp(page,"menu")==0){
    drawMainMenu();
    return true;
  }
  if(strcmp(page,"menu-help")==0){
    drawMainMenuHelp();
    return true;
  }
  if(strcmp(page,"wen-home")==0){
    activeApp = APP_WENYAO;
    drawWenYaoHome();
    return true;
  }
  if(strncmp(page,"wen-",4)==0){
    activeApp = APP_WENYAO;
    WenYao::loadLayoutDemo();
    if(strcmp(page,"wen-cast")==0){
      WenYao::state = WenYao::CAST;
      WenYao::drawCast();
    } else if(strcmp(page,"wen-summary")==0){
      WenYao::state = WenYao::SUMMARY;
      WenYao::drawSummary();
    } else if(strcmp(page,"wen-base")==0){
      WenYao::state = WenYao::DETAIL;
      WenYao::detailPage = 0;
      WenYao::drawDetail();
    } else if(strcmp(page,"wen-flow")==0){
      WenYao::state = WenYao::DETAIL;
      WenYao::detailPage = 1;
      WenYao::drawDetail();
    } else if(strcmp(page,"wen-yaoci")==0){
      WenYao::state = WenYao::DETAIL;
      WenYao::detailPage = 2;
      WenYao::drawDetail();
    } else if(strcmp(page,"wen-xiang")==0){
      WenYao::state = WenYao::DETAIL;
      WenYao::detailPage = 3;
      WenYao::drawDetail();
    } else if(strcmp(page,"wen-jie")==0){
      WenYao::state = WenYao::DETAIL;
      WenYao::detailPage = 4;
      WenYao::drawDetail();
    } else if(strcmp(page,"wen-yi")==0){
      WenYao::state = WenYao::DETAIL;
      WenYao::detailPage = 5;
      WenYao::drawDetail();
    } else if(strcmp(page,"wen-zhi")==0){
      WenYao::state = WenYao::DETAIL;
      WenYao::detailPage = WenYao::detailPageCount()-2;
      WenYao::drawDetail();
    } else if(strcmp(page,"wen-zhi-body")==0){
      WenYao::state = WenYao::DETAIL;
      WenYao::detailPage = WenYao::detailPageCount()-1;
      WenYao::drawDetail();
    } else {
      return false;
    }
    return true;
  }
  if(strcmp(page,"huang-home")==0){
    activeApp = APP_HUANGLI;
    drawHuangLiHome();
    return true;
  }
  if(strcmp(page,"huang-content")==0){
    activeApp = APP_HUANGLI;
    HuangLi::viewOffset = 0;
    HuangLi::screen = HuangLi::CONTENT;
    HuangLi::drawContent();
    return true;
  }
  if(strcmp(page,"huang-explain")==0){
    activeApp = APP_HUANGLI;
    HuangLi::viewOffset = 0;
    HuangLi::screen = HuangLi::EXPLAIN;
    HuangLi::drawExplain();
    return true;
  }
  if(strcmp(page,"huang-help")==0){
    activeApp = APP_HUANGLI;
    HuangLi::screen = HuangLi::KEY_HELP;
    HuangLi::drawKeyHelp();
    return true;
  }
  return false;
}

void processSerialLine(char* line){
  while(*line==' ') line++;
  if(strncmp(line,"LL ",3)==0) line += 3;
  if(handleProductionCommand(line)) return;
  if(strcmp(line,"POWER STATUS")==0){
    samplePower();
    const auto& r = batteryMonitor.raw;
    Serial.printf("POWER %s ms=%lu state=%s vbat_mv=%d vin_mv=%d chg_en=%d gpio=%d estimate_pct=%d standby=%d screen=%d\n",
      COMBO_VERSION,(unsigned long)batteryMonitor.sampledAt,Battery::code(batteryMonitor.state),
      r.batteryMv,r.inputMv,r.enabled,r.gpioBits,batteryMonitor.percent,
      chargeStandby,chargeStandbyScreenOn);
    return;
  }
  if(!ENABLE_LAYOUT_SERIAL){
    Serial.println("ERR PROD MODE");
    return;
  }
  if(strcmp(line,"PING")==0){
    Serial.printf("OK PONG %s\n",COMBO_VERSION);
  } else if(strcmp(line,"GET")==0){
    printLayout();
  } else if(strcmp(line,"SAVE")==0){
    saveLayout();
    Serial.println("OK SAVE");
  } else if(strcmp(line,"RESET")==0){
    resetLayout();
    redrawForLayout();
    Serial.println("OK RESET");
    printLayout();
  } else if(strncmp(line,"SET ",4)==0){
    char key[16];
    int value=0;
    if(sscanf(line+4,"%15s %d",key,&value)==2 && setLayoutKey(key,value)){
      redrawForLayout();
      Serial.printf("OK SET %s %d\n",key,clampLayoutValue(key,value));
    } else {
      Serial.println("ERR SET");
    }
  } else if(strncmp(line,"PAGE ",5)==0){
    char page[32];
    if(sscanf(line+5,"%31s",page)==1 && showLayoutPage(page)){
      Serial.printf("OK PAGE %s\n",page);
    } else {
      Serial.println("ERR PAGE");
    }
  } else {
    Serial.println("ERR CMD");
  }
}

void handleSerialLayout(){
  while(Serial.available()){
    char c = (char)Serial.read();
    if(c=='\n' || c=='\r'){
      if(serialLineLen>0){
        serialLine[serialLineLen]=0;
        processSerialLine(serialLine);
        serialLineLen=0;
      }
    } else if(serialLineLen < (int)sizeof(serialLine)-1){
      serialLine[serialLineLen++] = c;
    }
  }
}

void enterSelectedApp(){
  if(menuSel==0){ activeApp=APP_WENYAO; drawWenYaoHome(); }
  else if(menuSel==1){ activeApp=APP_HUANGLI; drawHuangLiHome(); }
  else { menuHelpPage=0; drawMainMenuHelp(); }
}

void handleMenuLoop(){
  if(navigationEvent==Navigation::B_SHORT){ menuSel=(menuSel+1)%3; drawMainMenu(); }
  else if(navigationEvent==Navigation::A_SHORT) enterSelectedApp();
}

void handleMenuHelpLoop(){
  if(navigationEvent==Navigation::A_SHORT){
    if(menuHelpPage<MENU_HELP_PAGE_COUNT-1){ menuHelpPage++; drawMainMenuHelp(); }
    else drawMainMenu();
  } else if(navigationEvent==Navigation::B_SHORT){
    if(menuHelpPage>0){ menuHelpPage--; drawMainMenuHelp(); }
    else drawMainMenu();
  } else if(navigationEvent==Navigation::B_LONG) drawMainMenu();
}

void handleHuangLiLoop(){
  HuangLi::Screen before=HuangLi::screen;
  if(HuangLi::screen==HuangLi::HOME){
    if(HuangLi::dateNeedsRefresh()) drawHuangLiHome();
  } else HuangLi::tick();
  HuangLi::handleInput(navigationEvent);
  if(HuangLi::exitRequested){
    HuangLi::exitRequested=false;
    drawMainMenu();
  } else if(before!=HuangLi::HOME && HuangLi::screen==HuangLi::HOME) drawHuangLiHome();
}

void saveWenYaoMode(){
  Preferences prefs;
  if(!prefs.begin("wenyao-ui",false)){
    Serial.println("WARN MODE NOT SAVED");
    return;
  }
  if(prefs.putBool("simple",WenYao::simpleMode)!=1) Serial.println("WARN MODE NOT SAVED");
  prefs.end();
}

void loadWenYaoMode(){
  Preferences prefs;
  if(prefs.begin("wenyao-ui",true)){
    WenYao::simpleMode=prefs.getBool("simple",false);
    prefs.end();
  }
}

void handleWenYaoLoop(){
  bool wasSimple=WenYao::simpleMode;
  WenYao::handleInput(navigationEvent);
  if(wasSimple!=WenYao::simpleMode) saveWenYaoMode();
  if(WenYao::blockAnimationRelease){
    navigationReader.suppressUntilReleased();
    WenYao::blockAnimationRelease=false;
  }
  if(WenYao::exitConfirmed){
    WenYao::exitConfirmed=false;
    drawMainMenu();
  }
}

void setup(){
  HuangLi::huangli_setup();
  WenYao::wenyao_setup();
  loadWenYaoMode();
  loadLayout();
  menuCv.setColorDepth(16);
  menuCv.createSprite(W,H);
  productionAuthorized = loadProductionAuthorization();
  samplePower();
  Serial.printf("WenYao+HuangLi %s\n", COMBO_VERSION);
  Serial.printf("PROD %s\n", productionAuthorized ? "AUTHORIZED" : "LOCKED");
  if(productionAuthorized && shouldEnterChargeStandby()) enterChargeStandby();
  else if(productionAuthorized) drawMainMenu();
  else drawProductionLocked();
}

void loop(){
  bool powerUpdated = samplePower();
  handleSerialLayout();
  if(waitButtonsReleased){
    M5.update();
    if(!btnA().isPressed() && !btnB().isPressed()){
      waitButtonsReleased=false;
      suppressNextBRelease=false;
      menuBLongHandled=false;
      wenYaoHomeBLongHandled=false;
      armInputLock();
      navigationReader.suppressUntilReleased();
    }
    delay(10);
    return;
  }
  if(chargeStandby){
    M5.update();
    if(Battery::validInput(batteryMonitor.raw.inputMv) && batteryMonitor.raw.inputMv < 1000){
      exitChargeStandby();
      return;
    }
    if(powerButtonPressed() || btnA().pressedFor(800)){
      exitChargeStandby();
      delay(50);
      return;
    }
    if(btnA().wasPressed() || btnB().wasPressed()) showChargeStandbyMessage();
    if(chargeStandbyScreenOn && (int32_t)(millis() - chargeStandbyHideAt) >= 0) hideChargeStandbyMessage();
    if(chargeStandbyScreenOn && powerUpdated) drawChargeStandbyMessage();
    delay(50);
    return;
  }
  if(chargeBootBypass == CHARGE_BOOT_BYPASS_MAGIC && chargeBootBypassClearAt &&
     (int32_t)(millis() - chargeBootBypassClearAt) >= 0){
    chargeBootBypass = 0;
    chargeBootBypassClearAt = 0;
  }
  if(PROD_LOCK_ENABLED && !productionAuthorized){
    M5.update();
    delay(20);
    return;
  }
  M5.update();
  navigationEvent=navigationReader.poll(millis(),btnA().isPressed(),btnB().isPressed());
  if(activeApp == APP_MENU){
    if(powerUpdated){
      drawMenuBattery(13,210);
      menuCv.pushSprite(0,0);
    }
    handleMenuLoop();
  } else if(activeApp == APP_MENU_HELP){
    handleMenuHelpLoop();
  } else if(activeApp == APP_HUANGLI){
    handleHuangLiLoop();
  } else {
    handleWenYaoLoop();
  }
  delay(10);
}
