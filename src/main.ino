#include "Arduino.h"
#include "TFT_eSPI.h"
#include "pin_config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "lots.h" // this file is auto-generated at build time from lots.yaml

#include <vector>
#include <map>

// first, make sure you have lots.yaml in the root directory with the lots of your stocks and their prices #EDITME
const char *defaultCurrency = "DKK"; // EDITME if you want to use a different currency
const char *ssid = "";  // EDITME put your wifi network name
const char *password = "";  // EDITME put your wifi password

const float ALERT_PERCENTAGE_LOSS = 0.05; // EDITME OPTIONAL 

// NTP Server settings
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

TFT_eSPI tft = TFT_eSPI();
// OneButton button1(PIN_BUTTON_1, true);

struct StockLot
{
    String symbol;
    float quantity;
    float unit_cost;
    String currency;
    float current_price;
    float total_value_dkk;
    float profit_dkk;
    float daily_profit_dkk;
};

std::vector<StockLot> stockLots;
std::map<String, float> exchangeRates;
float totalPortfolioValueDKK = 0.0;
float totalProfitDKK = 0.0;
float totalDailyProfitDKK = 0.0;

float getExchangeRate(String currency)
{
  if (currency == defaultCurrency)
    return 1.0f;
  if (exchangeRates.find(currency) != exchangeRates.end())
  {
    return exchangeRates[currency];
    }

    HTTPClient http;
    String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + currency + defaultCurrency +"=X?interval=1d&range=1d";
    http.begin(url);
    http.addHeader("User-Agent", "Mozilla/5.0");

    float rate = 1.0f;
    if (http.GET() == HTTP_CODE_OK)
    {
        String payload = http.getString();
        JsonDocument doc;

        if (!deserializeJson(doc, payload))
        {
            rate = doc["chart"]["result"][0]["meta"]["regularMarketPrice"].as<float>();
        }
    }
    http.end();

    exchangeRates[currency] = rate;
    return rate;
}

void setup()
{
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);

    Serial.begin(115200);

    tft.begin();

    tft.println("Connecting to WiFi...");
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password, 6);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }
    Serial.println(" Connected!");
    tft.println("WiFi connected.");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    tft.fillScreen(TFT_BLACK);

    tft.setRotation(3);
    tft.setSwapBytes(true);
    parseYAML();
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        totalPortfolioValueDKK = 0.0;
        totalProfitDKK = 0.0;

        // get all required exchange rates
        for (auto &lot : stockLots)
        {
            getExchangeRate(lot.currency);
        }

        // Then calculate values
        for (auto &lot : stockLots)
        {
            fetchStockPrice(lot);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString(String(lot.symbol), 240, 140, 2);

            // print stock data via serial for debugging
            // Serial.print("Symbol: ");
            // Serial.print(lot.symbol);
            totalPortfolioValueDKK += lot.total_value_dkk;
            totalProfitDKK += lot.profit_dkk;
            totalDailyProfitDKK += lot.daily_profit_dkk;
        }

        // pulsate red if loss is more than ALERT_PERCENTAGE_LOSS 
        if (totalDailyProfitDKK < 0 && abs(totalDailyProfitDKK) > totalPortfolioValueDKK * ALERT_PERCENTAGE_LOSS)
        {
            tft.fillScreen(TFT_RED);
            for (int i = 0; i < 10; i++)
            {
                tft.fillScreen(TFT_RED);
                delay(100);
                tft.fillScreen(TFT_BLACK);
                delay(100);
            }
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }

        displayStockData();
        
        // reset variables for next loop
        totalPortfolioValueDKK = 0.0;
        totalProfitDKK = 0.0;
        totalDailyProfitDKK = 0.0;
    }
    delay(60000);
}

