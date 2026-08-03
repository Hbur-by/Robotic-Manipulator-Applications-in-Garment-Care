/**
 * RMAGC — Robotic Manipulator Applications in Garment Care
 *
 * ESP32-S3 Firmware (Provincial Competition Version)
 *
 * Features:
 *   - 6-axis servo control via PCA9685
 *   - Slide rail motor control
 *   - OV2640 camera with MJPEG streaming
 *   - HSV color detection (red objects)
 *   - WiFi AP + built-in web control panel
 *   - Pre-programmed competition & game demo sequences
 *   - Serial command interface for debugging
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_camera.h>
#include <img_converters.h>
#include "config.h"

/**************************** WiFi & Web Server ****************************/
WebServer server(80);

/**************************** PCA9685 ****************************/
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

/**************************** Global State ****************************/
int angles[JOINT_COUNT] = {53, 230, 190, 240, 85, 135};

enum SlideState { STOP, FORWARD, REVERSE };
SlideState slideState = STOP;
bool demoRunning = false;

bool cam_ok = false;
String cached_detect_json = "{\"detected\":false}";
String arm_state = "闲置";

enum ActionMode { MODE_NONE, MODE_COMPETITION, MODE_GAME, MODE_IDLE };
volatile ActionMode webAction = MODE_NONE;

/**************************** Non-blocking Delay ****************************/
void delayNonBlock(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    delay(1);
  }
}

/**************************** Servo & Slide Control ****************************/
int angleToPWM(int angle) {
  int pulse = map(angle, 0, 240, SERVOMIN, SERVOMAX);
  return constrain(pulse, SERVOMIN, SERVOMAX);
}

void setJoint(uint8_t ch, int angle) {
  if (ch >= JOINT_COUNT) return;
  angle = constrain(angle, 0, 240);
  pwm.setPWM(ch, 0, angleToPWM(angle));
  angles[ch] = angle;
}

void setSlide(SlideState state) {
  switch (state) {
    case FORWARD: pwm.setPWM(SLIDE_CH, 0, SLIDE_FORWARD); break;
    case REVERSE: pwm.setPWM(SLIDE_CH, 0, SLIDE_REVERSE); break;
    default:      pwm.setPWM(SLIDE_CH, 0, SLIDE_STOP);    break;
  }
  slideState = state;
}

void fanOn()  { digitalWrite(FAN_PIN, HIGH); }
void fanOff() { digitalWrite(FAN_PIN, LOW);  }

void resetAll() {
  for (int i = 0; i < JOINT_COUNT; i++) {
    setJoint(i, HOME_ANGLES[i]);
  }
  setSlide(STOP);
  fanOff();
  demoRunning = false;
  arm_state = "闲置";
}

/**************************** Smooth Linear Move (with server keep-alive) ****************************/
void moveLinear(const int target[JOINT_COUNT], unsigned long durationMs) {
  int start[JOINT_COUNT];
  for (int i = 0; i < JOINT_COUNT; i++) start[i] = angles[i];

  unsigned long startTime = millis();
  while (millis() - startTime < durationMs) {
    server.handleClient();
    float progress = (float)(millis() - startTime) / durationMs;
    if (progress > 1.0) progress = 1.0;
    for (int i = 0; i < JOINT_COUNT; i++) {
      int ang = start[i] + (target[i] - start[i]) * progress;
      pwm.setPWM(i, 0, angleToPWM(constrain(ang, 0, 240)));
      angles[i] = ang;
    }
    delay(20);
  }
  for (int i = 0; i < JOINT_COUNT; i++) setJoint(i, target[i]);
}

