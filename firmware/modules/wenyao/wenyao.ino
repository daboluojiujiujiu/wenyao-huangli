// 问爻 WenYao · M5StickS3 · 摇卦、选卦、自定六爻
// 古籍刻本风:宣纸浅底 / 墨色宋体点阵 / 朱砂印 / 古铜钱 / 卦牌 / 版框
#include <M5Unified.h>
#include "../common/input.h"

static constexpr int W = 135, H = 240;
static constexpr int BODY_W = 104;

// ---- 调色板 (RGB565) ----
static constexpr uint16_t C_PAPER = lgfx::color565(231,220,194);
static constexpr uint16_t C_PAPER2= lgfx::color565(239,231,210);
static constexpr uint16_t C_INK   = lgfx::color565(38,33,26);
static constexpr uint16_t C_INKL  = lgfx::color565(156,144,120);
static constexpr uint16_t C_CIN   = lgfx::color565(178,58,46);
static constexpr uint16_t C_CIND  = lgfx::color565(138,44,34);
static constexpr uint16_t C_YIND  = lgfx::color565(70,96,72);
static constexpr uint16_t C_YINL  = lgfx::color565(42,62,44);
static constexpr uint16_t C_BL    = lgfx::color565(201,163,90);
static constexpr uint16_t C_BM    = lgfx::color565(154,117,54);
static constexpr uint16_t C_BD    = lgfx::color565(94,70,36);
static constexpr uint16_t C_PAT   = lgfx::color565(111,138,106);
static constexpr uint16_t C_SHAD  = lgfx::color565(150,130,90);
// 铜钱背面:暗橄榄铜绿(与正面金黄明显不同)
static constexpr uint16_t C_BACKM = lgfx::color565(104,118,84);
static constexpr uint16_t C_BACKH = lgfx::color565(138,150,108);
static constexpr uint16_t C_BACKD = lgfx::color565(64,76,52);

M5Canvas cv(&M5.Display);

// ---- 易经数据 ----
int vIdx[8];                       // 三爻值 -> 八卦表序(乾兑离震巽坎艮坤)
const char* NAMES[8][8] = {
 {"乾","夬","大有","大壮","小畜","需","大畜","泰"},
 {"履","兑","睽","归妹","中孚","节","损","临"},
 {"同人","革","离","丰","家人","既济","贲","明夷"},
 {"无妄","随","噬嗑","震","益","屯","颐","复"},
 {"姤","大过","鼎","恒","巽","井","蛊","升"},
 {"讼","困","未济","解","涣","坎","蒙","师"},
 {"遁","咸","旅","小过","渐","蹇","艮","谦"},
 {"否","萃","晋","豫","观","比","剥","坤"}};
const char* ETRI[8] = {"乾","兑","离","震","巽","坎","艮","坤"};
const char* EELE[8] = {"天","泽","火","雷","风","水","山","地"};
struct GM { const char* n; const char* m; const char* b; };  // 名/八字断语/大白话
const GM MEAN[] = {
 {"乾","自强不息","刚健旺盛,正该主动发力。顺势去做就顺,但别逞强过头"},
 {"坤","厚德载物","宜柔顺包容、不抢先。跟着大势走、踏实积累,自有所成"},
 {"屯","始生艰难,宜建侯","起步艰难、头绪乱。先扎稳根基、找对帮手,别急着冲"},
 {"蒙","蒙以养正,果行育德","像孩童蒙昧未开。放低姿态虚心求教,把基础打正才通"},
 {"需","需待时机,饮食宴乐","时机未到,强求无益。沉住气耐心等待,养精蓄锐再动"},
 {"讼","慎争息讼,作事谋始","易生争执口角。能让则让、把话讲在前头,别把关系闹僵"},
 {"师","行险而顺,以此毒天下","如统兵作战。要师出有名、立好规矩、有人带头方稳"},
 {"比","亲辅相比,不宁方来","宜亲近志同道合者。趁早结交、彼此扶持,迟疑则失良缘"},
 {"小畜","密云不雨,小有蓄积","力量尚弱、难成大事。先小处积累、稳扎稳打,大事暂缓"},
 {"履","履虎尾不咬,履而能泰","如履薄冰、步步惊心。守礼数、存敬畏、行事谨慎可化险"},
 {"泰","天地交泰,小往大来","上下通达、诸事顺遂。正是大好时机,顺势而为把握当下"},
 {"否","天地不交,君子俭德","闭塞不通、处境不顺。宜低调收敛、避开是非,静待转机"},
 {"同人","与人同心,利涉大川","与人同心同德。开诚布公、团结协作,再难的事也能成"},
 {"大有","火在天上,顺天休命","收获丰盛、家业兴旺。顺势行善积德,切忌骄纵自满"},
 {"谦","谦谦君子,卑以自牧","为人谦逊低调最得福。越放低姿态,越赢人心与长远"},
 {"豫","顺以动,利建侯行师","顺势而动、准备充分。此时适合行动作为,可放手去做"},
 {"随","随时而动,泽中有雷","宜随机应变、顺势而行。不固执己见,跟对趋势就有收获"},
 {"蛊","整饬积弊,先甲后甲","积弊已深、亟待整治。下决心革除旧弊、收拾烂摊子方振"},
 {"临","刚浸而长,临之以正","阳气上升、形势转好。趁这股向上的劲头主动作为,可期"},
 {"观","观民设教,中正以观","宜静观其变、看清全局。不急于出手,摸透形势再决定"},
 {"噬嗑","颐中有物,啮合乃通","前路有梗阻。要果断排除障碍、咬碎硬骨头,事才打通"},
 {"贲","文饰之道,刚柔交错","重在修饰外表、讲究体面。但别只顾门面,内在充实才久"},
 {"剥","剥落之时,顺而止之","处境衰落、根基受损。顺时收手、保存实力,不可硬撑"},
 {"复","一阳来复,反复其道","否极泰来、一阳来复。低谷已过,正可重整旗鼓再出发"},
 {"无妄","不妄为,动而愈健","切忌妄动妄求、投机取巧。守住本分、踏实做事自顺遂"},
 {"大畜","刚健笃实,日新其德","宜积蓄力量与德行。厚积才能薄发,沉住气把底子打厚"},
 {"颐","慎言语,节饮食","重在养。管好饮食言语、调养身心,养对了路自然有福"},
 {"大过","栋桡之时,独立不惧","担子过重、压力非常。非常时期需有担当、敢顶上不惧"},
 {"坎","重险习坎,行险有尚","重重险阻、危机四伏。沉住气、怀诚心、步步谨慎方脱险"},
 {"离","附丽乎正,继明照四方","宜依附正道、保持光明。像火持续发光,照亮己也照人"},
 {"咸","二气感应,虚受于人","彼此感应、两情相投。以诚相待、虚心接纳,水到渠成"},
 {"恒","恒久之道,立不易方","贵在持之以恒。守住正道、不轻易动摇,坚持终见成效"},
 {"遁","远小人,遁世无闷","宜激流勇退、保全自身。远离小人是非,该退则退留清白"},
 {"大壮","雷在天上,非礼弗履","气势正盛、力量强大。但越强越要守礼守度,忌恃强蛮干"},
 {"晋","明出地上,自昭明德","如旭日东升、前程光明。正宜主动进取、积极向上大胆为"},
 {"明夷","明入地中,晦而守志","光明受挫、贤者受困。宜收敛锋芒、外晦内明,守志待时"},
 {"家人","正家之道,言而有物","重在齐家。各守本分、言行有信,家和则万事兴"},
 {"睽","异中求同,小事可吉","彼此乖违、意见相左。求同存异、从小处入手,小事可成"},
 {"蹇","见险而止,反身修德","前有险阻、举步维艰。宜停下脚步、反躬自省,修己再行"},
 {"解","险难既解,赦过宥罪","困境开始松动化解。宽待过往、放下包袱,轻装上阵正当时"},
 {"损","损下益上,惩忿窒欲","需有所舍弃、损己利人。克制私欲、惩忿窒欲,舍小得大"},
 {"益","损上益下,见善则迁","正处受益增长之时。见善则学、积极进取,趁势而上多得"},
 {"夬","决而能和,施禄及下","到了该决断的关头。果断果决,但要讲方法、得人心别硬来"},
 {"姤","阴遇阳,勿用取女","不期而遇、阴长之始。提防来意不明者,莫贸然纠缠招祸"},
 {"萃","聚而有正,戒备不虞","人心聚拢、群贤会集。趁势团结众人,同时防患于未然"},
 {"升","柔以时升,积小高大","如苗木上长、步步高升。积小成大、顺势而上,前景向好"},
 {"困","处困守正,致命遂志","处境困顿、四面受限。守住正道、咬牙坚持,困极而后通"},
 {"井","井养不穷,劳民劝相","如水井养人。宜默默奉献、惠及他人,互相帮扶不绝"},
 {"革","顺天应人,治历明时","到了变革除旧的时候。顺天应人、果断革新,方焕然一新"},
 {"鼎","鼎新之道,正位凝命","除旧立新、稳定大局。摆正位置、稳住根基,各司其职"},
 {"震","洊雷之威,恐惧修省","突如其来的震动惊扰。临危不乱、心怀敬畏,借此自省安"},
 {"艮","时止则止,思不出位","该停就停、适可而止。安守本分、不越位妄动,静定生慧"},
 {"渐","渐进有序,居贤善俗","宜循序渐进、按部就班。一步一脚印,慢慢来反更稳久"},
 {"归妹","慎始虑终,征则有凶","行事须想清始终。一时冲动、贸然行事易招凶,谋定后动"},
 {"丰","盛大之时,宜照天下","盛大丰收、如日中天。把握当下、施惠四方,但盛极防衰"},
 {"旅","行旅在外,慎刑不留","如客居在外、漂泊不定。谨慎柔顺、安分守己,莫惹是非"},
 {"巽","申命行事,随风而入","如风顺势渗透。反复申明、循循渗入,以柔克刚方能行"},
 {"兑","丽泽相说,朋友讲习","和悦待人、喜气盈门。以诚相交、切磋共进,人和则事顺"},
 {"涣","风行水上,涣释群难","人心涣散、面临离析。宜凝聚人心、化解隔阂,重聚转危"},
 {"节","节以制度,当位以节","凡事贵在有节制。守住分寸、不过不欠,自律方行得通"},
 {"中孚","诚信中孚,信及豚鱼","内心诚信、表里如一。以诚感人,连难化者也能打动无阻"},
 {"小过","行过乎恭,宜下不宜上","可略偏谨慎恭谦。宜做小事、不宜大举,低调行事为佳"},
 {"既济","事已成,思患预防","大功已成、诸事齐备。守好成果、居安思危,谨防盛极生变"},
 {"未济","事未成,慎辨物居方","事情尚未成、正当过渡。别急着收尾,理清头绪各归位再推"}};
