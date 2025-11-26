/**
 * THOS v2.1
 * Platform: M5Stack Dial
 */

#include <M5Dial.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <vector>
#include "time.h"

// --- USER CONFIG ---
const char* WIFI_SSID = "SSID_NAME";     
const char* WIFI_PASS = "SSID_PASSWORD"; 
#define TIME_ZONE_OFFSET_HRS  +3  // UTC OFFSET

// --- SYSTEM CONFIG ---
#define SCREEN_W 240
#define SCREEN_H 240
#define CENTER   120
#define SCREENSAVER_TIMEOUT 20000 
#define SCREEN_ROTATION 2     // 0=Normal, 1=90, 2=180, 3=270

// --- INDUSTRIAL PALETTE ---
#define C_BG        0x0000  // Pure Black
#define C_FG        0xFFFF  // Stark White
#define C_ACCENT    0xFB20  // M5Stack Plastic Orange (Vibrant)
#define C_DIM       0x2124  // Dark Grey (Subtle tracks)
#define C_DARK      0x0000  // Black

// --- BRIGHTNESS ---
#define BRIGHT_ACTIVE 200
#define BRIGHT_AOD    10    // Dim for AOD

// --- OBJECTS ---
M5Canvas canvas(&M5Dial.Display);

// --- STATE ---
long encoderAccumulator = 0;
int currentAppIndex = 0;
bool inApp = false;
unsigned long lastInputTime = 0;
bool isScreensaver = false;
bool is24hFormat = true; 

struct AlarmState {
    int hour = 7;
    int minute = 0;
    bool enabled = false;
    bool ringing = false;
    bool snooze = false;
} globalAlarm;

// --- UTILITIES ---

void wakeUp() {
    lastInputTime = millis();
    if (isScreensaver) {
        isScreensaver = false;
        M5Dial.Display.setBrightness(BRIGHT_ACTIVE);
    }
}

// Helper: True Solid Arc (Gauge Style)
void drawGauge(float percent, uint16_t colorActive, uint16_t colorBg) {
    int r = 119;
    int th = 12; // Thickness
    
    // 1. Draw Background Track (Solid Arc)
    canvas.fillArc(CENTER, CENTER, r, r - th, 0, 360, colorBg);
    
    // 2. Draw Active Segment
    if (percent > 0.0) {
        int endAngle = percent * 360;
        if (endAngle > 360) endAngle = 360;
        canvas.fillArc(CENTER, CENTER, r, r - th, 0, endAngle, colorActive);
    }
}

// Shared UI for Ringing (Alarm & Timer)
void drawAlertScreen() {
    canvas.fillSprite(C_BG);
    drawGauge(1.0, C_ACCENT, C_DIM);
    
    canvas.setTextDatum(middle_center);
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextColor(C_FG);
    canvas.drawString("TAP TO STOP", CENTER, CENTER);
}

// ==========================================
// APP INTERFACE
// ==========================================
class App {
public:
    virtual void setup() {}      
    virtual void loop() {}       
    virtual void background() {} 
    virtual void draw() {}       
    virtual void onRotary(int delta) {}
    virtual void onTouch(int x, int y) {}
    virtual String getName() { return "APP"; }
    virtual bool isRunning() { return false; }
    virtual ~App() {}
};

// ==========================================
// 1. WATCH (Smart Redraw)
// ==========================================
class WatchApp : public App {
    unsigned long lastDraw = 0;
public:
    String getName() override { return "CLOCK"; }

    void setup() override {
        lastDraw = 0; // Force immediate redraw on entry
    }