/**************************** Competition Demo Sequence ****************************/
void executeCompetitionSequence() {
  demoRunning = true;
  arm_state = "比赛演示";

  const int home[JOINT_COUNT] = {53, 230, 190, 240, 85, 135};
  moveLinear(home, 500);
  setSlide(STOP);
  delayNonBlock(300);

  // Step 1: Rotate toward target
  const int step1[JOINT_COUNT] = {83, 230, 190, 240, 85, 135};
  moveLinear(step1, 1000);
  delayNonBlock(200);

  // Step 2: Slide reverse 8s + joint interpolation
  const int step2Start[JOINT_COUNT]  = {83, 230, 190, 240, 85, 135};
  const int step2Target[JOINT_COUNT] = {43, 240, 190, 145, 85, 135};
  setSlide(REVERSE);
  unsigned long startTime = millis();
  while (millis() - startTime < 8000) {
    server.handleClient();
    float p = (float)(millis() - startTime) / 8000;
    if (p > 1.0) p = 1.0;
    for (int i = 0; i < JOINT_COUNT; i++) {
      int ang = step2Start[i] + (step2Target[i] - step2Start[i]) * p;
      pwm.setPWM(i, 0, angleToPWM(constrain(ang, 0, 240)));
      angles[i] = ang;
    }
    delay(30);
  }
  for (int i = 0; i < JOINT_COUNT; i++) setJoint(i, step2Target[i]);
  setSlide(STOP);
  delayNonBlock(300);

  // Step 3: Fan on + gripping motion
  fanOn();
  const int step3[JOINT_COUNT] = {43, 240, 190, 175, 225, 45};
  moveLinear(step3, 2000);
  delayNonBlock(200);

  const int step4[JOINT_COUNT] = {43, 240, 190, 145, 85, 135};
  moveLinear(step4, 2000);
  delayNonBlock(200);

  // Repeat gripping motion
  moveLinear(step3, 2000);
  delayNonBlock(200);
  moveLinear(step4, 2000);
  delayNonBlock(200);

  // Step 7: Slide forward 8s return
  const int step7Start[JOINT_COUNT]  = {43, 240, 190, 145, 85, 135};
  const int step7Target[JOINT_COUNT] = {53, 230, 190, 240, 85, 135};
  setSlide(FORWARD);
  startTime = millis();
  while (millis() - startTime < 8000) {
    server.handleClient();
    float p = (float)(millis() - startTime) / 8000;
    if (p > 1.0) p = 1.0;
    for (int i = 0; i < JOINT_COUNT; i++) {
      int ang = step7Start[i] + (step7Target[i] - step7Start[i]) * p;
      pwm.setPWM(i, 0, angleToPWM(constrain(ang, 0, 240)));
      angles[i] = ang;
    }
    delay(30);
  }
  for (int i = 0; i < JOINT_COUNT; i++) setJoint(i, step7Target[i]);
  setSlide(STOP);
  fanOff();

  arm_state = "闲置";
  demoRunning = false;
}

/**************************** Game Mode Sequence ****************************/
void executeGameMode() {
  demoRunning = true;
  arm_state = "游戏模式";

  const int home[JOINT_COUNT] = {53, 230, 190, 240, 85, 135};
  moveLinear(home, 500);
  setSlide(STOP);
  delayNonBlock(300);

  const int preset[JOINT_COUNT] = {43, 240, 190, 175, 225, 45};
  setSlide(FORWARD);
  moveLinear(preset, 10000);
  setSlide(STOP);
  delayNonBlock(300);

  for (int round = 1; round <= 3; round++) {
    unsigned long wait = random(4000, 10001);
    delayNonBlock(wait);

    const int grab[JOINT_COUNT] = {53, 230, 190, 175, 230, 140};
    for (int i = 0; i < JOINT_COUNT; i++) setJoint(i, grab[i]);
    delayNonBlock(300);

    const int release[JOINT_COUNT] = {43, 240, 190, 175, 225, 45};
    for (int i = 0; i < JOINT_COUNT; i++) setJoint(i, release[i]);

    if (round < 3) delayNonBlock(3000);
  }

  // Wave goodbye
  const int bye1[JOINT_COUNT] = {53, 240, 190, 240, 225, 135};
  const int bye2[JOINT_COUNT] = {53, 240, 190, 240, 225, 45};
  for (int i = 0; i < 2; i++) {
    moveLinear(bye1, 500);
    delayNonBlock(400);
    moveLinear(bye2, 500);
    delayNonBlock(400);
  }

  const int returnTarget[JOINT_COUNT] = {53, 230, 190, 240, 85, 135};
  setSlide(REVERSE);
  moveLinear(returnTarget, 10000);
  setSlide(STOP);

  arm_state = "闲置";
  demoRunning = false;
}