const char* meaningOf(const char* n){
  for(auto& g: MEAN) if(strcmp(g.n,n)==0) return g.m;
  return "观象玩辞,守正以待";
}
const char* baiOf(const char* n){     // 卦义释文
  for(auto& g: MEAN) if(strcmp(g.n,n)==0) return g.b;
  return "顺其自然,守正以待";
}
int guaNo(const char* n){             // 六十四卦序号(MEAN 按王弼序排列)
  for(int i=0;i<64;i++) if(strcmp(MEAN[i].n,n)==0) return i+1;
  return 0;
}

#include "zhouyi_lines.h"

// ---- 卦象状态 ----
bool  yang[6], moving[6];
int   yaoN = 0;
bool  coinFace[6][3];
uint8_t yaoValue[6];
char  benFull[24], bianFull[24], benName[8], bianName[8];
int   movePos[6], moveCnt = 0;
enum ResultSource { COIN_CAST, MANUAL_CAST, TRIGRAM_BROWSE };
ResultSource resultSource = COIN_CAST;
bool summaryBackHomeOnB = false;

struct CastSnapshot {
  bool valid = false;
  bool manual = false;
  uint8_t values[6]{};
  bool faces[6][3]{};
};
CastSnapshot lastResult;

void saveLastResult(){
  if(yaoN != 6 || resultSource == TRIGRAM_BROWSE) return;
  memcpy(lastResult.values,yaoValue,sizeof(yaoValue));
  memcpy(lastResult.faces,coinFace,sizeof(coinFace));
  lastResult.manual = resultSource == MANUAL_CAST;
  lastResult.valid = true;
}

int triIdx(bool b0,bool b1,bool b2){ return vIdx[(b0?1:0)+(b1?2:0)+(b2?4:0)]; }
void nameOf(const bool* L, char* full, char* nm){
  int lo=triIdx(L[0],L[1],L[2]), hi=triIdx(L[3],L[4],L[5]);
  strcpy(nm, NAMES[lo][hi]);
  if(lo==hi){ sprintf(full,"%s为%s", ETRI[lo], EELE[lo]); }
  else { sprintf(full,"%s%s%s", EELE[hi], EELE[lo], NAMES[lo][hi]); }
}
void computeResult(){
  bool chg[6];
  for(int i=0;i<6;i++) chg[i]= moving[i]? !yang[i] : yang[i];
  nameOf(yang, benFull, benName);
  nameOf(chg,  bianFull, bianName);
  moveCnt=0;
  for(int i=0;i<6;i++) if(moving[i]) movePos[moveCnt++]=i+1;
}

bool restoreLastResult(){
  if(!lastResult.valid) return false;
  resultSource = lastResult.manual ? MANUAL_CAST : COIN_CAST;
  memcpy(yaoValue,lastResult.values,sizeof(yaoValue));
  memcpy(coinFace,lastResult.faces,sizeof(coinFace));
  for(int i=0;i<6;i++){
    yang[i] = yaoValue[i] == 7 || yaoValue[i] == 9;
    moving[i] = yaoValue[i] == 6 || yaoValue[i] == 9;
  }
  yaoN = 6;
  summaryBackHomeOnB = false;
  computeResult();
  return true;
}

// ---- 页面状态机 ----
enum State { HOME, CAST, SUMMARY, DETAIL, MODE_SELECT, TRIGRAM_SELECT, MANUAL_ENTRY, MANUAL_REVIEW, CONFIRM_EXIT };
State state = HOME;
State exitBackState = HOME;
bool exitToMenu = false, exitConfirmed = false, discardSelected = false;
bool blockAnimationRelease = false;
int detailPage = 0;
const char* VER_CODE = "1.8.6";
const char* VER = "版｜壹.捌.陆";
bool simpleMode = false;
static constexpr uint32_t NAV_RELEASE_GUARD_MS = 180;
static constexpr uint32_t NAV_LONG_PRESS_MS = 800;
uint32_t navLockUntil = 0;
bool navBLongHandled = false;
int modeChoice = 0;
int upperTri = 0, lowerTri = 0;
bool selectingUpper = true;
static constexpr uint8_t TRIGRAM_BITS[8] = {7,3,5,1,6,2,4,0};
static constexpr uint8_t MANUAL_CHOICES[4] = {7,8,9,6};
uint8_t manualValues[6]{};
int manualPos = 0, manualChoice = 0;
bool manualReviewEditable = true;

