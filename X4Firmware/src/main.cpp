// Fairberry Writer Deck -- X4 firmware
//
// A distraction-free writer app for the Xteink X4, built on FreeInk SDK,
// driven by the Fairberry keyboard over BLE HID (see ../BBQ10 for the
// keyboard side, and Documentation/Hardware_X4_Writer_Deck.md for the
// full picture of how the two halves fit together).
//
// CONFIDENCE NOTE: I built this against FreeInk SDK's documented API
// (freeink.org/docs) without a compiler or the actual SDK headers in
// front of me -- I could read the docs site but not browse the real
// source tree. The overall shape (BoardConfig/SdMan/EInkDisplay/BleHid
// init in setup(), a textArea-based editor screen, autosave to SD) is
// solidly grounded in what the docs describe. The exact field/method
// names marked "VERIFY:" below are my best reconstruction from prose
// descriptions of the API, not confirmed against a header file -- check
// these against your actual freeink-sdk checkout before this will
// compile, let alone run correctly.

#include <Arduino.h>
#include <FreeInkDisplay.h>   // VERIFY: actual header name/path in your checkout
#include <BoardConfig.h>
#include <SDCardManager.h>
#include <BleKeyboardHost.h>  // VERIFY: header name -- docs refer to the class as BleKeyboardHost, accessed via a `BleHid` global
#include <FreeInkUI.h>

// ---- Document state ----
// In-RAM buffer. Fine for a "few thousand words at a time" writer-deck
// document; a full-book-length single document would want a
// chunked/streaming buffer instead -- not attempted here, this is scoped
// to short-form distraction-free writing, not a general text editor.
String documentText = "";
size_t caretPos = 0;
String currentFilename = "";
unsigned long lastEditMs = 0;
unsigned long lastAutosaveMs = 0;
int editsSinceFullRefresh = 0;

#define AUTOSAVE_INTERVAL_MS 5000
#define FULL_REFRESH_EVERY_N_EDITS 40 // Periodic full refresh to clear e-ink ghosting from repeated fast partial updates

enum AppScreen { SCREEN_LIBRARY, SCREEN_EDITOR };
AppScreen currentScreen = SCREEN_LIBRARY;
AppScreen lastRenderedScreen = SCREEN_EDITOR; // deliberately != currentScreen so the first loop() renders once
bool screenDirty = true;

EInkDisplay display; // VERIFY: constructor likely wants a board profile/SPI pin set -- see lib-display docs

// Forward declarations -- unlike a .ino sketch, plain .cpp files here
// don't get Arduino's automatic function-prototype generation, so
// anything used before its definition needs one.
int wordCount();

// ---- SD helpers ----

String nextDraftPath() {
  for (int i = 0; i < 100000; i++) {
    char path[32];
    snprintf(path, sizeof(path), "/documents/DRAFT%05d.txt", i);
    if (!SdMan.exists(path)) { // VERIFY: SDCardManager's exists() call
      return String(path);
    }
  }
  return "";
}

void saveCurrentDocument() {
  if (currentFilename.length() == 0) return;
  File f = SdMan.open(currentFilename, FILE_WRITE); // VERIFY: SDCardManager's open() signature
  if (!f) return;
  f.print(documentText);
  f.close();
  lastAutosaveMs = millis();
}

// ---- Editor screen ----
// VERIFY: FreeInkApp's actual screen-function signature and the exact
// textArea()/statusBar() builder method parameter lists -- reconstructed
// from lib-ui's prose description ("the app owns the text buffer and
// caret offset; it word-wraps, draws the window of lines from topLine,
// and an optional caret") rather than a header.
ActionEvent editorScreen(ScreenBuilder &s) {
  s.statusBar(currentFilename.length() ? currentFilename : "(unsaved draft)",
              String(wordCount()) + " words");
  s.textArea(documentText, caretPos);
  return s.finish();
}

int wordCount() {
  int count = 0;
  bool inWord = false;
  for (size_t i = 0; i < documentText.length(); i++) {
    bool isSpace = isspace(documentText[i]);
    if (!isSpace && !inWord) count++;
    inWord = !isSpace;
  }
  return count;
}

// ---- Library screen ----
ActionEvent libraryScreen(ScreenBuilder &s) {
  s.header("Fairberry Writer Deck");
  // VERIFY: list()'s actual data-source signature -- this assumes it can
  // take a directory path and a selection callback, per lib-ui's "list --
  // virtualized rows with section headings" description, but the real
  // parameter shape wasn't confirmed.
  s.list("/documents", [](const String &path) {
    currentFilename = path;
    File f = SdMan.open(path, FILE_READ);
    if (f) {
      documentText = f.readString();
      f.close();
      caretPos = documentText.length();
    }
    currentScreen = SCREEN_EDITOR;
  });
  s.button("+ New draft", []() {
    currentFilename = nextDraftPath();
    documentText = "";
    caretPos = 0;
    currentScreen = SCREEN_EDITOR;
  });
  return s.finish();
}