    void draw() override {
        if (lastDraw != 0 && millis() - lastDraw < 200) return;
        lastDraw = millis();

        canvas.fillSprite(C_BG);
        auto t = M5Dial.Rtc.getTime();
        auto d = M5Dial.Rtc.getDate();

        canvas.drawCircle(CENTER, CENTER, 119, C_DIM);

        canvas.setTextDatum(middle_center);
        canvas.setFont(&fonts::FreeSansBold24pt7b); 
        canvas.setTextColor(C_FG);
        
        int displayHour = t.hours;
        if (!is24hFormat) {
            displayHour = displayHour % 12;
            if (displayHour == 0) displayHour = 12;
        }

        char timeBuf[10];
        sprintf(timeBuf, "%02d:%02d", displayHour, t.minutes);
        canvas.drawString(timeBuf, CENTER, CENTER - 15); 

        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(C_ACCENT);
        const char* months[] = {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
        int mIdx = (d.month > 0 && d.month <= 12) ? d.month - 1 : 0;
        char dateBuf[30];
        sprintf(dateBuf, "%s %02d", months[mIdx], d.date);
        canvas.drawString(dateBuf, CENTER, CENTER + 30);

        if (!is24hFormat) {
             String ampm = (t.hours >= 12) ? "PM" : "AM";
             canvas.setFont(&fonts::FreeSansBold9pt7b);
             canvas.setTextColor(C_DIM); 
             canvas.drawString(ampm, CENTER, CENTER + 55); 
        }
    }
};

// ==========================================
// 2. TIMER (Global Interrupt)
// ==========================================
class TimerApp : public App {
    long setTimeSec = 60;
    long startTime = 0;
    bool running = false;
    bool finished = false;
    unsigned long lastTone = 0;
public:
    String getName() override { return "TIMER"; }
    bool isRunning() override { return running || finished; }
    bool isRinging() { return finished; } 
    
    void setup() override { finished = false; }
    
    void background() override {
        if (running) {
            long elapsed = (millis() - startTime) / 1000;
            if (setTimeSec - elapsed <= 0) {
                running = false;
                finished = true;
                wakeUp(); 
            }
        }
    }
    
    void onRotary(int delta) override {
        if (running) return;
        int inc = (abs(delta) > 1) ? 60 : 10; 
        setTimeSec += (delta * inc);
        if (setTimeSec < 10) setTimeSec = 10;
        if (setTimeSec > 3599) setTimeSec = 3599; 
    }

    void onTouch(int x, int y) override {
        if (finished) { 
            finished = false; 
            running = false; 
            M5Dial.Speaker.tone(2000, 50); 
            return; 
        }
        if (!running) {
            startTime = millis(); running = true; M5Dial.Speaker.tone(4000, 50);
        } else {
            running = false; 
            long elapsed = (millis() - startTime) / 1000;
            setTimeSec -= elapsed;
            if (setTimeSec < 0) setTimeSec = 0;
        }
    }

    void draw() override {
        if (finished) {
            drawAlertScreen();
            if (millis() - lastTone > 500) {
                M5Dial.Speaker.tone(4000, 200);
                lastTone = millis();
            }
            return;
        }

        canvas.fillSprite(C_BG);
        long currentVal = setTimeSec;
        if (running) {
            long elapsed = (millis() - startTime) / 1000;
            currentVal = setTimeSec - elapsed;
            if (currentVal <= 0) currentVal = 0;
        }

        float total = (float)(running ? (setTimeSec + ((millis()-startTime)/1000)) : setTimeSec);
        if (total == 0) total = 1;
        float progress = (float)currentVal / total;
        if (!running && !finished) progress = 1.0;
        
        drawGauge(progress, C_ACCENT, C_DIM);

        canvas.setTextDatum(middle_center);
        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextColor(finished ? C_ACCENT : C_FG);
        char buf[20];
        sprintf(buf, "%02ld:%02ld", currentVal / 60, currentVal % 60);
        canvas.drawString(buf, CENTER, CENTER);
        
        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(C_DIM);
        canvas.drawString(running ? "RUNNING" : "SET", CENTER, CENTER + 45);
    }
};

// ==========================================
// 3. ALARM (Optimized Polling)
// ==========================================
class AlarmApp : public App {
    int editState = 0; // 0=View, 1=Hour, 2=Min
    unsigned long lastTone = 0; 
    unsigned long lastCheck = 0;
public:
    String getName() override { return "ALARM"; }