// ---- 绘图小工具 ----
int tuneX(int x){ return x + ::LayoutLab::cfg.wx; }
int tuneY(int y){ return y + ::LayoutLab::cfg.wy; }
int tuneW(int w){ return constrain(w + ::LayoutLab::cfg.ww, 76, 122); }
int tuneBodyTop(int y){ return y + ::LayoutLab::cfg.wbtop; }
int tuneBodyBottom(int y){ return y + ::LayoutLab::cfg.wbbot; }
int tuneHeaderY(int y){ return y + ::LayoutLab::cfg.wh; }
void hRule(int x,int y,int w,uint16_t col){
  cv.drawFastHLine(tuneX(x),tuneY(y),w,col);
}
bool navLocked(){
  return (int32_t)(millis() - navLockUntil) < 0;
}
void armNavLock(){
  navLockUntil = millis() + NAV_RELEASE_GUARD_MS;
}
bool takeARelease(){
  if(navLocked()) return false;
  if(btnA().wasReleased()){
    armNavLock();
    return true;
  }
  return false;
}
bool takeBRelease(){
  if(navLocked()) return false;
  if(btnB().wasReleased()){
    armNavLock();
    return true;
  }
  return false;
}
void setF(int n){
  switch(n){
    case 10: cv.setFont(&::TextFonts::cn10); break;
    case 12: cv.setFont(&::TextFonts::cn12); break;
    case 14: cv.setFont(&::TextFonts::cn14); break;
    case 16: cv.setFont(&::TextFonts::cn16); break;
    default: cv.setFont(&::TextFonts::cn24); break;
  }
}
bool isRareGuaGlyph(const char* ch){
  return false;  // All title glyphs now come from the verified font chain.
}
const char* displayGuaTitleText(const char* s){ return s; }
void drawRareGuaGlyph(const char* ch,int x,int y,int n,uint16_t col){
  int s=n, top=y-s/2, th=max(2,n/8);
  if(strcmp(ch,"夬")==0){
    int x1=x+s/5, x2=x+s*4/5, mid=x+s/2;
    int y1=top+s/6, y2=top+s*7/16, y3=top+s*9/16, y4=top+s*5/6;
    cv.fillRect(x1,y1,x2-x1,th,col);
    cv.fillRect(mid-th/2,y1,th,y3-y1,col);
    cv.fillRect(x1,y2,x2-x1,th,col);
    cv.fillRect(mid-th/2,y2,th,max(th,s/5),col);
    for(int k=0;k<th;k++){
      cv.drawLine(mid-k/2,y3,x1+k,y4,col);
      cv.drawLine(mid+k/2,y3,x2-k,y4,col);
    }
    return;
  }
  // 姤:左「女」右「后」的简化笔画,避免缺字方块。
  cv.drawLine(x+s/5, top+s/5, x+s/12, top+s*3/4, col);
  cv.drawLine(x+s/12, top+s/2, x+s*2/5, top+s/3, col);
  cv.fillRect(x+s/12, top+s*2/3, s/3, th, col);
  cv.fillRect(x+s/2, top+s/5, s*2/5, th, col);
  cv.drawLine(x+s/2, top+s/5, x+s*5/12, top+s*5/6, col);
  cv.fillRect(x+s*13/24, top+s/2, s/3, th, col);
  cv.drawRect(x+s*13/24, top+s*5/8, s/3, s/5, col);
}
int mixedTextWidth(const char* s,int n){
  s = displayGuaTitleText(s);
  int total=0, len=strlen(s), i=0;
  while(i<len){
    int w=TextUtf8::charBytes(s+i);
    char ch[5]; memcpy(ch,s+i,w); ch[w]=0;
    if(isRareGuaGlyph(ch)) total += n;
    else { setF(n); total += cv.textWidth(ch); }
    i += w;
  }
  return total;
}
void tcGua(const char* s,int x,int y,int n,uint16_t col){
  s = displayGuaTitleText(s);
  x = tuneX(x);
  y = tuneY(y);
  int total=mixedTextWidth(s,n);
  int left=x-total/2;
  int len=strlen(s), i=0;
  cv.setTextDatum(middle_left);
  cv.setTextColor(col);
  while(i<len){
    int w=TextUtf8::charBytes(s+i);
    char ch[5]; memcpy(ch,s+i,w); ch[w]=0;
    if(isRareGuaGlyph(ch)){
      drawRareGuaGlyph(ch,left,y,n,col);
      left += n;
    } else {
      setF(n);
      cv.drawString(ch,left,y);
      left += cv.textWidth(ch);
    }
    i += w;
  }
}
const char* displaySafeText(const char* s);
void tc(const char* s,int x,int y,int n,uint16_t col){
  s = displaySafeText(s);
  setF(n); cv.setTextDatum(middle_center); cv.setTextColor(col);
  cv.drawString(s,tuneX(x),tuneY(y));
}
void drawMeaningLine(const char* text,int y){
  const int sizes[]={24,16,14,12};
  int chosen=12;
  for(int n:sizes){
    setF(n);
    if((int)cv.textWidth(text)<=tuneW(BODY_W)){ chosen=n; break; }
  }
  tc(text,W/2,y,chosen,C_INK);
}
void tl(const char* s,int x,int y,int n,uint16_t col){
  s = displaySafeText(s);
  setF(n); cv.setTextDatum(middle_left); cv.setTextColor(col);
  cv.drawString(s,tuneX(x),tuneY(y));
}
void fitCenter(const char* s,int x,int y,int maxW,uint16_t col){
  const int sizes[] = {16,14,12,10};
  int chosen = 10;
  for(int n: sizes){
    if(mixedTextWidth(s,n) <= tuneW(maxW)){ chosen = n; break; }
  }
  tcGua(s,x,y,chosen,col);
}
int castMovingDotCount(){
  int cnt=0;
  for(int i=0;i<yaoN && i<6;i++) if(moving[i]) cnt++;
  return cnt;
}
bool isOldYangLine(int idx){
  return idx>=0 && idx<6 && moving[idx] && yaoValue[idx]==9;
}
bool isOldYinLine(int idx){
  return idx>=0 && idx<6 && moving[idx] && yaoValue[idx]==6;
}
uint16_t moveDotFill(int idx){
  return isOldYangLine(idx) ? C_CIN : C_YIND;
}
uint16_t moveDotStroke(int idx){
  return isOldYangLine(idx) ? C_CIND : C_YINL;
}
const char* movingKindName(int idx){
  if(isOldYangLine(idx)) return "老阳";
  if(isOldYinLine(idx)) return "老阴";
  return "动爻";
}
void drawCastMovingDotsC(M5Canvas& g,int ox,int oy){
  int cnt=castMovingDotCount();
  if(cnt<=0) return;
  const int dotR=3, dotGap=8, y=178;
  int groupW = cnt*dotR*2 + (cnt-1)*dotGap;
  int x = tuneX(W/2) - groupW/2 + dotR;
  int yy = tuneY(y);
  for(int i=0;i<yaoN && i<6;i++){
    if(!moving[i]) continue;
    g.fillCircle(x-ox,yy-oy,dotR,moveDotFill(i));
    g.drawCircle(x-ox,yy-oy,dotR+1,moveDotStroke(i));
    x += dotR*2 + dotGap;
  }
}
void drawCastMovingDots(){
  drawCastMovingDotsC(cv,0,0);
}
void drawCastYaoLabel(const char* s){
  const int labelY = 162;
  const int sizes[] = {16,14,12,10};
  int maxW = 116;
  int chosen = 10;
  for(int n: sizes){
    if(mixedTextWidth(s,n) <= maxW){ chosen = n; break; }
  }
  int labelX = W/2;
  tcGua(s,labelX,labelY,chosen,C_INKL);
  drawCastMovingDots();
}
// 居中自动换行(按 UTF-8 字符,超宽折行)
// 自动换行 + 垂直居中(以 yCenter 为内容中心);标点不挂行首
void wrapCenter(const char* s,int yCenter,int n,int maxW,int lineH,uint16_t col){
  setF(n); cv.setTextDatum(middle_center); cv.setTextColor(col);
  char lines[10][100]; int nl=0, cl=0;
  int len=strlen(s), i=0;
  while(i<len && nl<9){
    int w=TextUtf8::charBytes(s+i);
    char ch[5]; memcpy(ch,s+i,w); ch[w]=0;
    bool punc = strstr("，。、；：！？,.!?",ch)!=NULL;
    char test[104]; memcpy(test,lines[nl],cl); memcpy(test+cl,ch,w); test[cl+w]=0;
    if((int)cv.textWidth(test)>tuneW(maxW) && cl>0 && !punc){
      lines[nl][cl]=0; nl++; cl=0;
      memcpy(lines[nl],ch,w); cl=w;
    } else { memcpy(lines[nl]+cl,ch,w); cl+=w; }
    i+=w;
  }
  lines[nl][cl]=0; nl++;
  int y=yCenter-(nl-1)*lineH/2;
  for(int k=0;k<nl;k++){ cv.drawString(lines[k],tuneX(W/2),tuneY(y)); y+=lineH; }
}
const char* displayFallbackGlyph(const char* ch){ return ch; }
const char* displaySafeText(const char* s){ return s; }
int wrapLines(const char* s,int n,int maxW,char lines[][96],int maxLines,bool* complete=nullptr){
  setF(n);
  auto width=[](const char* text){ return (int)cv.textWidth(text); };
  auto result=TextLayout::wrap(displaySafeText(s),maxW,lines,maxLines,width);
  if(complete) *complete=result.complete;
  if(!result.complete && result.count) TextLayout::ellipsize(lines[result.count-1],maxW,width);
  return result.count;
}

