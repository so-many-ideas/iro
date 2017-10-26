#include "./ringIroMode.h"
#include "./iroModesManager.h"

void RingIroMode::animate(Adafruit_NeoPixel *pixels) {
  this->currentForegroundColor = lerpColor(this->currentForegroundColor, this->targetForegroundColor);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels->setPixelColor(i, pixels->Color(this->currentForegroundColor.r, this->currentForegroundColor.g, this->currentForegroundColor.b));
  }
  pixels->show();
}

RingIroMode::RingIroMode(IroModesManager *manager) {
  this->manager = manager;
  this->server = manager->server;
  this->manager->registerMode(this);
  this->server->on("/ring", [&]() {
    this->server->sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    this->server->sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    Color fc = {.r = 0, .g = 0, .b = 0};
    int value = 0;
    int canChangeMode = this->server->args() == 1 &&
                        this->server->argName(0) == "foreground" &&
                        sscanf(this->server->arg(0).c_str(), "r%dg%db%d", &fc.r, &fc.g, &fc.b) == 3 &&
                        checkColor(fc);
    if (canChangeMode) {
      this->manager->switchToMode(this);
      this->targetForegroundColor = fc;
      String response = String("{foreground: {r:") + fc.r + ",g:" + fc.g + ",b:" + fc.b + "}}";
      this->server->send(200, "application/json", response);
    } else {
      this->server->send(400, "application/json", String("{error: \"wrong parameters\", expected:\"r0-255g0-255b0-255\", received:\"") + this->server->arg(0) + "\", decoded:\"r:" + fc.r + ",g:" + fc.g + ",b:" + fc.b + "\"}");
    }
  });
}