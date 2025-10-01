#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

const char* SSID = "WIFI_SSID";
const char* PASS = "WIFI_PASSWORD";
String IP = "WEBSERVER_IP";
String API_URL = IP + "/api/images"; // replace with your API endpoint
bool noImages = false;
String webMsg = "";

TFT_eSPI tft;
String imgs[256];
int found = 0;
int count = 0;
int timesShown = 0;

bool tft_output_cb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bmp) {
  if (x >= tft.width() || y >= tft.height()) return true;
  if (x + w > tft.width())  w = tft.width()  - x;
  if (y + h > tft.height()) h = tft.height() - y;
  tft.pushImage(x, y, w, h, bmp);
  return true;
}

void wipeAllFiles() {
  File root = SPIFFS.open("/");
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    String name = f.name();
    size_t sz = f.size();
    f.close();
    SPIFFS.remove(name);
    Serial.printf("Removed %s (%u bytes)\n", name.c_str(), (unsigned)sz);
  }
}

static String makeTmpPath(int i) {
  return String("/tmp") + i + ".jpg";
}
static String makeUrlFor(const String& name) {
  return String(IP + "/photos/") + name;
}

uint8_t chooseScaleToFill(uint16_t iw, uint16_t ih, uint16_t DW, uint16_t DH) {
  const uint8_t scales[4] = {1, 2, 4, 8};
  for (uint8_t s: scales) if ((iw / s) >= DW && (ih / s) >= DH) return s;
  for (uint8_t s: scales) if ((iw / s) <= DW && (ih / s) <= DH) return s;
  return 8;
}

void drawImageFill_FS(const char* path) {
  uint16_t iw=0, ih=0;
  if (TJpgDec.getJpgSize(&iw, &ih, path) != JDR_OK) return;
  const uint16_t DW = tft.width(), DH = tft.height();
  const uint8_t scale = chooseScaleToFill(iw, ih, DW, DH);
  TJpgDec.setJpgScale(scale);
  const uint16_t sw = iw / scale, sh = ih / scale;
  tft.fillScreen(TFT_BLACK);
  if (sw >= DW && sh >= DH) {
    TJpgDec.drawJpg(-(sw - DW)/2, -(sh - DH)/2, path);
  } else {
    TJpgDec.drawJpg((DW - sw)/2, (DH - sh)/2, path);
  }
}

bool httpBeginSimple(HTTPClient& http, WiFiClient& c, WiFiClientSecure& cs, const String& url) {
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.useHTTP10(true);
  http.setUserAgent("ESP32/ImgScrape");
  return http.begin(c, url);
}

String httpGetText(const String& url) {
  WiFiClient c; WiFiClientSecure cs; HTTPClient http;
  if (!httpBeginSimple(http, c, cs, url)) return "";
  int code = http.GET();
  if (code != HTTP_CODE_OK) { http.end(); return ""; }
  String body = http.getString();
  http.end();
  return body;
}

bool httpDownloadToFile(const String& urlIn, const char* path) {
  String url = urlIn; 
  WiFiClient c; WiFiClientSecure cs; HTTPClient http;
  if (!httpBeginSimple(http, c, cs, url)) { Serial.println("begin fail"); return false; }

  int code = http.GET();
  Serial.printf("GET %s -> %d\n", url.c_str(), code);
  if (code != HTTP_CODE_OK) { http.end(); return false; }

  File f = SPIFFS.open(path, FILE_WRITE);
  if (!f) { Serial.printf("open(%s) fail\n", path); http.end(); return false; }

  WiFiClient* s = http.getStreamPtr();
  uint8_t buf[2048]; size_t total = 0;
  while (http.connected() || s->available()) {
    int n = s->readBytes(buf, sizeof(buf));
    if (n > 0) { f.write(buf, n); total += n; }
    yield();
  }
  f.close(); http.end();

  Serial.printf("Saved %u bytes -> %s\n", (unsigned)total, path);
  if (total == 0) { SPIFFS.remove(path); return false; }
  return true;
}

