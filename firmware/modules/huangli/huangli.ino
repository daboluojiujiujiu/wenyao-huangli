// 黄历 HuangLi · M5StickS3 离线黄历
// 问爻同系风格: 宣纸浅底 / 墨色点阵 / 朱砂题签 / 版框
#include <M5Unified.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include "../common/input.h"

static constexpr int W = 135, H = 240;
static constexpr const char* APP_VERSION = "0.7.5";
static constexpr const char* VER = "版｜零.柒.伍";
static constexpr const char* TIME_AP_SSID = "HuangLi";
static const IPAddress TIME_AP_IP(192,168,8,8);

static constexpr uint16_t C_PAPER = lgfx::color565(231,220,194);
static constexpr uint16_t C_PAPER2= lgfx::color565(239,231,210);
static constexpr uint16_t C_INK   = lgfx::color565(38,33,26);
static constexpr uint16_t C_INKL  = lgfx::color565(156,144,120);
static constexpr uint16_t C_CIN   = lgfx::color565(178,58,46);
static constexpr uint16_t C_CIND  = lgfx::color565(138,44,34);
static constexpr uint16_t C_LINE  = lgfx::color565(189,151,88);
static constexpr uint16_t C_PAT   = lgfx::color565(111,138,106);

M5Canvas cv(&M5.Display);
Preferences prefs;
WebServer timeServer(80);
bool clockTrusted = false;
static constexpr long NO_DISPLAYED_DAY = INT32_MIN;
long lastDisplayedDay = NO_DISPLAYED_DAY;
uint32_t lastDatePollAt = 0;
int todayBias = 0;
int viewOffset = 0;
int currentBuildDateCode = 0;
bool btnALongPressHandled = false;
bool btnBLongPressHandled = false;
bool comboPressActive = false;
uint32_t comboPressStart = 0;
bool timeServerActive = false;
bool timeRoutesReady = false;
bool phoneSyncIgnoreRelease = false;

enum Screen { HOME, CONTENT, EXPLAIN, CONFIRM_TODAY, KEY_HELP, PHONE_SYNC, TOOLS };
Screen screen = HOME;
Screen confirmBackScreen = CONTENT;
Screen toolsBackScreen = HOME;
int toolChoice = 0;
bool exitRequested = false;

int tuneX(int x){ return x + ::LayoutLab::cfg.hx; }
int tuneY(int y){ return y + ::LayoutLab::cfg.hy; }
int tuneW(int w){ return constrain(w + ::LayoutLab::cfg.hw, 58, 122); }
void hRule(int x,int y,int w,uint16_t col){
  cv.drawFastHLine(tuneX(x),tuneY(y),w,col);
}

struct LunarDate {
  int year;
  int month;
  int day;
  bool leap;
};

struct DayInfo {
  int y, m, d, w;
  LunarDate lunar;
  int yearIdx, monthIdx, dayIdx, jianIdx, daoIdx;
  bool valid;
  char rel[16];
  char solar[32];
  char lunarLine[42];
  char gzLine[52];
  char dayLine[48];
  char yi[64];
  char ji[64];
  char clash[48];
  char direction[64];
};

// 1900-2100 农历资料: 月大小、闰月、闰月大小。
#include "calendar_data.h"

static const char* GAN[10] = {"甲","乙","丙","丁","戊","己","庚","辛","壬","癸"};
static const char* ZHI[12] = {"子","丑","寅","卯","辰","巳","午","未","申","酉","戌","亥"};
static const char* SHENGXIAO[12] = {"鼠","牛","虎","兔","龙","蛇","马","羊","猴","鸡","狗","猪"};
static const char* WEEK[7] = {"日","一","二","三","四","五","六"};
static const char* LMONTH[12] = {"正","二","三","四","五","六","七","八","九","十","冬","腊"};
static const char* LDAY_NUM[10] = {"十","一","二","三","四","五","六","七","八","九"};
static const char* JIAN[12] = {"建","除","满","平","定","执","破","危","成","收","开","闭"};
static const char* DAO[12] = {"青龙","明堂","天刑","朱雀","金匮","天德","白虎","玉堂","天牢","玄武","司命","勾陈"};
static const bool DAO_GOOD[12] = {true,true,false,false,true,true,false,true,false,false,true,false};

static const char* YI[12] = {
  "出行 上任 会友 立事",
  "扫舍 沐浴 解除 求医",
  "祭祀 祈福 纳财 置业",
  "修饰 平治 求医 安身",
  "订盟 交易 安床 纳财",
  "立约 祭祀 捕捉 守成",
  "破屋 坏垣 求医 除旧",
  "安床 祭祀 祈福 静养",
  "开市 交易 嫁娶 求财",
  "纳财 收账 祈福 入库",
  "开市 出行 求财 动身",
  "祭祀 静修 养生 收敛"
};