void drawFittedBody(const char* s,int yTop,int yBottom,int maxW,uint16_t col,
                    const int* sizes,const int* steps,int choices,bool shortLarge=false,
                    bool even=false,bool liftShort=false){
  yTop=tuneBodyTop(yTop);
  yBottom=tuneBodyBottom(yBottom);
  maxW=tuneW(maxW);
  char lines[10][96]{};
  int nl=0,n=sizes[choices-1],lineH=steps[choices-1];
  bool complete=false,fit=false;
  for(int i=0;i<choices;i++){
    n=sizes[i]; lineH=steps[i];
    nl=wrapLines(s,n,maxW,lines,10,&complete);
    if(!complete || (shortLarge && i==0 && nl>4)) continue;
    if(n+(nl-1)*lineH<=yBottom-yTop){ fit=true; break; }
  }
  if(!fit){
    int visible=max(1,min(10,1+(yBottom-yTop-n)/lineH));
    nl=wrapLines(s,n,maxW,lines,visible);
  }
  if(!nl) return;
  setF(n); cv.setTextDatum(middle_center); cv.setTextColor(col);
  int topC=yTop+n/2, bottomC=yBottom-n/2;
  if(even && nl>1) lineH=max(lineH,(bottomC-topC)/(nl-1));
  int y=topC+max(0,(bottomC-topC-(nl-1)*lineH)/2);
  if(liftShort && nl<=3) y=min(y,yTop+34);
  for(int k=0;k<nl;k++){
    cv.drawString(lines[k],tuneX(W/2),tuneY(y));
    y+=lineH;
  }
}
void drawWrappedBlock(const char* s,int yTop,int yBottom,int maxW,uint16_t col){
  const int sizes[]={16,14,12},steps[]={22,20,18};
  drawFittedBody(s,yTop,yBottom,maxW,col,sizes,steps,3,true,false,true);
}
void drawWrappedBlockCentered(const char* s,int yTop,int yBottom,int maxW,uint16_t col){
  const int sizes[]={16,14,12},steps[]={22,20,18};
  drawFittedBody(s,yTop,yBottom,maxW,col,sizes,steps,3,true);
}
void drawWrappedBlockEven(const char* s,int yTop,int yBottom,int maxW,uint16_t col){
  const int sizes[]={16,14,12,10},steps[]={20,18,16,14};
  drawFittedBody(s,yTop,yBottom,maxW,col,sizes,steps,4,false,true);
}
void drawWrappedBlockCompact(const char* s,int yTop,int yBottom,int maxW,uint16_t col){
  const int sizes[]={16,14,12,10},steps[]={18,16,15,13};
  drawFittedBody(s,yTop,yBottom,maxW,col,sizes,steps,4);
}
void drawYaoDetailBody(const char* s,int yTop,int yBottom,uint16_t col){
  const int sizes[]={16,14,12},steps[]={22,20,18};
  drawFittedBody(s,yTop,yBottom,96,col,sizes,steps,3);
}
void drawYaoClassicBody(const char* s,int yTop,int yBottom,uint16_t col){
  const int sizes[]={24,16,14,12},steps[]={29,21,19,17};
  drawFittedBody(s,yTop,yBottom,108,col,sizes,steps,4,true);
}
void drawWrappedTop(const char* s,int yTop,int n,int lineH,int maxW,int maxLines,uint16_t col){
  yTop=tuneBodyTop(yTop);
  maxW=tuneW(maxW);
  char lines[8][96]{};
  int nl=wrapLines(s,n,maxW,lines,min(maxLines,8));
  setF(n); cv.setTextDatum(middle_center); cv.setTextColor(col);
  int y=yTop+n/2;
  for(int k=0;k<nl;k++){
    cv.drawString(lines[k],tuneX(W/2),tuneY(y));
    y+=lineH;
  }
}
void sectionTitle(const char* s,int y){
  tc(s,W/2,y,12,C_CIN);
  hRule(30,y+11,W-60,C_INKL);
}
void sectionTitleBig(const char* s,int y){
  tc(s,W/2,y,14,C_CIN);
  hRule(28,y+13,W-56,C_INKL);
}
void moveYaoTitle(const char* s,int y){
  const int sizes[] = {24,16,14,12};
  const int dotR=3, dotGap=7;
  int chosen=12, textW=0;
  for(int n: sizes){
    setF(n);
    int w=cv.textWidth(s);
    if(w + dotR*2 + dotGap <= 116){ chosen=n; textW=w; break; }
  }
  if(textW==0){ setF(chosen); textW=cv.textWidth(s); }
  int groupW=textW + dotR*2 + dotGap;
  int left=(W-groupW)/2;
  int tx = tuneX(left);
  int ty = tuneY(y);
  cv.fillCircle(tx+dotR,ty,dotR,C_CIN);
  setF(chosen);
  cv.setTextDatum(middle_left);
  cv.setTextColor(C_CIN);
  cv.drawString(s,tx+dotR*2+dotGap,ty);
}
void drawMovingOrderDots(int moveIndex,int y){
  int dotCount = max(1,min(moveIndex+1,moveCnt));
  const int dotR=3, dotGap=7;
  int groupW = dotCount*dotR*2 + (dotCount-1)*dotGap;
  int x = tuneX(W/2) - groupW/2 + dotR;
  int yy = tuneY(y);
  for(int i=0;i<dotCount;i++){
    int idx = movePos[i]-1;
    cv.fillCircle(x,yy,dotR,moveDotFill(idx));
    cv.drawCircle(x,yy,dotR+1,moveDotStroke(idx));
    x += dotR*2 + dotGap;
  }
}
const char* cnNum64(int n);
const char* yaoPosName(int pos);
void drawCurrentMovingKindMark(int moveIndex,int y){
  int lineIdx = movePos[moveIndex]-1;
  const int dotR=3, dotGap=7;
  char text[28];
  snprintf(text,sizeof(text),"%s",movingKindName(lineIdx));
  setF(16);
  int textW = cv.textWidth(text);
  int groupW = dotR*2 + dotGap + textW;
  int left = tuneX(W/2) - groupW/2;
  int yy = tuneY(y);
  cv.fillCircle(left+dotR,yy,dotR,moveDotFill(lineIdx));
  cv.drawCircle(left+dotR,yy,dotR+1,moveDotStroke(lineIdx));
  cv.setTextDatum(middle_left);
  cv.setTextColor(moveDotFill(lineIdx));
  cv.drawString(text,left+dotR*2+dotGap,yy);
}
void drawClassicSubtitle(const char* yaoName,int moveIndex,int y){
  int linePos = movePos[moveIndex];
  int lineIdx = linePos - 1;
  char sub[64];
  snprintf(sub,sizeof(sub),"第%s卦｜%s %s｜%s",cnNum64(guaNo(benName)),yaoPosName(linePos),yaoName,movingKindName(lineIdx));
  fitCenter(sub,W/2,y,112,C_INKL);
}
void drawClassicSubtitleDot(const char* yaoName,int moveIndex,int y){
  int linePos = movePos[moveIndex];
  int lineIdx = linePos - 1;
  char guaLine[18], posLine[24], kindLine[12];
  snprintf(guaLine,sizeof(guaLine),"第%s卦",cnNum64(guaNo(benName)));
  snprintf(posLine,sizeof(posLine),"%s %s",yaoPosName(linePos),yaoName);
  snprintf(kindLine,sizeof(kindLine),"%s",movingKindName(lineIdx));
  const int dotR=3, dotGap=6;
  tc(guaLine,W/2,y,10,C_INKL);
  tc(posLine,W/2,y+17,12,C_INK);
  setF(12);
  int textW = cv.textWidth(kindLine);
  int groupW = dotR*2 + dotGap + textW;
  int left = tuneX(W/2) - groupW/2;
  int yy = tuneY(y+35);
  int dotX = left + dotR;
  cv.fillCircle(dotX,yy,dotR,moveDotFill(lineIdx));
  cv.drawCircle(dotX,yy,dotR+1,moveDotStroke(lineIdx));
  cv.setTextDatum(middle_left);
  cv.setTextColor(moveDotFill(lineIdx));
  cv.drawString(kindLine,left+dotR*2+dotGap,yy);
}
void drawYaoTextHeader(const char* yaoName,const char* label,int moveIndex,bool showGuaLine,bool showMovingMark=true,uint16_t subCol=C_INK){
  tc(label,W/2,tuneHeaderY(28),24,C_CIN);
  hRule(30,tuneHeaderY(44),W-60,C_INKL);
  int linePos = movePos[moveIndex];
  char posLine[28];
  snprintf(posLine,sizeof(posLine),"%s %s",yaoPosName(linePos),yaoName);
  if(showGuaLine){
    drawClassicSubtitle(yaoName,moveIndex,tuneHeaderY(62));
  } else {
    fitCenter(posLine,W/2,tuneHeaderY(showMovingMark ? 60 : 66),108,subCol);
    if(showMovingMark) drawCurrentMovingKindMark(moveIndex,tuneHeaderY(82));
  }
}
void drawBodyBottomRule(int yBottom){
  hRule(30,tuneBodyBottom(yBottom),W-60,C_INKL);
}
void drawBodyRules(int yTop,int yBottom){
  hRule(30,tuneBodyTop(yTop),W-60,C_INKL);
  hRule(30,tuneBodyBottom(yBottom),W-60,C_INKL);
}
void drawBodyTopRule(int yTop){
  hRule(30,tuneBodyTop(yTop),W-60,C_INKL);
}
const char* skipLeadMark(const char* p){
  while(*p==' ') p++;
  if(strncmp(p,"。",3)==0 || strncmp(p,"：",3)==0 || strncmp(p,"、",3)==0) return p+3;
  if(*p=='.' || *p==':' || *p==';' || *p==',') return p+1;
  return p;
}
const char* stripTextPrefix(const char* text,const char* prefix){
  if(!text || !text[0]) return "";
  if(prefix && prefix[0]){
    int n=strlen(prefix);
    if(strncmp(text,prefix,n)==0) return skipLeadMark(text+n);
  }
  return text;
}
const char* stripXiangPrefix(const char* text){
  if(!text || !text[0]) return "";
  return stripTextPrefix(text,"象曰");
}
void tr(const char* s,int x,int y,int n,uint16_t col){
  s = displaySafeText(s);
  setF(n); cv.setTextDatum(middle_right); cv.setTextColor(col);
  cv.drawString(s,tuneX(x),tuneY(y));
}
void paperBg(){
  cv.fillScreen(C_PAPER);
  randomSeed(7);
  for(int i=0;i<240;i++) cv.drawPixel(random(W),random(H), lgfx::color565(214,202,174));
  for(int i=0;i<36;i++)  cv.drawPixel(random(W),random(H), lgfx::color565(198,184,154));
}
void frame(bool fishtail){
  cv.drawRect(6,6,W-12,H-12,C_INK);
  cv.drawRect(7,7,W-14,H-14,C_INK);
  cv.drawRect(10,10,W-20,H-20,C_INKL);
  (void)fishtail;   // 顶部鱼尾装饰已去掉
}
void dashedH(int x0,int x1,int y,uint16_t col){
  for(int x=x0;x<x1;x+=11){ int e=min(x+6,x1); hRule(x,y-1,e-x,col); hRule(x,y,e-x,col);}
}
void smallSeal(int cx,int cy,int sz,const char* ch){
  cv.fillRect(cx-sz/2,cy-sz/2,sz,sz,C_CIN);
  setF(14);
  cv.setTextDatum(middle_center);
  cv.setTextColor(C_PAPER);
  cv.drawString(ch,cx,cy);
}

// ---- 古铜钱(可画到任意画布) ----
void drawCoinC(M5Canvas& g,int cx,int cy,int r,bool face,float sx){
  // 正面=金黄+钱文; 背面=暗橄榄铜绿+光板。整枚颜色不同,一眼分正反
  uint16_t body = face? C_BM   : C_BACKM;
  uint16_t hi   = face? C_BL   : C_BACKH;
  uint16_t dk   = face? C_BD   : C_BACKD;
  if(sx<0.14f){ g.fillRect(cx-1,cy-r,3,2*r,dk); g.fillRect(cx,cy-r+1,1,2*r-2,body); return; }
  int rx=max(1,(int)(r*sx));
  g.fillEllipse(cx,cy,rx,r,dk);
  g.fillEllipse(cx,cy,max(1,rx-1),r-1,body);
  g.fillEllipse(cx-(int)(rx*0.3f),cy-(int)(r*0.3f),max(1,(int)(rx*0.5f)),max(1,(int)(r*0.5f)),hi);
  g.drawEllipse(cx,cy,rx,r,dk);
  int hs=(int)(r*0.34f), hx=max(1,(int)(hs*sx));
  g.fillRect(cx-hx,cy-hs,2*hx,2*hs,C_PAPER);
  g.drawRect(cx-hx,cy-hs,2*hx,2*hs,dk);
  if(face){                 // 正面:方孔四边粗钱文(有字)
    int d=(int)(r*0.58f), dxx=(int)(d*sx);
    g.fillRect(cx-1, cy-d-2, 3, 6, C_BD);     // 上 竖
    g.fillRect(cx-1, cy+d-3, 3, 6, C_BD);     // 下 竖
    g.fillRect(cx-dxx-2, cy-1, 6, 3, C_BD);   // 左 横
    g.fillRect(cx+dxx-3, cy-1, 6, 3, C_BD);   // 右 横
  } else {                  // 背面:光板 + 铜绿斑(无字)
    g.drawPixel(cx-(int)(rx*0.4f), cy-(int)(r*0.4f), C_PAT);
    g.drawPixel(cx+(int)(rx*0.35f),cy+(int)(r*0.3f), C_PAT);
    g.drawPixel(cx-(int)(rx*0.1f), cy+(int)(r*0.5f), C_PAT);
  }
}
inline void drawCoin(int cx,int cy,int r,bool face,float sx){ drawCoinC(cv,cx,cy,r,face,sx); }
// 三枚铜钱中心 x(不重叠) 与半径
static const int COINX[3] = {29, 67, 105};
static const int COINR = 14, COINY = 200;
void drawCoinSlot(int cx,int cy,int r){
  cv.fillEllipse(cx,cy,r,r,C_PAPER2);
  cv.drawEllipse(cx,cy,r,r,C_INKL);
  cv.drawEllipse(cx,cy,r-2,r-2,C_INKL);
  int hs=(int)(r*0.34f);
  cv.drawRect(cx-hs,cy-hs,hs*2,hs*2,C_INKL);
}