    void background() override {
        if (millis() - lastCheck < 500) return;
        lastCheck = millis();

        if (globalAlarm.enabled && !globalAlarm.ringing && !globalAlarm.snooze) {
            auto t = M5Dial.Rtc.getTime();
            if (t.hours == globalAlarm.hour && t.minutes == globalAlarm.minute && t.seconds == 0) {
                globalAlarm.ringing = true;
                wakeUp(); 
            }
        }
        if (globalAlarm.snooze) {
             auto t = M5Dial.Rtc.getTime();
             if (t.minutes != globalAlarm.minute) globalAlarm.snooze = false; 
        }
    }

    void onRotary(int delta) override {
        if (globalAlarm.ringing) return;
        if (editState == 0) {
            if (delta != 0) {
                globalAlarm.enabled = !globalAlarm.enabled; 
                M5Dial.Speaker.tone(globalAlarm.enabled ? 5000 : 3000, 20); 
            }
        } else if (editState == 1) {
            globalAlarm.hour += delta;
            if (globalAlarm.hour > 23) globalAlarm.hour = 0; else if (globalAlarm.hour < 0) globalAlarm.hour = 23;
        } else if (editState == 2) {
            globalAlarm.minute += delta;
            if (globalAlarm.minute > 59) globalAlarm.minute = 0; else if (globalAlarm.minute < 0) globalAlarm.minute = 59;
        }
    }

    void onTouch(int x, int y) override {
        if (globalAlarm.ringing) {
            globalAlarm.ringing = false; globalAlarm.snooze = true; 
            M5Dial.Speaker.tone(2000, 50); 
            return; 
        }
        editState++; if (editState > 2) editState = 0;
    }

    void draw() override {
        if (globalAlarm.ringing) {
             drawAlertScreen();
             if (millis() - lastTone > 500) {
                 M5Dial.Speaker.tone(4000, 200); 
                 lastTone = millis();
             }
             return;
        }

        canvas.fillSprite(C_BG);
        
        uint16_t ringColor = globalAlarm.enabled ? C_ACCENT : C_DIM;
        drawGauge(1.0, ringColor, C_DIM);

        canvas.setTextDatum(middle_center);
        canvas.setFont(&fonts::FreeSansBold24pt7b);
        
        canvas.setTextColor((editState == 1) ? C_ACCENT : C_FG);
        int displayHour = globalAlarm.hour;
        String ampm = "";
        
        if (!is24hFormat) {
            ampm = (displayHour >= 12) ? "PM" : "AM";
            displayHour = displayHour % 12;
            if (displayHour == 0) displayHour = 12;
        }
        
        canvas.drawString(String(displayHour), 70, CENTER);
        canvas.setTextColor(C_FG);
        canvas.drawString(":", CENTER, CENTER - 5);
        canvas.setTextColor((editState == 2) ? C_ACCENT : C_FG);
        String m = String(globalAlarm.minute);
        if (globalAlarm.minute < 10) m = "0" + m;
        canvas.drawString(m, 170, CENTER);
        
        canvas.setFont(&fonts::FreeSansBold9pt7b);
        if (!is24hFormat) {
            canvas.setTextColor(C_DIM);
            canvas.drawString(ampm, CENTER, 150); 
        }

        canvas.setTextColor(ringColor);
        String lbl = globalAlarm.enabled ? "ON" : "OFF";
        if (editState == 1) lbl = "SET HR";
        if (editState == 2) lbl = "SET MIN";
        canvas.drawString(lbl, CENTER, 180);
    }
};

// ==========================================
// 4. SETTINGS
// ==========================================
class SettingsApp : public App {
    int mode = 0; 
    String msg = "";
public:
    String getName() override { return "SETUP"; }
    