static const char* JI[12] = {
  "动土 开仓 争讼",
  "嫁娶 置业 远行",
  "动土 口舌 争讼",
  "开市 嫁娶 大举",
  "出行 争讼 破土",
  "搬迁 开仓 远行",
  "嫁娶 开市 入宅",
  "登高 远行 冒进",
  "争执 诉讼 动土",
  "开业 动土 远行",
  "安葬 破土 闭塞",
  "开市 出行 动土"
};

static const char* JIAN_SC[12] = {"建","除","满","平","定","执","破","危","成","收","开","闭"};

static const char* PLAIN_SC[12] = {
  "适合开始、见人、出门，先把事情立起来。",
  "适合清理旧事，把卡住的地方松开。",
  "适合收钱、置办、补齐资源，别贪多。",
  "适合修整、调养，把节奏稳下来。",
  "适合订约、定计划，定了就少反复。",
  "适合守成、按规矩办事，先稳住。",
  "适合拆旧、修坏、看病，处理问题。",
  "适合静养、留后手，凡事别冒进。",
  "适合成交、开市、办喜事，少争执。",
  "适合收账、入库、整理成果，先收住。",
  "适合出门、开张、启动，让事情动起来。",
  "适合休息、收心、整理，别急着开新局。"
};

static const char* TIP_SC[12] = {
  "做小开头，不要一口吃太大。",
  "先减负，再谈新安排。",
  "收进来可以，花出去要稳。",
  "把乱的地方理顺就好。",
  "适合把口头事落成准话。",
  "照计划走，比临时变动好。",
  "旧的不破，新的先别硬上。",
  "慢一点，留条后路。",
  "能成交就收口，别节外生枝。",
  "适合盘点，不适合扩张。",
  "想做的事可以先迈一步。",
  "少外放，多整理内部。"
};

static const char* YI_SC[12] = {
  "出门 见人 开始",
  "清理 整理 旧事",
  "收钱 置办 补齐",
  "修整 调养 稳住",
  "订约 交易 安顿",
  "守成 立约 查漏",
  "拆旧 修坏 看病",
  "静养 祈福 谨慎",
  "成交 开市 求财",
  "收账 入库 整理",
  "开张 出行 启动",
  "休息 收心 整理"
};

static const char* JI_SC[12] = {
  "动土 大改 争执",
  "婚嫁 置业 远行",
  "贪多 口舌 争执",
  "冒进 大举 开新",
  "反复改约 争执",
  "搬迁 远行 开仓",
  "开张 婚嫁 入宅",
  "冒进 远行 登高",
  "争执 诉讼 动土",
  "开业 动土 远行",
  "安葬 破土 收闭事",
  "出行 开业 动土"
};

static const char* XI_DIR[10] = {"东北","西北","西南","正南","东南","东北","西北","西南","正南","东南"};
static const char* CAI_DIR[10] = {"东北","东北","西南","西南","正北","正北","正东","正东","正南","正南"};
static const char* FU_DIR[10] = {"正北","西南","西北","东南","东北","正北","西南","西北","东南","东北"};
static const char* SHA_DIR[12] = {"南","东","北","西","南","东","北","西","南","东","北","西"};

void setF(int n){
  cv.setTextSize(1);
  switch(n){
    case 10: cv.setFont(&::TextFonts::cn10); break;
    case 12: cv.setFont(&::TextFonts::cn12); break;
    case 14: cv.setFont(&::TextFonts::cn14); break;
    case 16: cv.setFont(&::TextFonts::cn16); break;
    default: cv.setFont(&::TextFonts::cn24); break;
  }
}

void tc(const char* s,int x,int y,int n,uint16_t col){
  setF(n); cv.setTextDatum(middle_center); cv.setTextColor(col);
  cv.drawString(s,tuneX(x),tuneY(y));
}

void tcFit(const char* s,int x,int y,int n,uint16_t col,int maxW){
  maxW = tuneW(maxW);
  int size=n;
  while(size>10){
    setF(size);
    if((int)cv.textWidth(s)<=maxW) break;
    size-=2;
  }
  setF(size); cv.setTextDatum(middle_center); cv.setTextColor(col);
  cv.drawString(s,tuneX(x),tuneY(y));
}

void tl(const char* s,int x,int y,int n,uint16_t col){
  setF(n); cv.setTextDatum(middle_left); cv.setTextColor(col);
  cv.drawString(s,tuneX(x),tuneY(y));
}

