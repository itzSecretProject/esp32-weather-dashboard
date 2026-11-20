// Desktop preview: renders the dashboard with the same code as the firmware and saves a PNG.
//
//   preview --weather fixtures/paris.json --cities fixtures/paris-cities.json --out paris.png
//
// Run `preview --help` for all options.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "../../src/ui/dashboard.h"
#include "../../src/weather/weather_parse.h"

static void usage() {
  std::cout << "Usage: preview --weather <file.json> [options]\n\n"
               "  --weather <file>    Open-Meteo forecast response (required unless --message)\n"
               "  --cities <file>     Open-Meteo multi-location response for the map markers\n"
               "  --out <file.png>    output image (default: preview.png)\n"
               "  --location <name>   main city label        (default: \"Paris, FR\")\n"
               "  --city2 <name>      second map city label  (default: \"London, UK\")\n"
               "  --city3 <name>      third map city label   (default: \"Madrid, ES\")\n"
               "  --now <epoch>       render at this UTC time (default: time of the data)\n"
               "  --imperial          label units as \xC2\xB0""F / mph / mi\n"
               "  --12h               12-hour clock\n"
               "  --message <title> <detail>   render the status screen instead\n";
}

static bool readJson(const std::string& path, JsonDocument& doc) {
  std::ifstream in(path);
  if (!in) {
    std::cerr << "error: cannot open " << path << "\n";
    return false;
  }
  std::stringstream ss;
  ss << in.rdbuf();
  if (deserializeJson(doc, ss.str())) {
    std::cerr << "error: " << path << " is not valid JSON\n";
    return false;
  }
  return true;
}

int main(int argc, char** argv) {
  std::string weatherPath, citiesPath, out = "preview.png", msgTitle, msgDetail;
  std::string location = "Paris, FR", city2 = "London, UK", city3 = "Madrid, ES";
  long long now = 0;
  bool imperial = false, h12 = false, message = false;

  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "error: " << a << " needs a value\n";
        std::exit(2);
      }
      return argv[++i];
    };
    if (a == "--weather") weatherPath = next();
    else if (a == "--cities") citiesPath = next();
    else if (a == "--out") out = next();
    else if (a == "--location") location = next();
    else if (a == "--city2") city2 = next();
    else if (a == "--city3") city3 = next();
    else if (a == "--now") now = std::atoll(next().c_str());
    else if (a == "--imperial") imperial = true;
    else if (a == "--12h") h12 = true;
    else if (a == "--message") { message = true; msgTitle = next(); msgDetail = next(); }
    else if (a == "--help" || a == "-h") { usage(); return 0; }
    else { std::cerr << "error: unknown option " << a << "\n\n"; usage(); return 2; }
  }
  if (!message && weatherPath.empty()) { usage(); return 2; }

  LGFX_Sprite screen;
  screen.setColorDepth(16);  // same colour depth as the real panel
  screen.createSprite(480, 320);

  if (message) {
    uiDrawMessage(screen, msgTitle.c_str(), msgDetail.c_str());
  } else {
    JsonDocument doc;
    WeatherData w;
    if (!readJson(weatherPath, doc)) return 1;
    if (!parseWeather(doc, w)) { std::cerr << "error: unexpected forecast format\n"; return 1; }
    if (!citiesPath.empty()) {
      JsonDocument cdoc;
      if (!readJson(citiesPath, cdoc)) return 1;
      parseCities(cdoc, w.mapCities, 2);
    }
    if (now > 0) w.updated = (time_t)now;

    UiState st;
    st.now         = now > 0 ? (time_t)now : w.updated;
    st.location    = location.c_str();
    st.mapNames[0] = city2.c_str();
    st.mapNames[1] = city3.c_str();
    st.metric      = !imperial;
    st.clock24     = !h12;
    uiDrawDashboard(screen, w, st);
  }

  size_t len = 0;
  void* png = screen.createPng(&len, 0, 0, screen.width(), screen.height());
  if (!png) { std::cerr << "error: PNG encoding failed\n"; return 1; }
  FILE* f = std::fopen(out.c_str(), "wb");
  if (!f) { std::cerr << "error: cannot write " << out << "\n"; std::free(png); return 1; }
  std::fwrite(png, 1, len, f);
  std::fclose(f);
  std::free(png);
  std::cout << "wrote " << out << "\n";
  return 0;
}
