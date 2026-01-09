//SMART CLOCK BY @HDZ463


// thư viện cần thiết :
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "GD.h"
// --- Cấu hình Wifi ---
const char* ssid     = "TEN_WIFI";
const char* password = "MAT_KHAU_WIFI";

// --- Cấu hình API Thời tiết ---
String apiKey = "API CUA BAN";
String city   = "Hanoi"; // Địa chỉ (VÍ DỰ : HANOI)
String countryCode = "VN"; // Quốc gia bạn muốn do

TFT_eSPI tft = TFT_eSPI();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600); // Múi giờ VN là +7
#define CYD_BACKLIGHT_PIN 21

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo màn hình
  tft.init();
  tft.setRotation(2); // Chế độ ngang
  tft.fillScreen(TFT_BLACK);
  pinMode(CYD_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(CYD_BACKLIGHT_PIN, HIGH);
  
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Dang ket noi Wifi...", 10, 10, 2);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  tft.fillScreen(TFT_BLACK);
	digitalWrite(CYD_BACKLIGHT_PIN, LOW);
  tft.drawBitmap(0, 0, UI_bitmap, 320, 240, TFT_WHITE);
  timeClient.begin();
  drawInterface(); // Vẽ khung giao diện cố định
	tft.setSwapBytes(true);
	tft.pushImage(0, 0, 320, 240, NEN_bitmap);

}

void loop() {
  timeClient.update();
  updateTimeDisplay();
  
  // Cập nhật thời tiết mỗi 15 phút
  static unsigned long lastWeatherUpdate = 0;
  if (millis() - lastWeatherUpdate > 900000 || lastWeatherUpdate == 0) {
		tft.drawBitmap(0, 0, UI_bitmap, 320, 240, TFT_BLACK);
		tft.drawBitmap(0, 0, KHUNG_bitmap, 320, 240, TFT_WHITE);
    updateWeather();
    lastWeatherUpdate = millis();
  }
  
  delay(1000); 
	digitalWrite(CYD_BACKLIGHT_PIN, HIGH);
}

void drawInterface() {
  tft.drawBitmap(0, 0, UI_bitmap, 320, 240, TFT_BLACK);
	tft.drawBitmap(0, 0, KHUNG_bitmap, 320, 240, TFT_WHITE);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Temperature", 20, 120, 2);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("Air quality", 200, 120, 2);
}

void updateTimeDisplay() {
	tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Temperature", 20, 120, 2);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("Air quality", 200, 120, 2);
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  
  // 1. Hiển thị giờ lớn
  String fTime = timeClient.getFormattedTime();
  tft.drawCentreString(fTime, 160, 25, 7); 

  // 2. Lấy dữ liệu ngày tháng từ NTP
  time_t rawTime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime(&rawTime);

  // Mảng lưu tên các thứ và tháng (để hiển thị đẹp hơn)
  const char* daysOfWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  // Định dạng chuỗi: Thứ, Ngày Tháng Năm
  char dateBuf[30];
  sprintf(dateBuf, "%s, %02d %s %d", 
          daysOfWeek[ti->tm_wday], 
          ti->tm_mday, 
          months[ti->tm_mon], 
          ti->tm_year + 1900);

  // 3. Hiển thị lên màn hình
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillRect(40, 70, 250, 20, TFT_BLACK); // Xóa vùng chữ cũ để tránh đè chữ
  tft.drawCentreString(dateBuf, 160, 70, 2);
}

void updateWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // 1. Lấy Thời tiết (Sửa lỗi HTTP_OK thành 200)
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&units=metric&appid=" + apiKey;
    http.begin(url);
    
    if (http.GET() == 200) { // Đã sửa lỗi ở đây
      DynamicJsonDocument doc(1536);
      deserializeJson(doc, http.getString());
      float temp = doc["main"]["temp"];
      float lat = doc["coord"]["lat"];
      float lon = doc["coord"]["lon"];

      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextPadding(90); // Tránh lem chữ khi số thay đổi
      tft.drawCentreString(String(temp, 1) + " C", 60, 160, 4);

      // 2. Lấy AQI thực tế
      String aqiUrl = "http://api.openweathermap.org/data/2.5/air_pollution?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + apiKey;
      http.begin(aqiUrl);
      if (http.GET() == 200) {
        deserializeJson(doc, http.getString());
        int aqi = doc["list"][0]["main"]["aqi"];
        
        uint16_t color = TFT_GREEN;
        String label = "GOOD";
        if(aqi == 2) { color = TFT_YELLOW; label = "FAIR"; }
        else if(aqi == 3) { color = TFT_ORANGE; label = "MODER"; }
        else if(aqi >= 4) { color = TFT_RED; label = "POOR"; }

        tft.setTextColor(color, TFT_BLACK);
        tft.drawCentreString(label, 240, 160, 4);
      }
    }
    http.end();
  }
}