int utf8Len(unsigned char b){
  if((b & 0x80) == 0) return 1;
  if((b & 0xE0) == 0xC0) return 2;
  if((b & 0xF0) == 0xE0) return 3;
  if((b & 0xF8) == 0xF0) return 4;
  return 1;
}

void drawWrapped(const char* s,int x,int y,int maxW,int n,int lineH,int maxLines,uint16_t col){
  x=tuneX(x); y=tuneY(y); maxW=tuneW(maxW);
  const int height=n+(maxLines-1)*lineH;
  char lines[10][96]{};
  TextLayout::WrapResult result{};
  int size=n,step=lineH;
  auto width=[](const char* text){ return (int)cv.textWidth(text); };
  for(;;){
    setF(size);
    int limit=max(1,min(10,1+(height-size)/step));
    result=TextLayout::wrap(s,maxW,lines,limit,width);
    if(result.complete || size<=10) break;
    size-=2;
    step=max(size,lineH-(n-size));
  }
  if(!result.complete && result.count) TextLayout::ellipsize(lines[result.count-1],maxW,width);
  cv.setTextDatum(top_left); cv.setTextColor(col);
  for(int i=0;i<result.count;i++) cv.drawString(lines[i],x,y+i*step);
}
void paperBg(){
  cv.fillScreen(C_PAPER);
  randomSeed(7);
  for(int i=0;i<240;i++) cv.drawPixel(random(W),random(H), lgfx::color565(214,202,174));
  for(int i=0;i<36;i++)  cv.drawPixel(random(W),random(H), lgfx::color565(198,184,154));
}

void frame(bool guideLines=true){
  cv.drawRect(5,5,W-10,H-10,C_LINE);
  cv.drawRect(8,8,W-16,H-16,C_INKL);
  if(guideLines){
    cv.drawFastHLine(15,31,W-30,C_INKL);
    cv.drawFastHLine(15,209,W-30,C_INKL);
  }
  cv.drawLine(14,14,22,14,C_CIN);
  cv.drawLine(14,14,14,22,C_CIN);
  cv.drawLine(W-15,14,W-23,14,C_CIN);
  cv.drawLine(W-15,14,W-15,22,C_CIN);
  cv.drawLine(14,H-15,22,H-15,C_CIN);
  cv.drawLine(14,H-15,14,H-23,C_CIN);
  cv.drawLine(W-15,H-15,W-23,H-15,C_CIN);
  cv.drawLine(W-15,H-15,W-15,H-23,C_CIN);
}

void thinRule(int y){
  hRule(22,y,W-44,C_INKL);
}

void seal(int x,int y){
  cv.fillRect(x,y,33,33,C_CIN);
  cv.drawRect(x+3,y+3,27,27,C_PAPER);
  setF(24);
  cv.setTextDatum(middle_center);
  cv.setTextColor(C_PAPER);
  cv.drawString("历",x+16,y+17);
}