// ---- 六爻(方头粗横线,似刻本卦牌) ----
void drawYao(int y,int idx){
  int bw=100,x=tuneX((W-bw)/2),th=14;
  y = tuneY(y);
  if(idx>=yaoN){ // 未起:淡墨虚线占位
    for(int xx=x; xx<x+bw-4; xx+=14) cv.fillRect(xx,y-1,8,2,C_INKL);
    return;
  }
  if(yang[idx]){                       // 阳爻:一整条朱砂实线
    cv.fillRect(x,y-th/2,bw,th,C_CIN);
  } else {                             // 阴爻:两段墨色断线,中缝约 1/5
    int gap=18, seg=(bw-gap)/2;
    cv.fillRect(x,y-th/2,seg,th,C_INK);
    cv.fillRect(x+seg+gap,y-th/2,seg,th,C_INK);
  }
  // 起卦过程只在底部当前爻位置提示动爻,六爻本体保持纯净。
}

void drawMiniGua(int x,int y){
  const int bw=38, th=5, gap=11;
  x = tuneX(x);
  y = tuneY(y);
  for(int idx=5; idx>=0; --idx){
    int yy = y + (5-idx)*gap;
    if(yang[idx]){
      cv.fillRect(x,yy-th/2,bw,th,C_CIN);
    } else {
      int mid=8, seg=(bw-mid)/2;
      cv.fillRect(x,yy-th/2,seg,th,C_INK);
      cv.fillRect(x+seg+mid,yy-th/2,seg,th,C_INK);
    }
    if(moving[idx]){
      cv.fillCircle(x+bw+7,yy,3,moveDotFill(idx));
      cv.drawCircle(x+bw+7,yy,4,moveDotStroke(idx));
    }
  }
}

void drawLargeMoveGua(){
  const int bw=78, th=8, gap=19, dotGap=8, dotR=3, dotOuter=4;
  const int visualW = moveCnt > 0 ? bw + dotGap + dotOuter : bw;
  const int x=tuneX((W-visualW)/2);
  const int y=tuneY(70 + ::LayoutLab::cfg.wflow);
  for(int idx=5; idx>=0; --idx){
    int yy = y + (5-idx)*gap;
    if(yang[idx]){
      cv.fillRect(x,yy-th/2,bw,th,C_CIN);
    } else {
      int mid=18, seg=(bw-mid)/2;
      cv.fillRect(x,yy-th/2,seg,th,C_INK);
      cv.fillRect(x+seg+mid,yy-th/2,seg,th,C_INK);
    }
    if(moving[idx]){
      int cx=x+bw+dotGap;
      cv.fillCircle(cx,yy,dotR,moveDotFill(idx));
      cv.drawCircle(cx,yy,dotOuter,moveDotStroke(idx));
    }
  }
}

const char* posFallbackTitle(int pos){
  static const char* t[6] = {"初爻","二爻","三爻","四爻","五爻","上爻"};
  if(pos<1 || pos>6) return "动爻";
  return t[pos-1];
}
void yaoTitle(char* buf,int cap,int pos,bool isYang){
  if(pos==1) snprintf(buf,cap,isYang ? "初九" : "初六");
  else if(pos==6) snprintf(buf,cap,isYang ? "上九" : "上六");
  else {
    static const char* yangNames[4] = {"九二","九三","九四","九五"};
    static const char* yinNames[4]  = {"六二","六三","六四","六五"};
    int idx=pos-2;
    snprintf(buf,cap,"%s",(idx>=0 && idx<4) ? (isYang ? yangNames[idx] : yinNames[idx]) : "爻");
  }
}
const char* yaoPosName(int pos){
  static const char* names[6] = {"初爻","二爻","三爻","四爻","五爻","上爻"};
  if(pos<1 || pos>6) return "爻";
  return names[pos-1];
}
void yaoDisplayLabel(char* buf,int cap,int pos,bool isYang,bool isMoving){
  char title[12]; yaoTitle(title,sizeof(title),pos,isYang);
  snprintf(buf,cap,"%s：%s%s",yaoPosName(pos),title,isMoving ? "（动爻）" : "");
}
void currentYaoLabel(char* buf,int cap){
  if(yaoN<=0){
    snprintf(buf,cap,"待起初爻");
    return;
  }
  int pos = min(yaoN,6);
  yaoDisplayLabel(buf,cap,pos,yang[pos-1],moving[pos-1]);
}
void currentCastYaoLabel(char* buf,int cap){
  if(yaoN<=0){
    snprintf(buf,cap,"待起初爻");
    return;
  }
  int pos = min(yaoN,6);
  char title[12]; yaoTitle(title,sizeof(title),pos,yang[pos-1]);
  snprintf(buf,cap,"%s：%s",yaoPosName(pos),title);
}
const char* posStage(int pos){
  static const char* t[6] = {"初爻","二爻","三爻","四爻","五爻","上爻"};
  if(pos<1 || pos>6) return "爻位";
  return t[pos-1];
}
const char* posAdvice(int pos){
  static const char* t[6] = {
    "在最底下,代表刚开始,先稳住。",
    "代表内部关系,要找对人、守正。",
    "代表风险位,别急着硬冲。",
    "代表要往外走,先看清局面。",
    "代表主位,可以担当,但要走正道。",
    "在最上面,代表收尾,别做过头。"
  };
  if(pos<1 || pos>6) return "观其所处,守正应变。";
  return t[pos-1];
}
const char* movingNature(int value){
  if(value == 9) return "老阳变阴。";
  if(value == 6) return "老阴变阳。";
  return "动爻主变。";
}
const char* posIdea(int pos){
  static const char* t[6] = {
    "看起点是否稳。",
    "看关系是否得中。",
    "看风险是否过界。",
    "看进退是否合时。",
    "看担当是否中正。",
    "看收尾是否知止。"
  };
  if(pos<1 || pos>6) return "看变化是否合时。";
  return t[pos-1];
}
const char* posBrief(int pos){
  static const char* t[6] = {
    "初始宜稳",
    "内应守正",
    "临险慎进",
    "观势再动",
    "主位中正",
    "终局知止"
  };
  if(pos<1 || pos>6) return "守正应变";
  return t[pos-1];
}
bool hasUseLineCast();
int useLinePageCount();
int detailPageCount(){
  if(simpleMode) return resultSource!=TRIGRAM_BROWSE && moveCnt>0 ? 3 : 1;
  if(resultSource == TRIGRAM_BROWSE) return 16;
  return moveCnt > 0 ? (4 + moveCnt * 4 + useLinePageCount()) : 2;
}
bool hasUseLineCast(){
  if(resultSource == TRIGRAM_BROWSE) return false;
  if(moveCnt != 6) return false;
  if(strcmp(benName,"乾") == 0){
    for(int i=0;i<6;i++) if(yaoValue[i] != 9) return false;
    return true;
  }
  if(strcmp(benName,"坤") == 0){
    for(int i=0;i<6;i++) if(yaoValue[i] != 6) return false;
    return true;
  }
  return false;
}
int useLinePageCount(){
  return hasUseLineCast() ? 2 : 0;
}
void singleChangedFull(int linePos,char* full,char* name){
  bool one[6];
  for(int i=0;i<6;i++) one[i]=yang[i];
  if(linePos>=1 && linePos<=6) one[linePos-1]=!one[linePos-1];
  nameOf(one, full, name);
}
void buildMoveSummary(char* buf,int cap){
  if(moveCnt<=0){
    snprintf(buf,cap,"无动爻");
    return;
  }
  snprintf(buf,cap,"动爻 ");
  int used=strlen(buf);
  for(int i=0;i<moveCnt;i++){
    const YaoLineText* yt=yaoTextOf(benName,movePos[i]);
    const char* title=(yt && yt->title && yt->title[0]) ? yt->title : posFallbackTitle(movePos[i]);
    used += snprintf(buf+used,max(0,cap-used),"%s%s",i?" ":"",title);
    if(used>=cap) break;
  }
}
void buildMoveChangeLine(int moveIndex,char* buf,int cap){
  if(moveIndex<0 || moveIndex>=moveCnt){
    snprintf(buf,cap,"动爻");
    return;
  }
  int pos=movePos[moveIndex];
  char label[28]; yaoDisplayLabel(label,sizeof(label),pos,yang[pos-1],false);
  snprintf(buf,cap,"%s",label);
}

const char* moveCountTitle(int cnt){
  static const char* titles[7] = {
    "六爻皆静","一爻动","两爻动","三爻动","四爻动","五爻动","六爻动"
  };
  if(cnt<0) cnt=0;
  if(cnt>6) cnt=6;
  return titles[cnt];
}
const char* cnNum64(int n){
  static char buf[16];
  static const char* ones[]={"","一","二","三","四","五","六","七","八","九"};
  if(n<=0){ snprintf(buf,sizeof(buf),""); return buf; }
  if(n<10){ snprintf(buf,sizeof(buf),"%s",ones[n]); return buf; }
  if(n==10){ snprintf(buf,sizeof(buf),"十"); return buf; }
  if(n<20){ snprintf(buf,sizeof(buf),"十%s",ones[n%10]); return buf; }
  int tens=n/10, one=n%10;
  snprintf(buf,sizeof(buf),"%s十%s",ones[tens],ones[one]);
  return buf;
}

// ---- 页面 ----
const char* lineKindName(uint8_t value){
  switch(value){
    case 6: return "老阴";
    case 7: return "少阳";
    case 8: return "少阴";
    case 9: return "老阳";
    default: return "未选";
  }
}

void drawModeSelect(){
  paperBg(); frame(false);
  tc("问爻",W/2,30,24,C_INK);
  static const char* modes[] = {"摇卦","选卦","自定六爻","上一卦"};
  for(int i=0;i<4;i++){
    int y=51+i*34;
    if(i==modeChoice) cv.drawRect(18,y,99,30,C_CIN);
    tc(i==3 && !lastResult.valid ? "暂无上一卦" : modes[i],W/2,y+15,16,
       i==3 && !lastResult.valid ? C_INKL : i==modeChoice ? C_CIN : C_INK);
  }
  tc("B切换 A选定",W/2,201,12,C_INK);
  tc("长B返回",W/2,220,12,C_INKL);
  cv.pushSprite(0,0);
}