    void setup() override { mode = 0; msg = ""; }

    void onTouch(int x, int y) override {
        if (y < 120) {
            // SYNC
            mode = 1;
            draw(); canvas.pushSprite(0,0); 
            if (performSync()) { msg = "OK"; M5Dial.Speaker.tone(6000, 100); } 
            else { msg = "FAIL"; M5Dial.Speaker.tone(200, 500); }
            mode = 0;
        } else {
            is24hFormat = !is24hFormat;
            M5Dial.Speaker.tone(4000, 50);
        }
    }

    bool performSync() {
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        int t = 0;
        while (WiFi.status() != WL_CONNECTED) {
            delay(500); t++;
            if (t > 15) { WiFi.disconnect(true); return false; } 
        }
        configTime(TIME_ZONE_OFFSET_HRS * 3600, 0, "pool.ntp.org");
        struct tm timeinfo;
        t = 0;
        while(!getLocalTime(&timeinfo)) {
            delay(100); t++;
            if (t > 40) { WiFi.disconnect(true); return false; } 
        }
        m5::rtc_time_t rt;
        rt.hours = timeinfo.tm_hour;
        rt.minutes = timeinfo.tm_min;
        rt.seconds = timeinfo.tm_sec;
        M5Dial.Rtc.setTime(rt);
        m5::rtc_date_t rd;
        rd.year = timeinfo.tm_year + 1900;
        rd.month = timeinfo.tm_mon + 1;
        rd.date = timeinfo.tm_mday;
        M5Dial.Rtc.setDate(rd);
        WiFi.disconnect(true);
        return true;
    }

    void draw() override {
        canvas.fillSprite(C_BG);
        canvas.setTextDatum(middle_center);
        
        // --- BUTTON 1: SYNC ---
        canvas.drawLine(40, 120, 200, 120, C_DIM);

        canvas.setFont(&fonts::FreeSansBold12pt7b);
        if (mode == 1) {
            canvas.setTextColor(C_ACCENT);
            canvas.drawString("SYNCING...", CENTER, 80);
        } else {
            canvas.setTextColor(C_FG);
            canvas.drawString("SYNC TIME", CENTER, 70);
            if (msg != "") {
                canvas.setFont(&fonts::FreeSans9pt7b);
                canvas.setTextColor((msg=="OK")?C_ACCENT:C_DIM);
                canvas.drawString(msg, CENTER, 95);
            }
        }

        // --- BUTTON 2: FORMAT ---
        canvas.setFont(&fonts::FreeSansBold12pt7b);
        canvas.setTextColor(C_FG);
        canvas.drawString(is24hFormat ? "24H MODE" : "12H MODE", CENTER, 150);
        
        canvas.fillCircle(CENTER, 180, 6, is24hFormat ? C_ACCENT : C_DIM);
    }
};

// ==========================================
// SYSTEM KERNEL
// ==========================================
std::vector<App*> apps;
WatchApp watchApp;
TimerApp timerApp;
AlarmApp alarmApp;
SettingsApp settingsApp;

void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, true);
    
    M5Dial.Display.setRotation(SCREEN_ROTATION);

    M5Dial.Display.setBrightness(BRIGHT_ACTIVE);
    canvas.createSprite(SCREEN_W, SCREEN_H);
    
    apps.push_back(&watchApp);
    apps.push_back(&timerApp);
    apps.push_back(&alarmApp);
    apps.push_back(&settingsApp);
    
    wakeUp();
}