long daysFromCivil(int y,unsigned m,unsigned d){
  y -= m <= 2;
  const int era = (y >= 0 ? y : y-399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097L + (long)doe - 719468L;
}

int leapMonth(int y){ return LUNAR_INFO[y-1900] & 0xF; }
int leapDays(int y){
  int lm=leapMonth(y);
  if(!lm) return 0;
  return (LUNAR_INFO[y-1900] & 0x10000) ? 30 : 29;
}
int monthDays(int y,int m){ return (LUNAR_INFO[y-1900] & (0x10000 >> m)) ? 30 : 29; }
int yearDays(int y){
  int sum=348;
  for(uint32_t bit=0x8000; bit>0x8; bit>>=1) if(LUNAR_INFO[y-1900] & bit) sum++;
  return sum + leapDays(y);
}

bool solarToLunar(int y,int m,int d,LunarDate& out){
  if(y<1900 || y>2100) return false;
  long offset = daysFromCivil(y,m,d) - daysFromCivil(1900,1,31);
  if(offset < 0) return false;
  int ly=1900;
  while(ly<2101){
    int yd=yearDays(ly);
    if(offset < yd) break;
    offset -= yd;
    ly++;
  }
  int lm=1, leap=leapMonth(ly);
  bool isLeap=false;
  while(lm<=12){
    int md=monthDays(ly,lm);
    if(offset < md) break;
    offset -= md;
    if(leap==lm){
      isLeap=true;
      md=leapDays(ly);
      if(offset < md) break;
      offset -= md;
      isLeap=false;
    }
    lm++;
  }
  out.year=ly; out.month=lm; out.day=(int)offset+1; out.leap=isLeap;
  return true;
}

long julianDay(int y,int m,int d){
  int a=(14-m)/12;
  y=y+4800-a;
  m=m+12*a-3;
  return d + (153*m+2)/5 + 365L*y + y/4 - y/100 + y/400 - 32045;
}

void gzName(int idx,char* out,int len){
  idx=(idx%60+60)%60;
  snprintf(out,len,"%s%s",GAN[idx%10],ZHI[idx%12]);
}

void lunarDayName(int d,char* out,int len){
  if(d==10){ snprintf(out,len,"初十"); return; }
  if(d==20){ snprintf(out,len,"二十"); return; }
  if(d==30){ snprintf(out,len,"三十"); return; }
  const char* p = d<10 ? "初" : (d<20 ? "十" : (d<30 ? "廿" : "卅"));
  snprintf(out,len,"%s%s",p,LDAY_NUM[d%10]);
}

int monthIndex(const char* m){
  static const char* names[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  for(int i=0;i<12;i++) if(strncmp(m,names[i],3)==0) return i+1;
  return 1;
}

time_t buildEpochFromCompiler(struct tm* buildTm){
  char mon[4]={0};
  int day=1, year=2026, hh=0, mm=0, ss=0;
  sscanf(__DATE__,"%3s %d %d",mon,&day,&year);
  sscanf(__TIME__,"%d:%d:%d",&hh,&mm,&ss);
  struct tm t{};
  t.tm_year=year-1900;
  t.tm_mon=monthIndex(mon)-1;
  t.tm_mday=day;
  t.tm_hour=hh;
  t.tm_min=mm;
  t.tm_sec=ss;
  currentBuildDateCode = year * 10000 + monthIndex(mon) * 100 + day;
  if(buildTm) *buildTm = t;
  return mktime(&t);
}

bool rtcEpochLocal(time_t& epoch){
  if(!M5.Rtc.isEnabled()) return false;
  m5::rtc_datetime_t dt;
  if(!M5.Rtc.getDateTime(&dt)) return false;
  if(dt.date.year < 2024 || dt.date.year > 2100) return false;
  if(dt.date.month < 1 || dt.date.month > 12) return false;
  if(dt.date.date < 1 || dt.date.date > 31) return false;
  if(dt.time.hours > 23 || dt.time.minutes > 59 || dt.time.seconds > 59) return false;
  struct tm t{};
  t.tm_year = dt.date.year - 1900;
  t.tm_mon = dt.date.month - 1;
  t.tm_mday = dt.date.date;
  t.tm_hour = dt.time.hours;
  t.tm_min = dt.time.minutes;
  t.tm_sec = dt.time.seconds;
  epoch = mktime(&t);
  return t.tm_year == dt.date.year-1900 && t.tm_mon == dt.date.month-1 &&
         t.tm_mday == dt.date.date;
}

void initClockFromBuild(){
  setenv("TZ","CST-8",1);
  tzset();
  time_t buildEpoch = buildEpochFromCompiler(nullptr);
  time_t chosenEpoch = buildEpoch, rtcEpoch = 0;
  clockTrusted = rtcEpochLocal(rtcEpoch);
  if(clockTrusted) chosenEpoch = rtcEpoch;
  else {
    Preferences clockPrefs;
    if(clockPrefs.begin("clockstate",true)){
      int64_t saved = clockPrefs.getLong64("lastEpoch",0);
      clockPrefs.end();
      if(saved >= 1704067200LL && saved <= 4102444799LL) chosenEpoch=(time_t)saved;
    }
  }
  // A last-known value is a browsing reference, never proof of elapsed power-off time.
  struct timeval tv{chosenEpoch,0};
  settimeofday(&tv,nullptr);
}

void loadTodayBiasForBuild(){
  int storedBuildDate = prefs.getInt("buildDate",0);
  if(storedBuildDate != currentBuildDateCode){
    todayBias = 0;
    viewOffset = 0;
    prefs.putInt("todayBias",0);
    prefs.putInt("buildDate",currentBuildDateCode);
    return;
  }
  todayBias = prefs.getInt("todayBias",0);
}

void buildDayInfo(DayInfo& info){
  time_t now=time(nullptr) + (time_t)(todayBias + viewOffset) * 86400;
  struct tm t{};
  localtime_r(&now,&t);
  info.y=t.tm_year+1900;
  info.m=t.tm_mon+1;
  info.d=t.tm_mday;
  info.w=t.tm_wday;
  info.valid=true;
  if(viewOffset==0) snprintf(info.rel,sizeof(info.rel),clockTrusted ? "今日" : "参考日");
  else if(viewOffset==1) snprintf(info.rel,sizeof(info.rel),"明日");
  else if(viewOffset==-1) snprintf(info.rel,sizeof(info.rel),"昨日");
  else snprintf(info.rel,sizeof(info.rel),"%+d日",viewOffset);
  snprintf(info.solar,sizeof(info.solar),"%04d年%02d月%02d日 周%s",info.y,info.m,info.d,WEEK[info.w]);

  if(!solarToLunar(info.y,info.m,info.d,info.lunar)){
    info.valid=false;
    snprintf(info.lunarLine,sizeof(info.lunarLine),"超出日期范围");
    return;
  }
  char lday[12], ygz[8], mgz[8], dgz[8];
  lunarDayName(info.lunar.day,lday,sizeof(lday));
  snprintf(info.lunarLine,sizeof(info.lunarLine),"农历%s%s月%s",
           info.lunar.leap?"闰":"", LMONTH[info.lunar.month-1], lday);

  info.yearIdx=(info.lunar.year-4)%60;
  // The twelve Jie change the month pillar on their Beijing civil date.
  int monthBranch = info.d >= JIE_DAY[info.y-1900][info.m-1] ? info.m%12 : (info.m+11)%12;
  int jieYear = info.y - ((info.m < 2 || (info.m == 2 && info.d < JIE_DAY[info.y-1900][1])) ? 1 : 0);
  int yStem = ((jieYear-4)%10+10)%10;
  info.monthIdx = (yStem%5*12 + 2 + (monthBranch+10)%12)%60;
  int monthStem=info.monthIdx%10;
  info.dayIdx=(int)((julianDay(info.y,info.m,info.d)+49)%60);
  gzName(info.yearIdx,ygz,sizeof(ygz));
  snprintf(mgz,sizeof(mgz),"%s%s",GAN[monthStem],ZHI[monthBranch]);
  gzName(info.dayIdx,dgz,sizeof(dgz));
  snprintf(info.gzLine,sizeof(info.gzLine),"%s年  %s月  %s日",ygz,mgz,dgz);

  int dayStem=info.dayIdx%10;
  int dayBranch=info.dayIdx%12;
  info.jianIdx=(dayBranch - monthBranch + 12) % 12;
  int daoStart=(2*(monthBranch+4))%12;
  info.daoIdx=(dayBranch-daoStart+12)%12;
  snprintf(info.dayLine,sizeof(info.dayLine),"%s日 · %s%s",
           JIAN[info.jianIdx], DAO[info.daoIdx], DAO_GOOD[info.daoIdx]?"黄道":"黑道");
  snprintf(info.yi,sizeof(info.yi),"%s",YI[info.jianIdx]);
  snprintf(info.ji,sizeof(info.ji),"%s",JI[info.jianIdx]);
  int clash=(dayBranch+6)%12;
  snprintf(info.clash,sizeof(info.clash),"冲%s 煞%s",SHENGXIAO[clash],SHA_DIR[dayBranch]);
  snprintf(info.direction,sizeof(info.direction),"喜%s 财%s 福%s",XI_DIR[dayStem],CAI_DIR[dayStem],FU_DIR[dayStem]);
}

void formatHomeClock(char* out,size_t len){
  if(!clockTrusted){
    snprintf(out,len,"请校准日期");
    return;
  }
  time_t now = time(nullptr) + (time_t)todayBias * 86400;
  struct tm t{};
  localtime_r(&now,&t);
  snprintf(out,len,"%d/%d/%d",
           t.tm_year+1900,t.tm_mon+1,t.tm_mday);
}

void drawPhoneSync(const char* status){
  paperBg();
  frame(false);
  tc("手机校时",W/2,32,16,C_CIN);
  thinRule(54);
  tl("WiFi",18,76,12,C_CIN);
  tl(TIME_AP_SSID,54,74,16,C_INK);
  tl("地址",18,104,12,C_CIN);
  tc("192.168.8.8",W/2,124,16,C_INK);
  thinRule(138);
  drawWrapped("手机连接此WiFi后，打开上面地址，点一键校时。",16,158,103,12,17,3,C_INK);
  if(status && status[0]) tc(status,W/2,216,12,C_CIN);
  cv.pushSprite(0,0);
}

void applyPhoneEpoch(time_t epoch){
  struct timeval tv{epoch,0};
  settimeofday(&tv,nullptr);
  struct tm localTm{};
  localtime_r(&epoch,&localTm);
  if(M5.Rtc.isEnabled()) M5.Rtc.setDateTime(&localTm);
  todayBias = 0;
  viewOffset = 0;
  prefs.putInt("todayBias",0);
  clockTrusted = true;
  Preferences clockPrefs;
  if(clockPrefs.begin("clockstate",false)){
    clockPrefs.putLong64("lastEpoch",(int64_t)epoch);
    clockPrefs.end();
  }
  lastDisplayedDay = NO_DISPLAYED_DAY;
}

void handleTimeRoot(){
  drawPhoneSync("手机已连接");
  static const char html[] =
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>HuangLi校时</title>"
    "<style>body{font-family:-apple-system,BlinkMacSystemFont,'PingFang SC',sans-serif;background:#efe7d2;color:#26211a;padding:24px;line-height:1.6}"
    ".box{max-width:420px;margin:auto}.time{font-size:28px;font-weight:700;margin:18px 0}"
    "button{width:100%;font-size:20px;font-weight:700;padding:14px;border:0;border-radius:10px;background:#b23a2e;color:#fff}"
    "#msg{font-weight:700;color:#8a2c22}</style></head><body><div class='box'>"
    "<h2>HuangLi 手机校时</h2><p>手机当前时间</p><div id='now' class='time'></div>"
    "<button onclick='syncTime()'>一键校时</button><p id='msg'></p></div>"
    "<script>"
    "function pad(n){return n<10?'0'+n:n}"
    "function fmt(d){return d.getFullYear()+'/'+(d.getMonth()+1)+'/'+d.getDate()+' '+d.getHours()+':'+pad(d.getMinutes())}"
    "function tick(){document.getElementById('now').textContent=fmt(new Date())}"
    "async function syncTime(){let ts=Math.floor(Date.now()/1000);let m=document.getElementById('msg');m.textContent='校时中...';"
    "try{let r=await fetch('/sync?ts='+ts+'&_='+Date.now());let t=await r.text();m.textContent=r.ok?'校时完成，可以返回设备查看。':t}"
    "catch(e){m.textContent='校时失败，请确认仍连接HuangLi。'}}"
    "tick();setInterval(tick,1000);"
    "</script></body></html>";
  timeServer.sendHeader("Cache-Control","no-store");
  timeServer.send(200,"text/html; charset=utf-8",html);
}

void handleTimeSync(){
  String tsArg = timeServer.arg("ts");
  char* end=nullptr;
  long long raw = tsArg.length() ? strtoll(tsArg.c_str(),&end,10) : 0;
  if(!end || *end){ timeServer.send(400,"text/plain; charset=utf-8","时间无效"); return; }
  if(raw < 1704067200LL || raw > 4102444799LL){
    timeServer.send(400,"text/plain; charset=utf-8","时间无效");
    drawPhoneSync("时间无效");
    return;
  }
  applyPhoneEpoch((time_t)raw);
  timeServer.send(200,"text/plain; charset=utf-8","OK");
  drawPhoneSync("校时成功");
}

void startPhoneSync(){
  WiFi.mode(WIFI_AP);
  WiFi.softAPdisconnect(true);
  delay(80);
  WiFi.softAPConfig(TIME_AP_IP,TIME_AP_IP,IPAddress(255,255,255,0));
  bool ok = WiFi.softAP(TIME_AP_SSID);
  if(!timeRoutesReady){
    timeServer.on("/",HTTP_GET,handleTimeRoot);
    timeServer.on("/sync",HTTP_GET,handleTimeSync);
    timeRoutesReady = true;
  }
  timeServer.begin();
  timeServerActive = true;
  phoneSyncIgnoreRelease = true;
  drawPhoneSync(ok ? "等待手机连接" : "WiFi启动失败");
}

void stopPhoneSync(){
  if(timeServerActive){
    timeServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    timeServerActive = false;
  }
  phoneSyncIgnoreRelease = false;
  comboPressActive = false;
  btnALongPressHandled = false;
  btnBLongPressHandled = false;
}

void drawHome(){
  paperBg();
  frame(false);
  char clockLine[24];
  formatHomeClock(clockLine,sizeof(clockLine));
  tcFit(clockLine,W/2,30,16,C_INK,W-10);
  seal(51,44);

  tc("黄历",W/2,96,24,C_INK);
  tc("通",43,120,16,C_CIN);
  tc("书",59,120,16,C_CIN);
  tc("择",75,120,16,C_CIN);
  tc("日",91,120,16,C_CIN);
  hRule(26,139,W-52,C_INKL);

  tc("短A：今日",W/2,162,14,C_INK);
  tc("长A：日期功能",W/2,184,14,C_INK);
  tc(VER,W/2,224,12,C_INKL);

  cv.pushSprite(0,0);
}

void drawInvalidDate(){
  paperBg(); frame(false);
  tc("日期超出范围",W/2,88,16,C_CIN);
  tc("1900/1/31",W/2,120,14,C_INK);
  tc("2100/12/31",W/2,146,14,C_INK);
  cv.pushSprite(0,0);
}

void drawContent(){
  DayInfo info;
  buildDayInfo(info);
  if(!info.valid){ drawInvalidDate(); return; }
  paperBg();
  frame(false);

  tc(info.rel,W/2,20,16,C_CIN);
  tcFit(info.solar,W/2,39,12,C_INK,W-16);
  tcFit(info.lunarLine,W/2,55,12,C_INKL,W-16);
  tcFit(info.gzLine,W/2,71,12,C_INK,W-10);
  thinRule(83);
  tcFit(info.dayLine,W/2,94,14,DAO_GOOD[info.daoIdx]?C_CIN:C_CIND,W-12);

  tl("宜",14,120,16,C_CIN);
  drawWrapped(info.yi,36,109,86,14,16,2,C_INK);
  tl("忌",14,160,16,C_CIND);
  drawWrapped(info.ji,36,149,86,14,16,2,C_INK);

  tcFit(info.direction,W/2,199,12,C_INK,W-8);
  thinRule(211);
  tcFit(info.clash,W/2,224,12,C_INKL,W-18);

  cv.pushSprite(0,0);
  Serial.printf("HuangLi %s %04d-%02d-%02d offset=%d\n",APP_VERSION,info.y,info.m,info.d,viewOffset);
}

void drawExplain(){
  DayInfo info;
  buildDayInfo(info);
  if(!info.valid){ drawInvalidDate(); return; }

  paperBg();
  frame(false);

  char title[32];
  snprintf(title,sizeof(title),"%s日解",JIAN_SC[info.jianIdx]);
  char explain[192];
  snprintf(explain,sizeof(explain),"%s%s",PLAIN_SC[info.jianIdx],TIP_SC[info.jianIdx]);
  tc(title,W/2,27,16,C_CIN);
  thinRule(42);
  drawWrapped(explain,14,50,107,14,18,5,C_INK);
  thinRule(136);
  tl("宜做",14,153,14,C_CIN);
  drawWrapped(YI_SC[info.jianIdx],50,142,71,14,17,2,C_INK);
  tl("忌做",14,192,14,C_CIND);
  drawWrapped(JI_SC[info.jianIdx],50,181,71,14,17,2,C_INK);

  cv.pushSprite(0,0);
}

void drawConfirmToday(){
  DayInfo info;
  buildDayInfo(info);
  if(!info.valid){ drawInvalidDate(); return; }

  paperBg();
  frame(false);

  tc("设为今日？",W/2,40,16,C_CIN);
  thinRule(58);
  tcFit(info.solar,W/2,82,12,C_INK,W-16);
  tcFit(info.lunarLine,W/2,101,12,C_INKL,W-16);
  tcFit(info.dayLine,W/2,124,14,DAO_GOOD[info.daoIdx]?C_CIN:C_CIND,W-12);
  thinRule(143);
  tl("A",24,170,14,C_CIN);
  tl("确认",62,170,14,C_INK);
  tl("B",24,199,14,C_CIN);
  tl("取消",48,199,14,C_INK);

  cv.pushSprite(0,0);
}

void drawKeyHelp(){
  paperBg(); frame(false);
  tc("操作提示",W/2,36,16,C_CIN);
  thinRule(55);
  tc("短A：下一页",W/2,83,14,C_INK);
  tc("B：返回",W/2,109,14,C_INK);
  tc("长A：日期功能",W/2,140,14,C_INK);
  tc("换日、校时",W/2,165,14,C_INK);
  tc("均在功能菜单",W/2,186,14,C_INK);
  cv.pushSprite(0,0);
}

void drawTools(){
  paperBg(); frame(false);
  tc("黄历功能",W/2,30,16,C_CIN);
  static const char* choices[]={"后一日","前一日","回到今日","设为今日","手机校时"};
  for(int i=0;i<5;i++){
    int y=54+i*27;
    if(i==toolChoice) cv.drawRect(18,y,99,25,C_CIN);
    tc(choices[i],W/2,y+12,16,i==toolChoice ? C_CIN : C_INK);
  }
  tc("B切换 A选定",W/2,201,12,C_INK);
  tc("长B返回",W/2,221,12,C_INKL);
  cv.pushSprite(0,0);
}

void redrawCurrentScreen(){
  if(screen == CONTENT) drawContent();
  else if(screen == EXPLAIN) drawExplain();
  else if(screen == CONFIRM_TODAY) drawConfirmToday();
  else if(screen == KEY_HELP) drawKeyHelp();
  else if(screen == TOOLS) drawTools();
  else if(screen != PHONE_SYNC) drawHome();
}

void openTools(){
  toolsBackScreen=screen;
  screen=TOOLS;
  drawTools();
}

void shiftDay(int delta){
  if(screen != CONTENT && screen != EXPLAIN) return;
  time_t proposed=time(nullptr)+(time_t)(todayBias+viewOffset+delta)*86400;
  struct tm t{}; localtime_r(&proposed,&t);
  long day=daysFromCivil(t.tm_year+1900,t.tm_mon+1,t.tm_mday);
  if(day<daysFromCivil(1900,1,31) || day>daysFromCivil(2100,12,31)) return;
  viewOffset += delta;
  redrawCurrentScreen();
}

void confirmSetToday(){
  todayBias += viewOffset;
  clockTrusted = true;
  viewOffset = 0;
  prefs.putInt("todayBias",todayBias);
  screen = confirmBackScreen;
  redrawCurrentScreen();
}

void handleInput(Navigation::Event event){
  using namespace Navigation;
  if(event==NONE) return;
  if(event==B_LONG){
    if(screen==TOOLS){ screen=toolsBackScreen; redrawCurrentScreen(); return; }
    if(timeServerActive) stopPhoneSync();
    screen=HOME; exitRequested=true; return;
  }
  const bool next=event==A_SHORT, confirm=event==A_LONG, back=event==B_SHORT;
  if(screen==TOOLS){
    if(back){ toolChoice=(toolChoice+1)%5; drawTools(); }
    else if(next){
      if(toolChoice==4){ screen=PHONE_SYNC; startPhoneSync(); }
      else {
        screen=toolsBackScreen==EXPLAIN ? EXPLAIN : CONTENT;
        if(toolsBackScreen==HOME) viewOffset=0;
        if(toolChoice<2){ redrawCurrentScreen(); shiftDay(toolChoice==0 ? 1 : -1); }
        else if(toolChoice==2){ viewOffset=0; redrawCurrentScreen(); }
        else { confirmBackScreen=screen; screen=CONFIRM_TODAY; drawConfirmToday(); }
      }
    }
  } else if(screen==CONFIRM_TODAY){
    if(next) confirmSetToday();
    else if(back){ screen=TOOLS; drawTools(); }
  } else if(screen==PHONE_SYNC){
    if(back){ stopPhoneSync(); screen=TOOLS; drawTools(); }
  } else if(screen==KEY_HELP){
    if(next || back){ screen=HOME; drawHome(); }
  } else if(confirm) openTools();
  else if(next){
    if(screen==HOME){ viewOffset=0; screen=CONTENT; }
    else screen=screen==CONTENT ? EXPLAIN : CONTENT;
    redrawCurrentScreen();
  } else if(back){
    if(screen==HOME) exitRequested=true;
    else { screen=screen==EXPLAIN ? CONTENT : HOME; redrawCurrentScreen(); }
  }
}

bool dateNeedsRefresh(){
  if(millis()-lastDatePollAt < 500) return false;
  lastDatePollAt=millis();
  time_t now=time(nullptr)+(time_t)todayBias*86400;
  struct tm t{}; localtime_r(&now,&t);
  long day=daysFromCivil(t.tm_year+1900,t.tm_mon+1,t.tm_mday);
  if(lastDisplayedDay==day) return false;
  if(lastDisplayedDay==NO_DISPLAYED_DAY){ lastDisplayedDay=day; return false; }
  if(viewOffset!=0) viewOffset-=(int)(day-lastDisplayedDay);
  lastDisplayedDay=day;
  return true;
}

void setup(){
  auto cfg=M5.config();
  cfg.fallback_board = m5::board_t::board_M5StickS3;
  M5.begin(cfg);
  Serial.begin(115200);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(180);
  cv.setColorDepth(16);
  cv.createSprite(W,H);
  initClockFromBuild();
  prefs.begin("huangli",false);
  loadTodayBiasForBuild();
  drawHome();
}

void tick(){
  if(screen==PHONE_SYNC && timeServerActive) timeServer.handleClient();
  if(dateNeedsRefresh() && (screen==HOME || screen==CONTENT || screen==EXPLAIN)) redrawCurrentScreen();
}

void loop(){
  static Navigation::Reader reader;
  M5.update();
  tick();
  handleInput(reader.poll(millis(),btnA().isPressed(),btnB().isPressed()));
  delay(10);
}