void drawInputLine(int x,int y,int width,uint8_t value){
  const int th=6, gap=10;
  if(value<6 || value>9){
    for(int xx=x;xx<x+width;xx+=10) cv.fillRect(xx,y-1,6,2,C_INKL);
  } else if(value==7 || value==9){
    cv.fillRect(x,y-th/2,width,th,C_CIN);
  } else {
    int segment=(width-gap)/2;
    cv.fillRect(x,y-th/2,segment,th,C_INK);
    cv.fillRect(x+segment+gap,y-th/2,segment,th,C_INK);
  }
}

void drawTrigramSelect(){
  paperBg(); frame(false);
  tc(selectingUpper ? "选择上卦" : "选择下卦",W/2,28,16,C_CIN);
  char text[32];
  for(int part=0;part<2;part++){
    int tri=part==0 ? upperTri : lowerTri;
    bool selected=part==0 ? selectingUpper : !selectingUpper;
    int y=61+part*34;
    if(selected) cv.drawRect(18,y-14,99,28,C_CIN);
    snprintf(text,sizeof(text),"%s：%s%s",part==0 ? "上卦" : "下卦",ETRI[tri],EELE[tri]);
    tc(text,W/2,y,14,selected ? C_CIN : C_INK);
  }
  bool preview[6];
  for(int i=0;i<6;i++){
    preview[i]=(TRIGRAM_BITS[i<3 ? lowerTri : upperTri] & (1 << (i%3)))!=0;
    drawInputLine(43,118+(5-i)*10,48,preview[i] ? 7 : 8);
  }
  char full[24], name[8]; nameOf(preview,full,name);
  fitCenter(full,W/2,188,104,C_INK);
  tc("B切换 A选定",W/2,206,12,C_INKL);
  tc("长B退一步",W/2,221,12,C_INKL);
  cv.pushSprite(0,0);
}

void drawManualDiagram(bool editing){
  for(int i=5;i>=0;i--){
    int y=60+(5-i)*20;
    uint8_t value=editing && i==manualPos ? MANUAL_CHOICES[manualChoice] : manualValues[i];
    if(editing && i==manualPos) cv.drawRect(19,y-9,97,19,C_CIN);
    drawInputLine(27,y,42,value);
    tl(lineKindName(value),78,y,14,C_INK);
  }
}

void drawManualEntry(){
  paperBg(); frame(false);
  tc(yaoPosName(manualPos+1),W/2,28,24,C_CIN);
  drawManualDiagram(true);
  static const char* states[] = {"阳爻不变","阴爻不变","阳爻变阴","阴爻变阳"};
  tc(states[manualChoice],W/2,185,14,C_INK);
  tc("B切换 A选定",W/2,203,12,C_INK);
  tc("长B退一步",W/2,221,12,C_INKL);
  cv.pushSprite(0,0);
}

void drawManualReview(){
  paperBg(); frame(false);
  tc("六爻核对",W/2,28,16,C_CIN);
  drawManualDiagram(false);
  tc(manualReviewEditable ? "看看组合结果" : "本次六爻",W/2,189,14,C_INK);
  if(manualReviewEditable) tc("A查看 B修改",W/2,219,12,C_INKL);
  cv.pushSprite(0,0);
}

void drawHome(){
  paperBg(); frame(false);
  // 题签框:内部只放竖排书名「问爻」
  int sw=46,sx=W/2-sw/2,sy=24,sh=80;
  cv.fillRect(sx,sy,sw,sh,C_PAPER2);
  cv.drawRect(sx,sy,sw,sh,C_INK);
  cv.drawRect(sx+3,sy+3,sw-6,sh-6,C_INKL);
  tc("问",W/2,sy+26,24,C_INK);
  tc("爻",W/2,sy+56,24,C_INK);
  // 朱印单独放题签框外下方(书画落款钤印式)
  smallSeal(W/2,sy+sh+24,20,"易");
  // 使用说明
  tc(simpleMode ? "简单模式" : "专业模式",W/2,157,14,C_CIN);
  tc("A进入 B返回",W/2,178,14,C_INK);
  tc("长按B切换",W/2,199,12,C_INK);
  tc(VER,W/2,222,14,C_INKL);
  cv.pushSprite(0,0);
}

void drawCastStatic(){
  paperBg(); frame(false);
  int topY=22,rowH=23;
  for(int i=5;i>=0;i--) drawYao(topY+(5-i)*rowH, i);
  char ybuf[32]; currentCastYaoLabel(ybuf,sizeof(ybuf));
  drawCastYaoLabel(ybuf);
  if(yaoN>0){
    int idx=yaoN-1;
    drawCoin(COINX[0],COINY,COINR,coinFace[idx][0],1);
    drawCoin(COINX[1],COINY,COINR,coinFace[idx][1],1);
    drawCoin(COINX[2],COINY,COINR,coinFace[idx][2],1);
  } else {
    for(int i=0;i<3;i++) drawCoinSlot(COINX[i],COINY,COINR);
  }
  cv.pushSprite(0,0);
}
void drawCast(){ drawCastStatic(); }

void drawSummary(){          // 正统流程第一步:当前局面/本卦
  paperBg(); frame(true);
  tc(resultSource == TRIGRAM_BROWSE ? "选卦成象" : "本卦成象",W/2,28,14,C_CIN);
  tcGua(benFull,W/2,62,24,C_INK);               // 大字卦名
  hRule(24,94,W-48,C_INKL);
  const char* m=meaningOf(benName);             // 八字解释,逗号处分两行
  const char* comma=strchr(m,','); int cl=1;
  if(!comma){ comma=strstr(m,"，"); cl=3; }
  if(comma){
    int n=comma-m; char a[28],b[28];
    memcpy(a,m,n); a[n]=0; strcpy(b,comma+cl);
    drawMeaningLine(a,132);
    drawMeaningLine(b,164);
  } else {
    drawMeaningLine(m,148);
  }
  cv.pushSprite(0,0);
}

void drawGuaCiDetail(){      // 正统流程第二步:先占本卦卦辞
  paperBg(); frame(false);
  tc("卦辞象曰",W/2,28,18,C_CIN);
  fitCenter(benFull,W/2,50,108,C_INKL);
  const GuaText* gt=guaTextOf(benName);
  sectionTitle("卦辞",72);
  drawWrappedTop((gt && gt->ci && gt->ci[0]) ? gt->ci : meaningOf(benName),88,12,16,BODY_W,4,C_INK);
  sectionTitle("象曰",154);
  drawWrappedTop((gt && gt->xiang && gt->xiang[0]) ? gt->xiang : baiOf(benName),170,10,14,BODY_W,4,C_INK);
  cv.pushSprite(0,0);
}

void drawMoveFlowDetail(){   // 客户定制流程:只列动爻
  paperBg(); frame(false);
  tc(moveCountTitle(moveCnt),W/2,28,18,C_CIN);
  hRule(28,50,W-56,C_INKL);
  drawLargeMoveGua();
  if(moveCnt<=0){
    fitCenter("静占本卦",W/2,196,110,C_INKL);
    fitCenter(benFull,W/2,216,110,C_CIN);
    cv.pushSprite(0,0);
    return;
  }
  char ben[32]; snprintf(ben,sizeof(ben),"本卦：%s",benFull);
  char zhi[32]; snprintf(zhi,sizeof(zhi),"之卦：%s",bianFull);
  fitCenter(ben,W/2,192,112,C_INKL);
  fitCenter(zhi,W/2,216,112,C_CIN);
  cv.pushSprite(0,0);
}

void drawZhiGuaResultDetail(){     // 客户定制流程最后:先看之卦成象
  paperBg(); frame(true);
  tc("之卦成象",W/2,28,14,C_CIN);
  tcGua(bianFull,W/2,62,24,C_INK);
  hRule(24,94,W-48,C_INKL);
  const char* m=meaningOf(bianName);
  const char* comma=strchr(m,','); int cl=1;
  if(!comma){ comma=strstr(m,"，"); cl=3; }
  if(comma){
    int n=comma-m; char a[28],b[28];
    memcpy(a,m,n); a[n]=0; strcpy(b,comma+cl);
    drawMeaningLine(a,132);
    drawMeaningLine(b,164);
  } else {
    drawMeaningLine(m,148);
  }
  cv.pushSprite(0,0);
}

void drawZhiGuaBodyDetail(){       // 之卦正文单独成页
  paperBg(); frame(false);
  tc("之卦卦义",W/2,28,14,C_CIN);
  tcGua(bianFull,W/2,62,24,C_INK);
  hRule(24,94,W-48,C_INKL);
  drawWrappedBlockCentered(baiOf(bianName),100,206,BODY_W,C_INK);
  cv.pushSprite(0,0);
}

void drawBaseDetail(){       // 本卦卦义
  paperBg(); frame(false);
  tc("本卦卦义",W/2,28,14,C_CIN);
  tcGua(benFull,W/2,62,24,C_INK);
  hRule(24,94,W-48,C_INKL);
  drawWrappedBlockCentered(baiOf(benName), 100, 206, BODY_W, C_INK);
  cv.pushSprite(0,0);
}

void drawYaoCiDetail(int moveIndex){
  paperBg(); frame(false);
  int linePos=movePos[moveIndex];
  const YaoLineText* yt=yaoTextOf(benName,linePos);
  char yaoName[12]; yaoTitle(yaoName,sizeof(yaoName),linePos,yang[linePos-1]);
  drawClassicSubtitleDot(yaoName,moveIndex,tuneHeaderY(20));
  tc("爻辞",W/2,tuneBodyTop(84),16,C_CIN);
  hRule(34,tuneBodyTop(100),W-68,C_INKL);
  drawWrappedBlockCompact(yt?stripTextPrefix(yt->ci,yt->title):"爻辞暂缺",110,218,100,C_INK);
  cv.pushSprite(0,0);
}