void drawIcon(int index, int x, int y, int size, bool active) {
    uint16_t c = active ? C_ACCENT : C_DIM;
    if (index == 0) {
        canvas.fillCircle(x, y, size/2, c);
        if (active) {
            canvas.drawLine(x, y, x, y-(size/3), C_BG);
            canvas.drawLine(x, y, x+(size/4), y, C_BG);
        }
    }
    else if (index == 1) {
        int r = size/2;
        canvas.fillTriangle(x-r/2, y-r, x-r/2, y+r, x+r, y, c);
    }
    else if (index == 2) {
        int r = size/2;
        canvas.fillArc(x, y+4, r, 0, 180, 360, c);
        canvas.fillRect(x-r, y+4, size, 4, c);
        if (active) canvas.fillCircle(x, y+12, 4, c);
    }
    else if (index == 3) {
        int w = size;
        int h = 6; 
        int gap = 12;
        canvas.fillRect(x-w/2, y-gap, w, h, c);
        canvas.fillRect(x-w/2, y, w, h, c);
        canvas.fillRect(x-w/2, y+gap, w, h, c);
    }
}

void drawMenu() {
    canvas.fillSprite(C_BG);
    int prev = (currentAppIndex - 1 + apps.size()) % apps.size();
    int next = (currentAppIndex + 1) % apps.size();
    
    drawIcon(prev, 50, 120, 30, false);
    drawIcon(next, 190, 120, 30, false);
    drawIcon(currentAppIndex, CENTER, 110, 60, true);
    
    canvas.setTextDatum(middle_center);
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextColor(C_FG);
    canvas.drawString(apps[currentAppIndex]->getName(), CENTER, 180);
    
    canvas.fillCircle(CENTER, 215, 4, C_ACCENT);
}

void loop() {
    M5Dial.update();
    
    // 1. BACKGROUND TASKS
    for(auto* app : apps) app->background();

    // 2. Rotary Debounce
    long newPos = M5Dial.Encoder.read();
    long rawDelta = newPos - encoderAccumulator;
    int uiDelta = 0;
    if (abs(rawDelta) >= 4) {
        uiDelta = (rawDelta > 0) ? 1 : -1;
        encoderAccumulator = newPos;
    } else {
        encoderAccumulator = newPos - rawDelta;
    }

    auto t = M5Dial.Touch.getDetail();
    if (uiDelta != 0 || t.wasPressed() || M5Dial.BtnA.wasPressed()) wakeUp();

    // 3. PRIORITY INTERRUPTS (Alarm & Timer)
    if (globalAlarm.ringing) {
        alarmApp.draw();
        canvas.pushSprite(0,0);
        if (t.wasPressed()) alarmApp.onTouch(0,0);
        return;
    }
    if (timerApp.isRinging()) {
        timerApp.draw();
        canvas.pushSprite(0,0);
        if (t.wasPressed()) timerApp.onTouch(0,0);
        return;
    }

    // 4. AOD Logic
    bool preventSleep = (inApp && apps[currentAppIndex]->isRunning());
    if (!preventSleep && !isScreensaver && (millis() - lastInputTime > SCREENSAVER_TIMEOUT)) {
        isScreensaver = true;
        M5Dial.Display.setBrightness(BRIGHT_AOD);
    }
    
    if (isScreensaver) {
        watchApp.draw(); 
        canvas.pushSprite(0,0);
        return; 
    }
    
    // 5. App Logic
    if (inApp) {
        if (M5Dial.BtnA.wasPressed()) {
            inApp = false; 
            M5Dial.Speaker.tone(2000, 20);
        } else {
            apps[currentAppIndex]->onRotary(uiDelta); 
            if (t.wasPressed()) apps[currentAppIndex]->onTouch(t.x, t.y);
            apps[currentAppIndex]->draw();
        }
    } else {
        if (uiDelta != 0) {
            currentAppIndex += uiDelta;
            if (currentAppIndex < 0) currentAppIndex = apps.size() - 1;
            if (currentAppIndex >= apps.size()) currentAppIndex = 0;
            M5Dial.Speaker.tone(4000, 10);
        }
        drawMenu();
        if (t.wasPressed() || M5Dial.BtnA.wasPressed()) {
            inApp = true;
            apps[currentAppIndex]->setup();
            M5Dial.Speaker.tone(5000, 20);
        }
    }
    canvas.pushSprite(0,0);
}