// ---- Keyboard input handling ----
// VERIFY: KeyEvent's exact field names and SpecialKey enum members --
// lib-ble's docs describe "ev.special is a SpecialKey (PageDown,
// arrows…); ev.ch is printable input" but I don't have the full
// SpecialKey enum (does it include Enter/Backspace/Escape, or do those
// arrive as ev.ch with the usual ASCII control codes \n / \b / 0x1B?).
// Written here assuming the ASCII-control-code path since that's the
// more common HID-to-app convention and needs no extra enum members --
// swap to ev.special checks if your checkout's SpecialKey enum actually
// carries these.
void handleKeyEvent(const KeyEvent &ev) {
  lastEditMs = millis();
  editsSinceFullRefresh++;

  if (ev.special == SpecialKey::ArrowLeft) {
    if (caretPos > 0) caretPos--;
    return;
  }
  if (ev.special == SpecialKey::ArrowRight) {
    if (caretPos < documentText.length()) caretPos++;
    return;
  }
  // ArrowUp/ArrowDown for line navigation intentionally not implemented
  // yet -- needs textArea to expose which column/line the caret is on,
  // which wasn't in the docs I could read. Left/right + word-wrap
  // scrolling covers basic editing in the meantime.

  if (ev.ch == '\b' || ev.ch == 0x7F) {
    if (caretPos > 0) {
      documentText.remove(caretPos - 1, 1);
      caretPos--;
    }
    return;
  }
  if (ev.ch == 0x1B) { // Escape: back to library (with an autosave first)
    saveCurrentDocument();
    currentScreen = SCREEN_LIBRARY;
    return;
  }
  if (ev.ch == '\n' || ev.ch == '\r' || ev.ch >= 0x20) {
    char c = (ev.ch == '\r') ? '\n' : ev.ch;
    documentText = documentText.substring(0, caretPos) + c + documentText.substring(caretPos);
    caretPos++;
  }
}

void setup() {
  BoardConfig::holdPowerRails();
  BoardConfig::releaseSdRail();
  SdMan.begin();
  if (!SdMan.exists("/documents")) {
    SdMan.mkdir("/documents"); // VERIFY: SDCardManager's mkdir() call
  }

  display.begin();
  display.clearScreen();

  BleHid.begin("Fairberry Writer Deck");
}

void loop() {
  BleHid.poll();
  KeyEvent ev;
  while (BleHid.popKey(ev)) {
    if (currentScreen == SCREEN_EDITOR) {
      handleKeyEvent(ev);
      screenDirty = true;
    }
    // Library screen navigation currently relies on the X4's own touch/
    // button input via InputManager, not keyboard events -- not wired up
    // in this pass, see the doc's Known Gaps section. (Those input
    // callbacks already set screenDirty themselves when they fire, once
    // wired up -- for now the library screen only redraws on first entry
    // and when returning to it from the editor.)
  }

  if (currentScreen != lastRenderedScreen) {
    screenDirty = true;
  }

  if (currentScreen == SCREEN_EDITOR &&
      millis() - lastAutosaveMs > AUTOSAVE_INTERVAL_MS) {
    // Only worth checking/saving if something was actually typed since
    // the last save -- lastEditMs > lastAutosaveMs captures that without
    // needing a separate "dirty since last save" flag.
    if (lastEditMs > lastAutosaveMs) {
      saveCurrentDocument();
    }
  }

  // Renders only on an actual state change (a keystroke, entering a
  // different screen), not every loop() tick -- e-ink shouldn't be
  // refreshed continuously with nothing new to show.
  if (screenDirty) {
    screenDirty = false;
    lastRenderedScreen = currentScreen;

    RefreshHint hint = (editsSinceFullRefresh >= FULL_REFRESH_EVERY_N_EDITS)
      ? RefreshHint::Full
      : RefreshHint::Fast; // VERIFY: RefreshHint's actual enum member names
    if (editsSinceFullRefresh >= FULL_REFRESH_EVERY_N_EDITS) {
      editsSinceFullRefresh = 0;
    }
    // VERIFY: FreeInkApp's actual render-dispatch call -- this assumes a
    // simple screen-function-pointer + hint call per lib-ui's "render()
    // re-runs your screen function" description.
    if (currentScreen == SCREEN_EDITOR) {
      FreeInkApp::render(editorScreen, hint);
    } else {
      FreeInkApp::render(libraryScreen, hint);
    }
  }
}