/**************************** Color Detection ****************************/
struct DetectResult {
  bool detected;
  float x_pct, y_pct, w_pct, h_pct, cx_pct, cy_pct;
};

DetectResult detectFromFrameBuffer(camera_fb_t* fb) {
  DetectResult res = {false, 0, 0, 0, 0, 0, 0};
  if (!fb) return res;

  size_t rgb_size = fb->width * fb->height * 3;
  uint8_t* rgb_buf = (uint8_t*)ps_malloc(rgb_size);
  if (!rgb_buf) return res;

  if (!fmt2rgb888(fb->buf, fb->len, fb->format, rgb_buf)) {
    free(rgb_buf);
    return res;
  }

  int w = fb->width, h = fb->height;
  int min_x = w, min_y = h, max_x = 0, max_y = 0;
  long sum_x = 0, sum_y = 0;
  int red_cnt = 0;
  int step = 3;

  for (int y = 0; y < h; y += step) {
    for (int x = 0; x < w; x += step) {
      int i = (y * w + x) * 3;
      int r = rgb_buf[i], gv = rgb_buf[i + 1], b = rgb_buf[i + 2];

      // White balance compensation
      r  = constrain((r * 120) / 100, 0, 255);
      gv = constrain((gv * 80) / 100, 0, 255);
      b  = constrain((b * 105) / 100, 0, 255);

      if (r > gv * 1.1f && r > b * 1.1f && r > 30) {
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        sum_x += x;
        sum_y += y;
        red_cnt++;
      }
    }
  }

  free(rgb_buf);

  if (red_cnt > 30) {
    float px = w * 0.02f, py = h * 0.02f;
    res.x_pct  = max(0.0f, (min_x - px) * 100.0f / w);
    res.y_pct  = max(0.0f, (min_y - py) * 100.0f / h);
    res.w_pct  = min(100.0f - res.x_pct, (max_x - min_x + px * 2) * 100.0f / w);
    res.h_pct  = min(100.0f - res.y_pct, (max_y - min_y + py * 2) * 100.0f / h);
    res.cx_pct = ((float)sum_x / red_cnt) * 100.0f / w;
    res.cy_pct = ((float)sum_y / red_cnt) * 100.0f / h;
    res.detected = true;
  }

  return res;
}

DetectResult detectColorFromCamera() {
  if (!cam_ok) return {false, 0, 0, 0, 0, 0, 0};
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) return {false, 0, 0, 0, 0, 0, 0};
  DetectResult res = detectFromFrameBuffer(fb);
  esp_camera_fb_return(fb);
  return res;
}

String detectResultToJson(const DetectResult& res) {
  if (res.detected) {
    String json = "{";
    json += "\"detected\":true,";
    json += "\"x\":" + String(res.x_pct, 1) + ",";
    json += "\"y\":" + String(res.y_pct, 1) + ",";
    json += "\"w\":" + String(res.w_pct, 1) + ",";
    json += "\"h\":" + String(res.h_pct, 1) + ",";
    json += "\"cx\":" + String(res.cx_pct, 1) + ",";
    json += "\"cy\":" + String(res.cy_pct, 1) + ",";
    json += "\"color_en\":\"red\",";
    json += "\"type\":\"--\"";
    json += "}";
    return json;
  }
  return "{\"detected\":false,\"type\":\"--\"}";
}

