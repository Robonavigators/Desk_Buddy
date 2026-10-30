// Desk Buddy v2 — independent implementation based only on README.md.
// Target: ESP32-S3-Zero, Arduino-ESP32 3.x.
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <TFT_eSPI.h>
#include <driver/i2s_std.h>
#include <esp_sleep.h>
#include <esp32-hal-cpu.h>
#include <time.h>
#include <math.h>

constexpr int PIN_TOUCH=3, PIN_LED=2, PIN_DHT=4, PIN_AMP_SD=14;
constexpr int PIN_I2S_DOUT=5, PIN_I2S_BCLK=6, PIN_I2S_LRCK=7;
constexpr uint16_t PORTAL_DNS=53;
constexpr uint32_t TOUCH_DEBOUNCE=35, LONG_PRESS_MS=650, DOUBLE_TAP_MS=330;
String apSSID(){
  uint64_t id=ESP.getEfuseMac();char suffix[5];
  snprintf(suffix,sizeof(suffix),"%04X",(unsigned)(id&0xFFFF));
  return "Desk_Buddy-"+String(suffix);
}
const char* AP_PASSWORD="Desk_Buddy1234@";
const char* NTP_SERVER="pool.ntp.org";
const char* TONES[]={"Mario","Zelda","Beep","StarWars"};
struct Alarm { bool enabled=false; uint8_t hour=7,minute=0,tone=2; int lastDay=-1; };
Alarm alarms[10];
struct Settings {
  String ssid,password,openWeatherKey,city="Thiruvananthapuram",accent="#0484f8";
  bool hour24=true;
  float lat=8.5241f, lon=76.9366f;
  String timezone="Asia/Kolkata";
  int utcOffset=19800;
} cfg;
struct Weather {
  float temperature=NAN, wind=NAN;
  int code=-1;
  String dates[3]; float highs[3], lows[3]; int codes[3];
  uint32_t lastAttempt=0;
  bool ready=false,forecastReady=false;
} weather;
Preferences prefs;
WebServer server(80);
DNSServer dns;
DHT dht(PIN_DHT,DHT11);
TFT_eSPI tft;
TFT_eSprite frame(&tft);
bool frameReady=false;
i2s_chan_handle_t audioChannel=nullptr;
TaskHandle_t audioTaskHandle=nullptr;
portMUX_TYPE alarmMux=portMUX_INITIALIZER_UNLOCKED;
volatile int activeAlarm=-1, requestedTone=2;
volatile uint32_t ringGeneration=0;
bool portal=false, otaStarted=false, wifiOnline=false;
uint8_t screenIndex=0;
uint32_t lastDraw=0,lastSensor=0,lastWifiAttempt=0,lastTouchTransition=0,lastShortRelease=0;
float indoorTemp=NAN, indoorHumidity=NAN;
bool touchWasDown=false, longHandled=false, pendingTap=false;
bool forcedPortal=false;
RTC_DATA_ATTR uint32_t wakeMagic=0;