int extractJsonStrings(const String& s, String out[], int maxOut) {
  int n = 0;
  for (int i = 0; i < s.length() && n < maxOut;) {
    int q1 = s.indexOf('"', i);
    if (q1 < 0) break;
    int q2 = q1 + 1;
    while (q2 < s.length()) {
      if (s[q2] == '\\') { q2 += 2; continue; }
      if (s[q2] == '"') break;
      q2++;
    }
    if (q2 >= s.length()) break;
    String v = s.substring(q1 + 1, q2);
    v.replace("\\\"", "\"");
    v.replace("\\\\", "\\");
    out[n++] = v;
    i = q2 + 1;
  }
  return n;
}

void setup() {
  Serial.begin(115200);
  SPIFFS.begin(true);
  SPIFFS.format();            
  SPIFFS.end();               
  SPIFFS.begin(true);
  tft.init();
  tft.setRotation(2);
  TJpgDec.setCallback(tft_output_cb);
  TJpgDec.setSwapBytes(true);

  // WiFi 
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin(SSID, PASS);
  Serial.print("Connecting WiFi");
  uint32_t tStart = millis();

  // tries for up to 30 seconds)
  while (WiFi.status() != WL_CONNECTED && (millis() - tStart) < 30000) {
    delay(500);
    Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK");
  } else {
    Serial.println(" Failed to connect");
  }

  if (WiFi.status() == WL_CONNECTED) {
  Serial.printf("Connected to %s  IP=%s  RSSI=%d dBm\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
    Serial.println(WiFi.macAddress());
    String html = httpGetText(API_URL);
    found = extractJsonStrings(html, imgs, 256);
    for (int i=0;i<found;i++) Serial.printf("Image[%d]: %s\n", i, imgs[i].c_str());

    Serial.println(found);
    cycleImages();
  } else {
    Serial.println("Couldn't connect to any networks.");
  }
}



void cycleImages() {
    // Remove all prev imgs
    int startingCt = count;
    int idx = min(found, 10);

    for (int i= 0; i <= 9; i++) {
        String path = makeTmpPath(i);
        SPIFFS.remove(path.c_str());
    } // Download new images. Then Loop will continue again.
    for (int i = 0; i <= 9; i++) {
      if (found > 0) {
        int imgNum = (count + i) % found;
        Serial.println(imgNum);
        String url  = makeUrlFor(imgs[(imgNum)]);
        String path = makeTmpPath(i);
        httpDownloadToFile(url, path.c_str());
        if (i == 4) {
            drawImageFill_FS(makeTmpPath(0).c_str());
            getMessage();
        }
      } else {
        Serial.println("No Images found...");
        noImages = true;
        break;
      }
    }

    count = (startingCt + idx) % found;
}

void drawText(String msg) {
    // Execute overlay on display
    tft.setCursor(50, 50); 
    Serial.println(msg.length());
      tft.fillRoundRect(50, 50, 220, 100, 16, tft.color565(24,32,44));
      tft.drawRoundRect(50, 50, 220, 100, 16, tft.color565(70,110,160));
      tft.setTextColor(TFT_WHITE, tft.color565(24,32,44));
      tft.setTextDatum(TL_DATUM);        
      int x = 60, y = 60, step = tft.fontHeight();

      for (uint16_t i = 0; i < msg.length(); i += 33) {
        uint16_t end = (i + 33 < msg.length()) ? (i + 33) : msg.length();
        tft.drawString(msg.substring(i, end), x, y);
        y += step;
      }
}

void getMessage() {
  // check for messages
  String msg = httpGetText(IP + "/api/sendMessage");
  if (msg != "") {
      Serial.println(msg);
      webMsg = msg;
      drawText(msg);
  } else if (webMsg != "") {
      Serial.println("using cached message");
      timesShown += 1;
      Serial.println(webMsg);
      drawText(webMsg);
      if (timesShown >= 10) {
        webMsg = "";
        timesShown = 0;  // Reset counter and remove message after roughly 20 mins on 2 min delay
      }
  } else {
      Serial.println("No message found.");
  }
}

void loop() {
  // ADD TO LOOP 
  if (!noImages) {
    Serial.println("Looping");
    for (int i = 1; i <= 9; i++) { // int = 1 bc it skips tmp0. tmp0 is displayed in cycleImages to fake the wait 
      String imgPath = makeTmpPath(i);
      Serial.println("Displaying Image:" + imgPath);
      drawImageFill_FS(imgPath.c_str());
      getMessage();
      delay(120000); // 2 min delay
    }
    cycleImages();
  }
}

