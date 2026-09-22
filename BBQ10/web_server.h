// Local transfer mode: a local web server for browsing/downloading notes
// over the LAN, as an alternative/complement to the cloud Whisper sync.
// Only usable while connected to WiFi (see whisperPoll()'s WiFi
// connection for the same constraint on the cloud-sync side) -- there is
// no offline way to reach this dashboard, by design, since it needs an IP
// address on the same network as whatever browser is viewing it.
//
// Toggle with SYM + T. Since this device has no screen of its own, on
// activation it "types" its local URL as keystrokes into whatever's
// currently focused on the paired host (reusing the same BLE HID
// keystroke path used for regular typing) rather than displaying it
// anywhere -- focus a text field on your X4/phone/computer before
// toggling this on if you want to actually capture the URL.
//
// Tag filtering from the original feature list isn't implemented --
// there's no tagging mechanism in this firmware yet at all (recordings
// are just numbered). This is browse/download/stream only.

#if defined(TRANSFER_MODE_ENABLED) && BOARD_TYPE == FAIRBERRY_ESP32S3_SMART

#include <WebServer.h>
#include <SD.h>

WebServer transferServer(80);
bool transferModeActive = false;

String transferContentType(const String &filename) {
  if (filename.endsWith(".wav")) return "audio/wav";
  if (filename.endsWith(".txt")) return "text/plain";
  return "application/octet-stream";
}

void transferHandleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Fairberry notes</title>"
                "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                "</head><body><h1>Fairberry notes</h1><table border=1 cellpadding=6>"
                "<tr><th>Recording</th><th>Transcript</th><th>Audio</th></tr>";

  File dir = SD.open("/recordings");
  File entry = dir.openNextFile();
  while (entry) {
    String name = String(entry.name());
    if (name.endsWith(".wav")) {
      String txtPath = storageTranscriptPathFor("/recordings/" + name);
      bool hasTranscript = SD.exists(txtPath);
      html += "<tr><td>" + name + "</td><td>";
      if (hasTranscript) {
        html += "<a href=\"/download?file=" + name.substring(0, name.length() - 4) + ".txt\">.txt</a>";
      } else {
        html += "(not transcribed yet)";
      }
      html += "</td><td><a href=\"/download?file=" + name + "\">.wav</a></td></tr>";
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();

  html += "</table></body></html>";
  transferServer.send(200, "text/html", html);
}

void transferHandleDownload() {
  if (!transferServer.hasArg("file")) {
    transferServer.send(400, "text/plain", "Missing file parameter");
    return;
  }
  String filename = transferServer.arg("file");
  // Reject anything that isn't a plain filename -- this only ever serves
  // out of /recordings, a path with a slash in it is not a valid request.
  if (filename.indexOf('/') != -1 || filename.indexOf("..") != -1) {
    transferServer.send(400, "text/plain", "Invalid filename");
    return;
  }
  String path = "/recordings/" + filename;
  File f = SD.open(path, FILE_READ);
  if (!f) {
    transferServer.send(404, "text/plain", "Not found");
    return;
  }
  transferServer.streamFile(f, transferContentType(filename));
  f.close();
}

void transferModeStart() {
  if (!wifiConnect()) {
    return;
  }
  transferServer.on("/", transferHandleRoot);
  transferServer.on("/download", transferHandleDownload);
  transferServer.begin();
  transferModeActive = true;

  String url = "Transfer mode: http://" + WiFi.localIP().toString() + "/";
  typeString(url.c_str());
}

void transferModeStop() {
  transferServer.stop();
  wifiDisconnect();
  transferModeActive = false;
}

void transferModeToggle() {
  if (transferModeActive) {
    transferModeStop();
  } else {
    transferModeStart();
  }
}

// Call once per loop() iteration while transfer mode may be active.
void transferModePoll() {
  if (transferModeActive) {
    transferServer.handleClient();
  }
}

#endif