uint16_t accent565(){
  long rgb=strtol(cfg.accent.substring(1).c_str(),nullptr,16);
  return tft.color565((rgb>>16)&255,(rgb>>8)&255,rgb&255);
}
String htmlEscape(String s){
  s.replace("&","&amp;"); s.replace("<","&lt;"); s.replace(">","&gt;");
  s.replace("\"","&quot;"); s.replace("'","&#39;"); return s;
}
String urlEncode(const String& s){
  const char *hex="0123456789ABCDEF"; String out;
  for(size_t i=0;i<s.length();i++){uint8_t c=(uint8_t)s[i];
    if(isalnum(c)||c=='-'||c=='_'||c=='.') out+=(char)c;
    else {out+='%';out+=hex[c>>4];out+=hex[c&15];}
  } return out;
}
void saveSettings(){
  prefs.begin("deskbuddy",false);
  prefs.putString("ssid",cfg.ssid); prefs.putString("pass",cfg.password);
  prefs.putString("owmkey",cfg.openWeatherKey);
  prefs.putString("city",cfg.city); prefs.putString("accent",cfg.accent);
  prefs.putBool("hour24",cfg.hour24); prefs.putFloat("lat",cfg.lat);
  prefs.putFloat("lon",cfg.lon); prefs.putString("tz",cfg.timezone);
  prefs.putInt("offset",cfg.utcOffset);
  for(int i=0;i<10;i++){
    String k="a"+String(i);
    uint32_t value=(alarms[i].enabled?1UL:0UL)|((uint32_t)alarms[i].hour<<1)|
      ((uint32_t)alarms[i].minute<<6)|((uint32_t)alarms[i].tone<<12);
    prefs.putUInt(k.c_str(),value);
  }
  prefs.end();
}
void loadSettings(){
  prefs.begin("deskbuddy",true);
  cfg.ssid=prefs.getString("ssid","");
  cfg.password=prefs.getString("pass","");
  cfg.openWeatherKey=prefs.getString("owmkey","");
  cfg.city=prefs.getString("city",cfg.city);
  cfg.accent=prefs.getString("accent",cfg.accent);
  cfg.hour24=prefs.getBool("hour24",true);
  cfg.lat=prefs.getFloat("lat",cfg.lat);
  cfg.lon=prefs.getFloat("lon",cfg.lon);
  cfg.timezone=prefs.getString("tz",cfg.timezone);
  cfg.utcOffset=prefs.getInt("offset",cfg.utcOffset);
  for(int i=0;i<10;i++){
    uint32_t v=prefs.getUInt(("a"+String(i)).c_str(),0);
    alarms[i].enabled=v&1; alarms[i].hour=(v>>1)&31;
    alarms[i].minute=(v>>6)&63; alarms[i].tone=(v>>12)&7;
    if(alarms[i].hour>23||alarms[i].minute>59||alarms[i].tone>3)
      alarms[i]=Alarm();
  }
  prefs.end();
}
void stopAlarm(){portENTER_CRITICAL(&alarmMux);activeAlarm=-1;ringGeneration++;portEXIT_CRITICAL(&alarmMux);}
void startAlarm(int index){
  portENTER_CRITICAL(&alarmMux);
  activeAlarm=index;requestedTone=alarms[index].tone;ringGeneration++;
  portEXIT_CRITICAL(&alarmMux);
}
void dismissAlarm(){
  int index=activeAlarm;
  if(index>=0){
    struct tm now;
    if(getLocalTime(&now,10)) alarms[index].lastDay=now.tm_yday;
    stopAlarm();
  }
}
uint32_t snoozeAt=0;
int snoozeAlarm=-1;
void snooze(){
  snoozeAlarm=activeAlarm;snoozeAt=millis()+300000UL;stopAlarm();
}
void toneSamples(float hz,uint16_t ms,uint32_t generation){
  constexpr uint32_t rate=16000;
  int16_t samples[128];
  uint32_t count=(uint32_t)rate*ms/1000,phase=0;
  uint32_t step=hz>0?(uint32_t)((hz*4294967296.0)/rate):0;
  for(uint32_t n=0;n<count;n+=128){
    if(generation!=ringGeneration||activeAlarm<0) return;
    uint32_t frames=min((uint32_t)128,count-n);
    for(uint32_t i=0;i<frames;i++){phase+=step;
      float wave=hz>0?sinf((phase/4294967296.0f)*6.2831853f):0;
      samples[i]=(int16_t)(wave*8500);
    }
    size_t written=0;
    i2s_channel_write(audioChannel,samples,frames*sizeof(int16_t),&written,50);
  }
}
struct Note { uint16_t hz,ms; };
const Note mario[]={{659,140},{659,140},{0,90},{659,140},{0,90},{523,140},{659,140},{784,280},{392,260}};
const Note zelda[]={{698,150},{880,150},{988,150},{1047,300},{0,100},{988,150},{880,150},{698,320}};
const Note starwars[]={{392,200},{392,200},{392,200},{311,150},{466,70},{392,200},{311,150},{466,70},{392,400}};
const Note beep[]={{880,220},{0,110},{880,220},{0,240}};
void audioTask(void*){
  for(;;){
    int alarm=activeAlarm;
    if(alarm<0||!audioChannel){digitalWrite(PIN_AMP_SD,LOW);vTaskDelay(pdMS_TO_TICKS(10));continue;}
    digitalWrite(PIN_AMP_SD,HIGH);
    const Note* melody=beep;size_t count=sizeof(beep)/sizeof(Note);
    switch(requestedTone){
      case 0:melody=mario;count=sizeof(mario)/sizeof(Note);break;
      case 1:melody=zelda;count=sizeof(zelda)/sizeof(Note);break;
      case 3:melody=starwars;count=sizeof(starwars)/sizeof(Note);break;
    }
    uint32_t generation=ringGeneration;
    for(size_t i=0;i<count&&generation==ringGeneration&&activeAlarm>=0;i++)
      toneSamples(melody[i].hz,melody[i].ms,generation);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
void beginAudio(){
  pinMode(PIN_AMP_SD,OUTPUT);digitalWrite(PIN_AMP_SD,LOW);
  i2s_chan_config_t chan=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0,I2S_ROLE_MASTER);
  if(i2s_new_channel(&chan,&audioChannel,nullptr)!=ESP_OK){audioChannel=nullptr;return;}
  i2s_std_config_t config={
    .clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(16000),
    .slot_cfg=I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_MONO),
    .gpio_cfg={.mclk=I2S_GPIO_UNUSED,.bclk=(gpio_num_t)PIN_I2S_BCLK,
      .ws=(gpio_num_t)PIN_I2S_LRCK,.dout=(gpio_num_t)PIN_I2S_DOUT,
      .din=I2S_GPIO_UNUSED,.invert_flags={false,false,false}}
  };
  if(i2s_channel_init_std_mode(audioChannel,&config)!=ESP_OK||
     i2s_channel_enable(audioChannel)!=ESP_OK){i2s_del_channel(audioChannel);audioChannel=nullptr;return;}
  xTaskCreatePinnedToCore(audioTask,"alarm-audio",4096,nullptr,2,&audioTaskHandle,0);
}
void drawHeader(const String& title){
  frame.fillScreen(TFT_BLACK);frame.fillRect(0,0,128,22,accent565());
  frame.setTextColor(TFT_WHITE,accent565());frame.setTextSize(1);
  frame.setCursor(7,7);frame.print(title);
}
String timeLabel(uint8_t h,uint8_t m){
  char buf[16];int hour=cfg.hour24?h:((h+11)%12+1);
  snprintf(buf,sizeof(buf),"%02d:%02d",hour,m);
  String s(buf);if(!cfg.hour24)s+=h>=12?" PM":" AM";return s;
}
void drawScreens(){
  if(!frameReady)return;
  frame.setTextFont(1);
  frame.setTextSize(1);
  int alarm=activeAlarm;
  if(alarm>=0){
    drawHeader("ALARM RINGING");
    frame.setTextSize(2);frame.setTextColor(TFT_YELLOW,TFT_BLACK);
    frame.setCursor(6,44);frame.print(timeLabel(alarms[alarm].hour,alarms[alarm].minute));
    frame.setTextSize(1);frame.setCursor(5,105);frame.print("Tap: stop");
    frame.setCursor(5,119);frame.print("2 taps: snooze");
    frame.setCursor(5,133);frame.print("Hold: dismiss");frame.pushSprite(0,0);return;
  }
  struct tm now;
  bool valid=getLocalTime(&now,2);
  switch(screenIndex){
    case 0:{
      drawHeader("DESK BUDDY");
      frame.setTextColor(TFT_WHITE,TFT_BLACK);frame.setTextSize(2);
      frame.setCursor(4,40);frame.print(valid?timeLabel(now.tm_hour,now.tm_min):"--:--");
      frame.setTextSize(1);frame.setCursor(5,80);
      if(valid){char d[28];strftime(d,sizeof(d),"%a %d %b %Y",&now);frame.print(d);}
      else frame.print("Syncing time...");
      frame.setCursor(5,103);frame.printf("Room: %.0f C",indoorTemp);
      frame.setCursor(5,118);frame.printf("Humidity: %.0f%%",indoorHumidity);
      frame.setCursor(5,143);frame.print(wifiOnline?"WiFi connected":"Offline");break;
    }
    case 1:{
      drawHeader("WEATHER");
      frame.setTextColor(TFT_WHITE,TFT_BLACK);frame.setTextSize(1);
      frame.setCursor(5,34);frame.print(cfg.city.substring(0,20));
      frame.setTextSize(2);frame.setCursor(7,61);
      if(weather.ready)frame.printf("%.1f C",weather.temperature);else frame.print("-- C");
      frame.setTextSize(1);frame.setCursor(5,104);
      if(weather.ready)frame.printf("Code: %d",weather.code);
      frame.setCursor(5,122);if(weather.ready)frame.printf("Wind: %.0f km/h",weather.wind);
      break;
    }
    case 2:{
      drawHeader("3 DAY FORECAST");
      frame.setTextColor(TFT_WHITE,TFT_BLACK);frame.setTextSize(1);
      for(int i=0;i<3;i++){
        frame.setCursor(4,34+i*38);
        if(weather.forecastReady){frame.print(weather.dates[i]);frame.setCursor(4,46+i*38);
          frame.printf("%.0f / %.0f C",weather.highs[i],weather.lows[i]);}
        else frame.print("Waiting for WiFi...");
      }break;
    }
    case 3:{
      drawHeader("ALARMS");
      frame.setTextColor(TFT_WHITE,TFT_BLACK);frame.setTextSize(1);
      for(int i=0;i<10;i++){frame.setCursor(3,25+i*13);
        frame.printf("%02d %s %s",i+1,
          timeLabel(alarms[i].hour,alarms[i].minute).c_str(),alarms[i].enabled?"ON":"--");
      }break;
    }
    default:{
      frame.fillScreen(TFT_BLACK);
      frame.fillRoundRect(10,blink?72:48,44,blink?5:53,blink?2:12,accent565());
      frame.fillRoundRect(74,blink?72:48,44,blink?5:53,blink?2:12,accent565());
      int drift=(int)roundf(sinf(millis()*0.0024f)*7.0f);
      bool blink=(millis()%5100)<120;
      if(!blink)frame.fillCircle(32+drift,74,10,TFT_BLACK);
      if(!blink)frame.fillCircle(96+drift,74,10,TFT_BLACK);
      frame.setTextColor(TFT_WHITE,TFT_BLACK);frame.setTextSize(1);
      frame.setCursor(28,125);frame.print("HELLO!");break;
    }
  }
  frame.pushSprite(0,0);
}
String portalHtml(){
  String p=R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Desk Buddy Setup</title><style>
*{box-sizing:border-box}body{margin:0;font:15px system-ui;background:linear-gradient(130deg,#0484f8,#10284e);color:white;min-height:100vh;padding:24px}
main{max-width:650px;margin:auto;background:#ffffff23;border:1px solid #ffffff55;backdrop-filter:blur(20px);border-radius:24px;padding:25px;box-shadow:0 20px 60px #0005}
h1{margin-top:0}label{display:block;margin:12px 0 5px}input,select{width:100%;padding:12px;background:#ffffffdd;border:0;border-radius:10px;color:#152443}
button{background:#fff;color:#0469c9;font-weight:bold;border:0;border-radius:12px;padding:14px;width:100%;margin-top:20px}
.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}.alarm{border-top:1px solid #ffffff66;padding:10px 0}
small{opacity:.85}</style></head><body><main><h1>Desk Buddy</h1><p>Configure your smart desk companion</p><form action="/save" method="post">
<label>Wi-Fi network</label><select name="ssid"><option value="">Choose network</option>)HTML";
  int n=WiFi.scanNetworks(false,true);
  for(int i=0;i<n;i++)p+="<option value=\""+htmlEscape(WiFi.SSID(i))+"\""+(cfg.ssid==WiFi.SSID(i)?" selected":"")+">"+htmlEscape(WiFi.SSID(i))+"</option>";
  p+="</select><label>Wi-Fi password (leave blank to keep saved)</label><input name='password' type='password' placeholder='Wi-Fi password'>";
  p+="<label>OpenWeatherMap API key (optional)</label><input type='password' name='owmkey' autocomplete='off' placeholder='Paste key, blank keeps saved'>";
  if(!cfg.openWeatherKey.isEmpty())p+="<small>API key saved. Leave blank to keep it.</small>";
  p+="<label><input type='checkbox' name='clearowm' value='1'> Clear saved API key</label>";
  p+="<label>City</label><input name='city' value='"+htmlEscape(cfg.city)+"' required>";
  p+="<label>Time format</label><select name='hour24'><option value='1'"+String(cfg.hour24?" selected":"")+">24-hour</option><option value='0'"+String(!cfg.hour24?" selected":"")+">12-hour</option></select>";
  p+="<label>Accent color</label><input type='color' name='accent' value='"+htmlEscape(cfg.accent)+"'>";
  p+="<h2>10 alarms</h2><small>Use HH:MM in 24-hour time.</small>";
  for(int i=0;i<10;i++){
    String k=String(i); char tm[6];snprintf(tm,sizeof(tm),"%02u:%02u",alarms[i].hour,alarms[i].minute);
    p+="<div class='alarm'><b>Alarm "+String(i+1)+"</b><div class='grid'>";
    p+="<label><input type='checkbox' name='e"+k+"' value='1'"+String(alarms[i].enabled?" checked":"")+">On</label>";
    p+="<input type='time' name='t"+k+"' value='"+String(tm)+"' style='grid-column:span 2'>";
    p+="<select name='m"+k+"'>";
    for(int j=0;j<4;j++)p+="<option value='"+String(j)+"'"+String(alarms[i].tone==j?" selected":"")+">"+TONES[j]+"</option>";
    p+="</select></div></div>";
  }
  p+="<button type='submit'>Save All Settings</button></form></main></body></html>";
  return p;
}
bool validColor(const String& s){
  if(s.length()!=7||s[0]!='#')return false;
  for(int i=1;i<7;i++)if(!isxdigit((unsigned char)s[i]))return false;
  return true;
}
void handleSave(){
  String ssid=server.arg("ssid"),city=server.arg("city"),color=server.arg("accent");
  if(ssid.isEmpty()||city.isEmpty()||city.length()>80||!validColor(color)){
    server.send(400,"text/plain","Select Wi-Fi, enter city and valid color.");return;
  }
  cfg.ssid=ssid;
  if(server.hasArg("password")&&!server.arg("password").isEmpty())cfg.password=server.arg("password");
  if(server.hasArg("clearowm"))cfg.openWeatherKey="";
  else if(server.hasArg("owmkey")&&!server.arg("owmkey").isEmpty()){
    String key=server.arg("owmkey");key.trim();
    if(key.length()>128){server.send(400,"text/plain","API key too long");return;}
    cfg.openWeatherKey=key;
  }
  cfg.city=city;cfg.accent=color;cfg.hour24=server.arg("hour24")=="1";
  for(int i=0;i<10;i++){
    String k=String(i),tm=server.arg("t"+k);
    int h=-1,m=-1;
    if(sscanf(tm.c_str(),"%d:%d",&h,&m)!=2||h<0||h>23||m<0||m>59){
      server.send(400,"text/plain","Invalid alarm time.");return;
    }
    alarms[i].hour=h;alarms[i].minute=m;
    alarms[i].enabled=server.hasArg("e"+k);
    alarms[i].tone=constrain(server.arg("m"+k).toInt(),0,3);
  }
  saveSettings();
  server.send(200,"text/html","<h2>Saved! Desk Buddy is restarting.</h2>");
  delay(600);ESP.restart();
}
void startPortal(){
  portal=true;wifiOnline=false;
  WiFi.mode(WIFI_AP_STA);
  String ap=apSSID();
  WiFi.softAP(ap.c_str(),AP_PASSWORD);
  dns.start(PORTAL_DNS,"*",WiFi.softAPIP());
  server.on("/",HTTP_GET,[]{server.send(200,"text/html",portalHtml());});
  server.on("/save",HTTP_POST,handleSave);
  server.on("/generate_204",HTTP_GET,[]{server.sendHeader("Location","/",true);server.send(302,"text/plain","");});
  server.on("/hotspot-detect.html",HTTP_GET,[]{server.sendHeader("Location","/",true);server.send(302,"text/plain","");});
  server.onNotFound([]{server.sendHeader("Location","http://192.168.4.1/",true);server.send(302,"text/plain","");});
  server.begin();
}
void connectWifi(){
  if(cfg.ssid.isEmpty())return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.ssid.c_str(),cfg.password.c_str());
  lastWifiAttempt=millis();
}
bool getJson(const String& url,JsonDocument& doc){
  WiFiClientSecure client;client.setInsecure(); // HTTPS encryption without CA verification; see BUILD.md
  HTTPClient http;http.setTimeout(6500);
  if(!http.begin(client,url))return false;
  int status=http.GET();
  bool ok=status==HTTP_CODE_OK && deserializeJson(doc,http.getStream())==DeserializationError::Ok;
  http.end();return ok;
}
void fetchWeather(){
  if(!wifiOnline)return;
  weather.lastAttempt=millis();
  JsonDocument geo;
  String url="https://geocoding-api.open-meteo.com/v1/search?name="+urlEncode(cfg.city)+"&count=1";
  if(getJson(url,geo)&&geo["results"].is<JsonArray>()&&!geo["results"].as<JsonArray>().isEmpty()){
    JsonObject place=geo["results"][0];
    cfg.lat=place["latitude"]|cfg.lat;cfg.lon=place["longitude"]|cfg.lon;
    cfg.timezone=(const char*)(place["timezone"]|"Asia/Kolkata");
    configTime(cfg.utcOffset,0,NTP_SERVER,"time.google.com");
  }
  JsonDocument doc;
  String forecast="https://api.open-meteo.com/v1/forecast?latitude="+String(cfg.lat,5)+"&longitude="+String(cfg.lon,5)+
    "&current=temperature_2m,weather_code,wind_speed_10m&daily=weather_code,temperature_2m_max,temperature_2m_min&forecast_days=3&timezone=auto";
  if(!getJson(forecast,doc))return;
  cfg.utcOffset=doc["utc_offset_seconds"]|cfg.utcOffset;
  configTime(cfg.utcOffset,0,NTP_SERVER,"time.google.com");
  weather.temperature=doc["current"]["temperature_2m"]|NAN;
  weather.code=doc["current"]["weather_code"]|-1;
  weather.wind=doc["current"]["wind_speed_10m"]|NAN;
  for(int i=0;i<3;i++){
    weather.dates[i]=String((const char*)(doc["daily"]["time"][i]|"---"));
    weather.highs[i]=doc["daily"]["temperature_2m_max"][i]|NAN;
    weather.lows[i]=doc["daily"]["temperature_2m_min"][i]|NAN;
    weather.codes[i]=doc["daily"]["weather_code"][i]|-1;
  }
  weather.ready=true;
  saveSettings();
}
void enterSleep(){
  stopAlarm();digitalWrite(PIN_AMP_SD,LOW);
  tft.writecommand(0x28); // TFT display off
  digitalWrite(PIN_LED,HIGH);
  gpio_hold_en((gpio_num_t)PIN_LED);
  gpio_deep_sleep_hold_en();
  // TTP223 active-HIGH, ESP32-S3 supports EXT1 level wakeup.
  esp_sleep_enable_ext1_wakeup(1ULL<<PIN_TOUCH,ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_deep_sleep_start();
}
void onTap(){
  if(activeAlarm>=0){stopAlarm();return;}
  screenIndex=(screenIndex+1)%5;lastDraw=0;
}
void onDouble(){
  if(activeAlarm>=0){snooze();return;}
  prefs.begin("deskbuddy",false);prefs.putBool("setup",true);prefs.end();
  ESP.restart();
}
void onHold(){
  if(activeAlarm>=0)dismissAlarm();
  else enterSleep();
}
void handleTouch(){
  bool pressed=digitalRead(PIN_TOUCH)==HIGH;
  uint32_t now=millis();
  if(pressed!=touchWasDown && now-lastTouchTransition>=TOUCH_DEBOUNCE){
    touchWasDown=pressed;lastTouchTransition=now;
    if(pressed){longHandled=false;}
    else if(!longHandled){
      if(pendingTap && now-lastShortRelease<DOUBLE_TAP_MS){
        pendingTap=false;onDouble();
      }else{pendingTap=true;lastShortRelease=now;}
    }
  }
  if(pressed&&!longHandled&&now-lastTouchTransition>=LONG_PRESS_MS){
    longHandled=true;pendingTap=false;onHold();
  }
  if(pendingTap&&now-lastShortRelease>=DOUBLE_TAP_MS){
    pendingTap=false;onTap();
  }
}
void checkAlarms(){
  if(portal||activeAlarm>=0)return;
  if(snoozeAlarm>=0 && (int32_t)(millis()-snoozeAt)>=0){
    int a=snoozeAlarm;snoozeAlarm=-1;startAlarm(a);return;
  }
  struct tm now;
  if(!getLocalTime(&now,2))return;
  for(int i=0;i<10;i++){
    if(alarms[i].enabled&&alarms[i].hour==now.tm_hour&&
       alarms[i].minute==now.tm_min&&alarms[i].lastDay!=now.tm_yday){
      alarms[i].lastDay=now.tm_yday;startAlarm(i);break;
    }
  }
}
void checkThermal(){
  // ESP32-S3 temperature is chip temperature, not room temperature.
  float c=temperatureRead();
  if(!isfinite(c))return;
  if(c>=70){enterSleep();return;}
  static bool throttled=false;
  if(c>=65&&!throttled){setCpuFrequencyMhz(80);throttled=true;}
  if(c<=52&&throttled){setCpuFrequencyMhz(240);throttled=false;}
}
void setup(){
  Serial.begin(115200);
  pinMode(PIN_LED,OUTPUT);digitalWrite(PIN_LED,HIGH);
  pinMode(PIN_TOUCH,INPUT);
  touchWasDown=digitalRead(PIN_TOUCH)==HIGH;
  gpio_hold_dis((gpio_num_t)PIN_LED);
  tft.init();tft.setRotation(0);tft.fillScreen(TFT_BLACK);
  frame.setColorDepth(16);
  frameReady=frame.createSprite(128,160)!=nullptr;
  if(!frameReady){frame.setColorDepth(8);frameReady=frame.createSprite(128,160)!=nullptr;}
  if(!frameReady){Serial.println("Sprite allocation failed");tft.print("Display RAM error");}
  dht.begin();loadSettings();beginAudio();
  prefs.begin("deskbuddy",false);forcedPortal=prefs.getBool("setup",false);
  prefs.putBool("setup",false);prefs.end();
  if(cfg.ssid.isEmpty()||forcedPortal)startPortal();
  else connectWifi();
  drawScreens();
}
void loop(){
  handleTouch();
  uint32_t now=millis();
  if(portal){dns.processNextRequest();server.handleClient();}
  else {
    bool online=WiFi.status()==WL_CONNECTED;
    if(online&&!wifiOnline){
      wifiOnline=true;
      configTzTime(cfg.timezone.c_str(),NTP_SERVER,"time.google.com");
      ArduinoOTA.setHostname("Desk_Buddy");
      ArduinoOTA.begin();otaStarted=true;
      weather.lastAttempt=now-1800001UL;
    }
    if(!online&&wifiOnline){wifiOnline=false;otaStarted=false;}
    if(!online&&now-lastWifiAttempt>20000){WiFi.reconnect();lastWifiAttempt=now;}
    if(otaStarted&&online)ArduinoOTA.handle();
    checkAlarms();
    if(online&&(!weather.ready||now-weather.lastAttempt>1800000UL)&&now-weather.lastAttempt>10000)
      fetchWeather();
  }
  if(now-lastSensor>3000){lastSensor=now;float t=dht.readTemperature(),h=dht.readHumidity();
    if(isfinite(t))indoorTemp=t;if(isfinite(h))indoorHumidity=h;
    checkThermal();
  }
  uint32_t renderPeriod=(activeAlarm<0&&screenIndex==4)?100:1000;
  if(lastDraw==0||now-lastDraw>=renderPeriod){lastDraw=now;drawScreens();}
  delay(3); // cooperative yield; Core 0 audio does not rely on this loop.
}