void drawYaoXiangDetail(int moveIndex){
  paperBg(); frame(false);
  int linePos=movePos[moveIndex];
  const YaoLineText* yt=yaoTextOf(benName,linePos);
  char yaoName[12]; yaoTitle(yaoName,sizeof(yaoName),linePos,yang[linePos-1]);
  drawClassicSubtitleDot(yaoName,moveIndex,tuneHeaderY(20));
  tc("象曰",W/2,tuneBodyTop(84),16,C_CIN);
  hRule(34,tuneBodyTop(100),W-68,C_INKL);
  drawWrappedBlockCompact(yt?stripXiangPrefix(yt->xiang):"象辞暂缺",110,218,100,C_INK);
  cv.pushSprite(0,0);
}

void drawUseLineDetail(bool showXiang){
  paperBg(); frame(false);
  const YaoLineText* yt=useLineTextOf(benName);
  const char* title=(yt && yt->title && yt->title[0]) ? yt->title : "用爻";
  fitCenter(benFull,W/2,tuneHeaderY(22),112,C_INKL);
  tc(title,W/2,tuneHeaderY(48),20,C_CIN);
  hRule(34,tuneHeaderY(65),W-68,C_INKL);
  tc(showXiang ? "象曰" : "爻辞",W/2,tuneBodyTop(88),16,C_CIN);
  hRule(34,tuneBodyTop(104),W-68,C_INKL);
  const char* body = showXiang
    ? (yt ? stripXiangPrefix(yt->xiang) : "象辞暂缺")
    : (yt ? stripTextPrefix(yt->ci,yt->title) : "爻辞暂缺");
  drawWrappedBlockCompact(body,114,218,100,C_INK);
  cv.pushSprite(0,0);
}

void drawYaoPlainDetail(int moveIndex){
  paperBg(); frame(false);
  int linePos=movePos[moveIndex];
  char yaoName[12]; yaoTitle(yaoName,sizeof(yaoName),linePos,yang[linePos-1]);
  drawYaoTextHeader(yaoName,"爻解",moveIndex,false,false,C_CIN);
  char plain[150];
  snprintf(plain,sizeof(plain),"%s",posAdvice(linePos));
  drawYaoDetailBody(plain,90,216,C_INK);
  cv.pushSprite(0,0);
}

void drawYaoPhilosophyDetail(int moveIndex){
  paperBg(); frame(false);
  int linePos=movePos[moveIndex];
  char yaoName[12]; yaoTitle(yaoName,sizeof(yaoName),linePos,yang[linePos-1]);
  drawYaoTextHeader(yaoName,"爻义",moveIndex,false,false,C_CIN);
  char idea[170];
  snprintf(idea,sizeof(idea),"%s %s，%s",movingNature(yaoValue[linePos-1]),meaningOf(benName),posIdea(linePos));
  drawYaoDetailBody(idea,90,216,C_INK);
  cv.pushSprite(0,0);
}

void drawBrowseGuaTextDetail(bool showXiang){
  paperBg(); frame(false);
  tc(showXiang ? "象曰" : "卦辞",W/2,28,24,C_CIN);
  fitCenter(benFull,W/2,62,104,C_INK);
  hRule(30,84,W-60,C_INKL);
  const GuaText* gt=guaTextOf(benName);
  const char* body=gt ? (showXiang ? stripXiangPrefix(gt->xiang) : gt->ci) : "经文暂缺";
  drawWrappedBlockCompact(body,94,218,100,C_INK);
  cv.pushSprite(0,0);
}

void drawBrowseLines(){
  paperBg(); frame(false);
  tc("六爻",W/2,28,24,C_CIN);
  hRule(28,50,W-56,C_INKL);
  drawLargeMoveGua();
  fitCenter(benFull,W/2,194,104,C_INK);
  char parts[32];
  int lo=triIdx(yang[0],yang[1],yang[2]), hi=triIdx(yang[3],yang[4],yang[5]);
  snprintf(parts,sizeof(parts),"上%s%s 下%s%s",ETRI[hi],EELE[hi],ETRI[lo],EELE[lo]);
  tc(parts,W/2,216,12,C_INKL);
  cv.pushSprite(0,0);
}

void drawBrowseClassicDetail(int linePos,bool showXiang){
  paperBg(); frame(false);
  char guaLine[24], posLine[28], yaoName[12];
  yaoTitle(yaoName,sizeof(yaoName),linePos,yang[linePos-1]);
  snprintf(guaLine,sizeof(guaLine),"第%s卦",cnNum64(guaNo(benName)));
  snprintf(posLine,sizeof(posLine),"%s %s",yaoPosName(linePos),yaoName);
  tc(guaLine,W/2,20,10,C_INKL);
  tc(posLine,W/2,42,16,C_INK);
  tc(showXiang ? "象曰" : "爻辞",W/2,76,24,C_CIN);
  hRule(34,94,W-68,C_INKL);
  const YaoLineText* yt=yaoTextOf(benName,linePos);
  const char* body=yt ? (showXiang ? stripXiangPrefix(yt->xiang) : stripTextPrefix(yt->ci,yt->title)) : "经文暂缺";
  drawWrappedBlockCompact(body,104,218,100,C_INK);
  cv.pushSprite(0,0);
}

void drawDetail(){
  if(simpleMode){
    if(detailPage==0) drawBaseDetail();
    else if(detailPage==1 && resultSource!=TRIGRAM_BROWSE && moveCnt>0) drawZhiGuaResultDetail();
    else if(detailPage==2 && resultSource!=TRIGRAM_BROWSE && moveCnt>0) drawZhiGuaBodyDetail();
    else { detailPage=0; drawBaseDetail(); }
    return;
  }
  if(resultSource==TRIGRAM_BROWSE){
    if(detailPage==0) drawBaseDetail();
    else if(detailPage==1 || detailPage==2) drawBrowseGuaTextDetail(detailPage==2);
    else if(detailPage==3) drawBrowseLines();
    else drawBrowseClassicDetail((detailPage-4)/2+1,(detailPage-4)%2==1);
    return;
  }
  if(detailPage<=0){ drawBaseDetail(); return; }
  if(detailPage==1){ drawMoveFlowDetail(); return; }
  if(moveCnt<=0){ drawMoveFlowDetail(); return; }
  int usePage = 2 + moveCnt * 4;
  if(useLinePageCount() > 0){
    if(detailPage == usePage){ drawUseLineDetail(false); return; }
    if(detailPage == usePage + 1){ drawUseLineDetail(true); return; }
  }
  int zhiPage = 2 + moveCnt * 4 + useLinePageCount();
  if(detailPage == zhiPage){ drawZhiGuaResultDetail(); return; }
  if(detailPage == zhiPage + 1){ drawZhiGuaBodyDetail(); return; }
  int idx=(detailPage-2)/4;
  int part=(detailPage-2)%4;
  if(idx<0 || idx>=moveCnt){ drawZhiGuaBodyDetail(); return; }
  if(part==0) drawYaoCiDetail(idx);
  else if(part==1) drawYaoXiangDetail(idx);
  else if(part==2) drawYaoPlainDetail(idx);
  else drawYaoPhilosophyDetail(idx);
}

// ---- 投掷动画:仅局部刷新铜钱条,慢翻顺滑 ----
void tossAnimate(bool t0,bool t1,bool t2){
  static M5Canvas band(&M5.Display);
  static bool inited=false;
  const int BX=12, BY=174, BW=111, BH=52;       // 铜钱条(避开版框竖线)
  if(!inited){ band.setColorDepth(16); band.createSprite(BW,BH); inited=true; }
  const int N=3, dur=460, off[3]={0,70,140};     // 翻 1.5 圈,错开起转,总约 0.6s
  bool tg[3]={t0,t1,t2};
  bool start[3]={ (N%2)? !t0:t0, (N%2)? !t1:t1, (N%2)? !t2:t2 };
  const int rx[3]={ COINX[0]-BX, COINX[1]-BX, COINX[2]-BX }, cyr=COINY-BY;
  uint32_t t=millis();
  for(;;){
    M5.update();
    bool skip = btnA().wasPressed() || btnB().wasPressed();  // 按键立即落定
    bool spinning=false;
    band.fillScreen(C_PAPER);
    for(int i=0;i<3;i++){
      float p=(float)((long)millis()-(long)t-off[i])/dur;
      if(skip) p=1;
      if(p<0)p=0; if(p>1)p=1; else spinning=true;
      float e = p<0.5f? 2*p*p : 1-powf(-2*p+2,2)/2;  // easeInOut
      float ang=e*N*PI, sx=fabsf(cosf(ang));
      int half=(int)(ang/PI+1e-6f);
      bool f=(half%2==0)? start[i] : !start[i];
      if(p>=1){ sx=1; f=tg[i]; }
      drawCoinC(band, rx[i], cyr, COINR, f, sx);
    }
    drawCastMovingDotsC(band,BX,BY);
    band.pushSprite(BX,BY);
    if(!spinning || skip) break;
    delay(4);
  }
}

void doToss(){
  if(yaoN>=6) return;
  bool t0=esp_random()&1, t1=esp_random()&1, t2=esp_random()&1; // true=字面=2
  tossAnimate(t0,t1,t2);
  int sum=(t0?2:3)+(t1?2:3)+(t2?2:3);     // 6..9
  coinFace[yaoN][0]=t0;
  coinFace[yaoN][1]=t1;
  coinFace[yaoN][2]=t2;
  yaoValue[yaoN]=sum;
  yang[yaoN]   = (sum==7||sum==9);
  moving[yaoN] = (sum==6||sum==9);
  yaoN++;
  drawCast();                       // 先画出当前这一爻(含第六爻)
  if(yaoN>=6){
    computeResult();
    saveLastResult();
    delay(1300);                    // 停一下,让人看清完整六爻
    state=SUMMARY; drawSummary();
  }
}

void resetCast(){
  yaoN=0; moveCnt=0; detailPage=0; summaryBackHomeOnB=false;
  resultSource=COIN_CAST;
  benName[0]=bianName[0]=benFull[0]=bianFull[0]=0;
  for(int i=0;i<6;i++){
    yang[i]=false; moving[i]=false; yaoValue[i]=0;
    for(int j=0;j<3;j++) coinFace[i][j]=false;
  }
}