/**************************** Web UI HTML ****************************/
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>RMAGC 焕衣智联</title>
<style>
:root{--bg:#f0f2f5;--card:#fff;--text:#1a1a2e;--sub:#8e8e93;--radius:20px}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,sans-serif;background:var(--bg);color:var(--text);min-height:100vh}
.header{background:var(--card);padding:16px 20px;display:flex;align-items:center;justify-content:space-between;position:sticky;top:0;z-index:10}
.logo{width:38px;height:38px;border-radius:10px;background:linear-gradient(135deg,#4A90D9,#8B5CF6);display:flex;align-items:center;justify-content:center;color:#fff;font-size:20px;font-weight:700}
.status-dot{width:9px;height:9px;border-radius:50%;background:#34C759;animation:pulse 2s infinite}
@keyframes pulse{0%,100%{box-shadow:0 0 0 0 rgba(52,199,89,.4)}50%{box-shadow:0 0 0 8px rgba(52,199,89,0)}}
.main{padding:16px;max-width:560px;margin:0 auto}
.camera-card{background:var(--card);border-radius:var(--radius);box-shadow:0 2px 16px rgba(0,0,0,.06);overflow:hidden;margin-bottom:16px}
.camera-label{padding:14px 18px;font-size:14px;font-weight:600;border-bottom:1px solid #f0f0f0}
.live-badge{font-size:10px;background:#FF3B30;color:#fff;padding:2px 8px;border-radius:10px;animation:livePulse 1.5s infinite}
@keyframes livePulse{0%,100%{opacity:1}50%{opacity:.5}}
.camera-body{position:relative;background:#000;aspect-ratio:4/3}
.camera-body img{width:100%;height:100%;object-fit:contain}
.detect-box{position:absolute;top:0;left:0;width:0;height:0;opacity:0;border:2px solid #3B82F6;pointer-events:none;z-index:5;transition:opacity .25s,left .3s,top .3s,width .3s,height .3s}
.detect-box.active{opacity:1}
.detect-label{position:absolute;top:-11px;left:50%;transform:translateX(-50%);background:#3B82F6;color:#fff;font-size:10px;padding:2px 12px;border-radius:10px;opacity:0;transition:opacity .25s}
.detect-box.active .detect-label{opacity:1}
.status-row{display:flex;gap:10px;margin-bottom:16px}
.chip{flex:1;background:var(--card);border-radius:14px;padding:12px;box-shadow:0 2px 16px rgba(0,0,0,.06);display:flex;align-items:center;gap:10px}
.chip-val{font-size:16px;font-weight:700}
.chip-label{font-size:11px;color:var(--sub)}
.btn-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:16px}
.btn-card{background:var(--card);border:none;border-radius:var(--radius);box-shadow:0 2px 16px rgba(0,0,0,.06);padding:20px;display:flex;flex-direction:column;align-items:center;gap:10px;cursor:pointer}
.btn-card:active{transform:scale(.95)}
.btn-card:disabled{opacity:.4}
.btn-icon{width:48px;height:48px;border-radius:14px;display:flex;align-items:center;justify-content:center}
.btn-competition .btn-icon{background:linear-gradient(135deg,#4A90D9,#357ABD)}
.btn-game .btn-icon{background:linear-gradient(135deg,#FF6B6B,#E5484D)}
.btn-idle .btn-icon{background:linear-gradient(135deg,#8B5CF6,#7C3AED)}
.btn-text{font-size:14px;font-weight:600}
.toast-wrap{position:fixed;bottom:24px;left:16px;right:16px;display:flex;justify-content:center;pointer-events:none;z-index:100}
.toast{padding:12px 24px;border-radius:26px;font-size:14px;font-weight:600;color:#fff;opacity:0;transition:all .35s}
.toast.show{opacity:1}
.toast.ok{background:rgba(52,199,89,.92)}
.toast.err{background:rgba(255,59,48,.92)}
</style>
</head>
<body>
<div class="header">
  <div class="logo">焕</div>
  <div class="status-dot"></div>
</div>
<div class="main">
  <div class="camera-card">
    <div class="camera-label">实时画面 <span class="live-badge">LIVE</span></div>
    <div class="camera-body">
      <img id="camImg" src="/stream" alt="Camera">
      <div class="detect-box" id="detectBox">
        <div class="detect-label" id="detectLabel"></div>
      </div>
    </div>
  </div>
  <div class="status-row">
    <div class="chip"><div class="chip-label">机械臂状态</div><div class="chip-val" id="state">闲置</div></div>
    <div class="chip"><div class="chip-label">画面帧率</div><div class="chip-val" id="fps">-- fps</div></div>
  </div>
  <div class="status-row">
    <div class="chip"><div class="chip-label">衣物类型</div><div class="chip-val" id="clothType">--</div></div>
    <div class="chip"><div class="chip-label">颜色识别</div><div class="chip-val" id="clothColor">--</div></div>
  </div>
  <div class="btn-grid">
    <button class="btn-card btn-competition" onclick="sendCmd('competition')"><div class="btn-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.5"><polygon points="5 3 19 12 5 21 5 3"/></svg></div><span class="btn-text">比赛模式</span></button>
    <button class="btn-card btn-game" onclick="sendCmd('game')"><div class="btn-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.5"><path d="M12 2L8 8h8l-4-6z"/><path d="M8 8v10a2 2 0 004 0V8"/></svg></div><span class="btn-text">游戏模式</span></button>
    <button class="btn-card btn-idle" onclick="sendCmd('idle')"><div class="btn-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.5"><rect x="6" y="6" width="12" height="12" rx="1"/><line x1="8" y1="12" x2="16" y2="12"/></svg></div><span class="btn-text">归 位</span></button>
  </div>
  <div style="text-align:center;font-size:10px;color:#c0c0c0">RMAGC · ESP32-S3</div>
</div>
<div class="toast-wrap"><div id="toast" class="toast"></div></div>
<script>
var labels={competition:'比赛模式',game:'游戏模式',idle:'归位'};
function toast(msg,type){
  var t=document.getElementById('toast');
  t.textContent=msg;t.className='toast '+type+' show';
  clearTimeout(t._tid);t._tid=setTimeout(function(){t.classList.remove('show')},2200);
}
function sendCmd(c){
  document.querySelectorAll('.btn-card').forEach(b=>b.disabled=true);
  fetch('/cmd?action='+c)
    .then(r=>r.text())
    .then(t=>{toast(t,'ok');document.getElementById('state').textContent=labels[c];})
    .catch(()=>toast('发送失败','err'))
    .finally(()=>document.querySelectorAll('.btn-card').forEach(b=>b.disabled=false));
}
var fc=0,lastTs=0;
setInterval(()=>{
  var n=Date.now();
  if(n-lastTs>=1000){document.getElementById('fps').textContent=fc+' fps';fc=0;lastTs=n}
},800);
document.getElementById('camImg').addEventListener('load',()=>fc++);
function updateDetectBox(d){
  var box=document.getElementById('detectBox');
  var label=document.getElementById('detectLabel');
  if(d.detected){
    box.className='detect-box active';
    box.style.left=d.x+'%';
    box.style.top=d.y+'%';
    box.style.width=d.w+'%';
    box.style.height=d.h+'%';
    label.textContent='RED';
  }else{
    box.className='detect-box';
    label.textContent='';
  }
  document.getElementById('clothColor').textContent=d.detected?'红色':'--';
}
function fetchDetect(){
  fetch('/api/detect')
    .then(r=>r.json())
    .then(d=>{
      document.getElementById('clothType').textContent=d.type||'--';
      updateDetectBox(d);
    })
    .catch(e=>console.error('Fetch detect error:',e));
}
fetchDetect();
setInterval(fetchDetect,250);
</script>
</body>
</html>
)rawliteral";

/**************************** Web Server Routes ****************************/
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleCmd() {
  String action = server.arg("action");
  if (action == "competition") {
    if (!demoRunning) webAction = MODE_COMPETITION;
    server.send(200, "text/plain", "比赛模式启动");
  } else if (action == "game") {
    if (!demoRunning) webAction = MODE_GAME;
    server.send(200, "text/plain", "游戏模式启动");
  } else if (action == "idle") {
    webAction = MODE_IDLE;
    server.send(200, "text/plain", "机械臂归位");
  } else {
    server.send(400, "text/plain", "未知指令");
  }
}

void handleDetect() {
  server.send(200, "application/json", cached_detect_json);
}

void handleStream() {
  WiFiClient client = server.client();
  if (!client || !cam_ok) return;

  String boundary = "frame";
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=" + boundary);
  client.println();

  static unsigned long lastDetectStream = 0;
  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { delay(1); continue; }

    if (millis() - lastDetectStream > 250) {
      lastDetectStream = millis();
      DetectResult res = detectFromFrameBuffer(fb);
      cached_detect_json = detectResultToJson(res);
    }

    client.println("--" + boundary);
    client.println("Content-Type: image/jpeg");
    client.println("Content-Length: " + String(fb->len));
    client.println();
    client.write(fb->buf, fb->len);
    client.println();
    esp_camera_fb_return(fb);
    delay(10);
  }
}

/**************************** Camera Initialization ****************************/
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk  = XCLK_GPIO_NUM;
  config.pin_pclk  = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format  = PIXFORMAT_JPEG;
  config.frame_size    = FRAMESIZE_QVGA;
  config.jpeg_quality  = 25;
  config.fb_count      = 2;

  esp_err_t err = esp_camera_init(&config);
  return (err == ESP_OK);
}

/**************************** Setup ****************************/
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Init servo driver
  Wire.begin(I2C_SDA, I2C_SCL);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  resetAll();

  // Init camera (non-fatal)
  if (initCamera()) {
    cam_ok = true;
    Serial.println("Camera OK");
  } else {
    cam_ok = false;
    Serial.println("Camera FAIL - camera features disabled");
  }

  // WiFi AP
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Server routes
  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/api/detect", handleDetect);
  server.on("/stream", handleStream);
  server.begin();

  randomSeed(analogRead(0));
  Serial.println("RMAGC Server Ready");
}

/**************************** Main Loop ****************************/
void loop() {
  server.handleClient();

  // Web-triggered actions
  if (webAction != MODE_NONE) {
    if (!demoRunning) {
      if (webAction == MODE_COMPETITION) {
        Serial.println("Web: Competition Mode");
        executeCompetitionSequence();
      } else if (webAction == MODE_GAME) {
        Serial.println("Web: Game Mode");
        executeGameMode();
      } else if (webAction == MODE_IDLE) {
        Serial.println("Web: Reset");
        resetAll();
      }
    }
    webAction = MODE_NONE;
  }

  // Serial command interface
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') {} // skip
    else if (cmd == '5') { if (!demoRunning) executeCompetitionSequence(); }
    else if (cmd == '6') { if (!demoRunning) executeGameMode(); }
    else if (cmd == 'h') { resetAll(); }
    else if (cmd == 'p') {
      Serial.print("Joints: ");
      for (int i = 0; i < JOINT_COUNT; i++) {
        Serial.print("CH"); Serial.print(i);
        Serial.print("="); Serial.print(angles[i]);
        Serial.print("° ");
      }
      Serial.print("Slide: ");
      Serial.println(slideState == STOP ? "STOP" : slideState == FORWARD ? "FWD" : "REV");
    }
    else if (!demoRunning) {
      if (cmd == 't') { setSlide(FORWARD); Serial.println("Slide FWD"); }
      else if (cmd == 'g') { setSlide(REVERSE); Serial.println("Slide REV"); }
      else if (cmd == 'b') { setSlide(STOP);    Serial.println("Slide STOP"); }
      else {
        int ch = -1, delta = 0;
        switch (cmd) {
          case 'q': ch = 0; delta =  10; break; case 'w': ch = 0; delta = -10; break;
          case 'a': ch = 1; delta =   5; break; case 's': ch = 1; delta =  -5; break;
          case 'z': ch = 2; delta =   5; break; case 'x': ch = 2; delta =  -5; break;
          case 'e': ch = 3; delta =   5; break; case 'r': ch = 3; delta =  -5; break;
          case 'd': ch = 4; delta =   5; break; case 'f': ch = 4; delta =  -5; break;
          case 'c': ch = 5; delta =   5; break; case 'v': ch = 5; delta =  -5; break;
        }
        if (ch >= 0) { angles[ch] += delta; setJoint(ch, angles[ch]); }
      }
    }
  }

  // Periodic color detection
  if (cam_ok) {
    static unsigned long lastDetect = 0;
    if (millis() - lastDetect > 300) {
      lastDetect = millis();
      DetectResult res = detectColorFromCamera();
      cached_detect_json = detectResultToJson(res);
    }
  }

  delay(2);
}