void parseYAML()
{
  String yaml = LOTS_YAML;
  int start = yaml.indexOf("lots:");
  while (true)
  {
    int symbolStart = yaml.indexOf("symbol:", start);
    if (symbolStart == -1)
      break;

    int symbolEnd = yaml.indexOf("\n", symbolStart);
    String symbol = yaml.substring(symbolStart + 9, symbolEnd - 1);

    int quantityStart = yaml.indexOf("quantity:", symbolEnd);
    int quantityEnd = yaml.indexOf("\n", quantityStart);
    float quantity = yaml.substring(quantityStart + 10, quantityEnd).toFloat();

    int costStart = yaml.indexOf("unit_cost:", quantityEnd);
    int costEnd = yaml.indexOf("\n", costStart);
    float unit_cost = yaml.substring(costStart + 11, costEnd).toFloat();

    int currencyStart = yaml.indexOf("currency:", costEnd);
    String currency = "DKK";   // Default currency
    int currencyEnd = costEnd; // Initialize with fallback position

    if (currencyStart != -1)
    {
      currencyEnd = yaml.indexOf("\n", currencyStart);
      currency = yaml.substring(currencyStart + 9, currencyEnd);
      currency.trim();
    }

    stockLots.push_back({symbol, quantity, unit_cost, currency, 0.0, 0.0, 0.0});
    start = currencyEnd;
    }
}

void fetchStockPrice(StockLot &lot)
{
    HTTPClient http;
    String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + lot.symbol + "?interval=1d&range=1d";
    http.begin(url);
    http.addHeader("User-Agent", "Mozilla/5.0");

    if (http.GET() == HTTP_CODE_OK)
    {
        String payload = http.getString();
        JsonDocument doc;
        if (!deserializeJson(doc, payload))
        {
            float price = doc["chart"]["result"][0]["meta"]["regularMarketPrice"].as<float>();
            float previousClose = doc["chart"]["result"][0]["meta"]["chartPreviousClose"].as<float>();

            float exchangeRate = getExchangeRate(lot.currency);

            lot.current_price = price;
            lot.total_value_dkk = lot.quantity * price * exchangeRate;
            lot.profit_dkk = (price * exchangeRate - lot.unit_cost * exchangeRate) * lot.quantity;
            lot.daily_profit_dkk = (price - previousClose) * lot.quantity * exchangeRate;
        }
    }
    http.end();
}

void displayStockData()
{
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 10);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(7);
    tft.printf("%.0f", totalPortfolioValueDKK);
    tft.setTextSize(1);
    tft.print("total");

    tft.setTextSize(7);
    tft.print("\n");

    tft.setTextSize(6);
    if (totalProfitDKK >= 0)
    {
        tft.setTextColor(TFT_GREEN);
        tft.printf("+%.0f", totalProfitDKK);
        tft.setTextSize(1);
        tft.print(" gain");
    }
    else
    {
        tft.setTextColor(tft.color565(255, 43, 131));
        tft.printf("%.0f", totalProfitDKK);
        tft.setTextSize(1);
        tft.print(" loss");
    }
    tft.setTextSize(6);
    tft.print("\n");

    tft.setTextSize(5);
    if (totalDailyProfitDKK >= 0)
    {
        tft.setTextColor(TFT_GREEN);
        tft.printf("+%.0f", totalDailyProfitDKK);
    }
    else
    {
        tft.setTextColor(tft.color565(255, 43, 131));
        tft.printf("%.0f", totalDailyProfitDKK);
    }
    tft.setTextSize(1);
    tft.print(" day");

    // show last updated time
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        tft.setTextSize(1);
        tft.setCursor(260, 160);
        tft.setTextColor(TFT_WHITE);
        tft.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
}

// TFT Pin check
#if PIN_LCD_WR  != TFT_WR || \
    PIN_LCD_RD  != TFT_RD || \
    PIN_LCD_CS    != TFT_CS   || \
    PIN_LCD_DC    != TFT_DC   || \
    PIN_LCD_RES   != TFT_RST  || \
    PIN_LCD_D0   != TFT_D0  || \
    PIN_LCD_D1   != TFT_D1  || \
    PIN_LCD_D2   != TFT_D2  || \
    PIN_LCD_D3   != TFT_D3  || \
    PIN_LCD_D4   != TFT_D4  || \
    PIN_LCD_D5   != TFT_D5  || \
    PIN_LCD_D6   != TFT_D6  || \
    PIN_LCD_D7   != TFT_D7  || \
    PIN_LCD_BL   != TFT_BL  || \
    TFT_BACKLIGHT_ON   != HIGH  || \
    170   != TFT_WIDTH  || \
    320   != TFT_HEIGHT
// #error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
// #error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
// #error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
// #error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#error  "The current version is not supported for the time being, please use a version below Arduino ESP32 3.0"
#endif