void openModeSelect(){
  modeChoice=0;
  state=MODE_SELECT;
  drawModeSelect();
}

void startTrigramSelect(){
  selectingUpper=true;
  state=TRIGRAM_SELECT;
  drawTrigramSelect();
}

bool commitTrigramSelection(){
  if(upperTri<0 || upperTri>=8 || lowerTri<0 || lowerTri>=8) return false;
  resetCast();
  resultSource=TRIGRAM_BROWSE;
  for(int i=0;i<6;i++){
    yang[i]=(TRIGRAM_BITS[i<3 ? lowerTri : upperTri] & (1 << (i%3)))!=0;
    yaoValue[i]=yang[i] ? 7 : 8;
  }
  yaoN=6;
  computeResult();
  state=SUMMARY;
  drawSummary();
  return true;
}

bool validLineValue(uint8_t value){ return value>=6 && value<=9; }

void selectManualPosition(int pos){
  manualPos=pos;
  manualChoice=0;
  for(int i=0;i<4;i++) if(MANUAL_CHOICES[i]==manualValues[pos]) manualChoice=i;
  state=MANUAL_ENTRY;
  drawManualEntry();
}

void startManualEntry(){
  memset(manualValues,0,sizeof(manualValues));
  manualReviewEditable=true;
  selectManualPosition(0);
}

void acceptManualLine(){
  if(manualPos<0 || manualPos>=6 || manualChoice<0 || manualChoice>=4) return;
  manualValues[manualPos]=MANUAL_CHOICES[manualChoice];
  if(manualPos==5){
    manualReviewEditable=true;
    state=MANUAL_REVIEW;
    drawManualReview();
  } else selectManualPosition(manualPos+1);
}

void backManualLine(){
  if(manualPos>0) selectManualPosition(manualPos-1);
  else openModeSelect();
}

bool commitManualResult(){
  for(int i=0;i<6;i++) if(!validLineValue(manualValues[i])) return false;
  resetCast();
  resultSource=MANUAL_CAST;
  for(int i=0;i<6;i++){
    yaoValue[i]=manualValues[i];
    yang[i]=yaoValue[i]==7 || yaoValue[i]==9;
    moving[i]=yaoValue[i]==6 || yaoValue[i]==9;
  }
  // Custom combinations are for browsing, not saved over the last coin cast.
  yaoN=6;
  computeResult();
  manualReviewEditable=false;
  state=SUMMARY;
  drawSummary();
  return true;
}

void showManualHistory(){
  memcpy(manualValues,yaoValue,sizeof(manualValues));
  manualReviewEditable=false;
  state=MANUAL_REVIEW;
  drawManualReview();
}

void setDemoCoins(int idx,int sum){
  bool f0=false,f1=false,f2=false;
  if(sum==6){ f0=true; f1=true; f2=true; }
  else if(sum==7){ f0=true; f1=true; f2=false; }
  else if(sum==8){ f0=true; f1=false; f2=false; }
  else { f0=false; f1=false; f2=false; }
  coinFace[idx][0]=f0;
  coinFace[idx][1]=f1;
  coinFace[idx][2]=f2;
}

void loadLayoutDemo(){
  resetCast();
  const int demo[6] = {9,8,6,7,8,7};
  for(int i=0;i<6;i++){
    yaoValue[i]=demo[i];
    yang[i]=(demo[i]==7 || demo[i]==9);
    moving[i]=(demo[i]==6 || demo[i]==9);
    setDemoCoins(i,demo[i]);
  }
  yaoN=6;
  computeResult();
}

void drawExitConfirm(){
  paperBg(); frame(false);
  tc("退出本次？",W/2,40,16,C_CIN);
  hRule(25,60,W-50,C_INKL);
  tc("未完成的内容",W/2,87,14,C_INK);
  tc("将会清空",W/2,108,14,C_INK);
  for(int i=0;i<2;i++){
    int y=133+i*31;
    if(discardSelected==(i==1)) cv.drawRect(23,y,89,27,C_CIN);
    tc(i==0 ? "继续" : "放弃并退出",W/2,y+13,14,C_INK);
  }
  tc("B切换 A选定",W/2,209,12,C_INK);
  tc("长B继续",W/2,223,12,C_INKL);
  cv.pushSprite(0,0);
}

void redrawCurrentScreen(){
  switch(state){
    case HOME: drawHome(); break;
    case CAST: drawCast(); break;
    case SUMMARY: drawSummary(); break;
    case DETAIL: drawDetail(); break;
    case MODE_SELECT: drawModeSelect(); break;
    case TRIGRAM_SELECT: drawTrigramSelect(); break;
    case MANUAL_ENTRY: drawManualEntry(); break;
    case MANUAL_REVIEW: drawManualReview(); break;
    case CONFIRM_EXIT: drawExitConfirm(); break;
  }
}

bool hasUnfinishedInput(){
  if(state==CAST) return yaoN>0 && yaoN<6;
  if(state==MANUAL_ENTRY || (state==MANUAL_REVIEW && manualReviewEditable)){
    for(int i=0;i<6;i++) if(validLineValue(manualValues[i])) return true;
  }
  return false;
}

void requestExit(bool toMenu){
  if(state==CONFIRM_EXIT) return;
  exitToMenu=toMenu;
  if(hasUnfinishedInput()){
    exitBackState=state;
    discardSelected=false;
    state=CONFIRM_EXIT;
    drawExitConfirm();
  } else {
    exitConfirmed=toMenu;
    state=toMenu ? HOME : MODE_SELECT;
    redrawCurrentScreen();
  }
}

void handleInput(Navigation::Event event){
  using namespace Navigation;
  if(event==NONE) return;
  if(state==HOME && event==B_LONG){
    simpleMode=!simpleMode;
    detailPage=0;
    drawHome();
    return;
  }
  if(state==CONFIRM_EXIT){
    if(event==B_LONG){ state=exitBackState; redrawCurrentScreen(); }
    else if(event==B_SHORT){ discardSelected=!discardSelected; drawExitConfirm(); }
    else if(event==A_SHORT){
      if(discardSelected){
        exitConfirmed=exitToMenu;
        state=exitToMenu ? HOME : MODE_SELECT;
        redrawCurrentScreen();
      }
      else { state=exitBackState; redrawCurrentScreen(); }
    }
    return;
  }
  const bool selecting=state==MODE_SELECT || state==TRIGRAM_SELECT || state==MANUAL_ENTRY;
  if(event==B_LONG && !selecting){ requestExit(true); return; }
  const bool next=event==A_SHORT, confirm=event==A_LONG, back=event==B_SHORT;
  const bool cycle=event==B_SHORT, select=event==A_SHORT, parent=event==B_LONG;
  switch(state){
    case HOME:
      if(next || confirm) openModeSelect();
      else if(back) requestExit(true);
      break;
    case MODE_SELECT:
      if(cycle){ modeChoice=(modeChoice+1)%4; drawModeSelect(); }
      else if(parent){ state=HOME; drawHome(); }
      else if(select){
        if(modeChoice==0){ resetCast(); state=CAST; drawCast(); }
        else if(modeChoice==1) startTrigramSelect();
        else if(modeChoice==2) startManualEntry();
        else if(restoreLastResult()){ state=SUMMARY; drawSummary(); }
      }
      break;
    case TRIGRAM_SELECT:
      if(cycle){
        int& selected=selectingUpper ? upperTri : lowerTri;
        selected=(selected+1)%8;
        drawTrigramSelect();
      } else if(select){
        if(selectingUpper){ selectingUpper=false; drawTrigramSelect(); }
        else commitTrigramSelection();
      } else if(parent){
        if(selectingUpper) openModeSelect();
        else { selectingUpper=true; drawTrigramSelect(); }
      }
      break;
    case MANUAL_ENTRY:
      if(cycle){ manualChoice=(manualChoice+1)%4; drawManualEntry(); }
      else if(select) acceptManualLine();
      else if(parent){
        if(manualPos>0) selectManualPosition(manualPos-1);
        else requestExit(false);
      }
      break;
    case MANUAL_REVIEW:
      if(manualReviewEditable){
        if(select) commitManualResult();
        else if(back) selectManualPosition(5);
      } else if(next || back){ state=SUMMARY; drawSummary(); }
      break;
    case CAST:
      if(next && yaoN<6){ doToss(); blockAnimationRelease=true; }
      else if(back) requestExit(false);
      break;
    case SUMMARY:
      if(next){ detailPage=0; state=DETAIL; drawDetail(); }
      else if(back){
        if(resultSource==TRIGRAM_BROWSE){
          selectingUpper=false; state=TRIGRAM_SELECT; drawTrigramSelect();
        } else { state=HOME; drawHome(); }
      }
      break;
    case DETAIL:
      if(next){
        if(detailPage<detailPageCount()-1){ detailPage++; drawDetail(); }
        else { detailPage=0; state=SUMMARY; drawSummary(); }
      } else if(back){
        if(detailPage>0){ detailPage--; drawDetail(); }
        else { state=SUMMARY; drawSummary(); }
      }
      break;
    case CONFIRM_EXIT: break;
  }
}

void setup(){
  auto cfg=M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  M5.Display.setRotation(0);            // 竖屏 135x240
  M5.Display.setBrightness(180);
  cv.setColorDepth(16);
  cv.createSprite(W,H);
  // vIdx 映射
  vIdx[7]=0;vIdx[3]=1;vIdx[5]=2;vIdx[1]=3;vIdx[6]=4;vIdx[2]=5;vIdx[4]=6;vIdx[0]=7;
  resetCast();
  drawHome();
}

void loop(){
  static Navigation::Reader reader;
  M5.update();
  handleInput(reader.poll(millis(),btnA().isPressed(),btnB().isPressed()));
  if(blockAnimationRelease){ reader.suppressUntilReleased(); blockAnimationRelease=false; }
  delay(10);
}
