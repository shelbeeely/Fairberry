# Which parts of this page apply to e-ink devices?

Everything the mainboard firmware does — cursor mode, sticky modifiers, dead keys, backlight brightness — happens before anything leaves the keyboard, so it applies the same way no matter what it's plugged into. That's the "Built-in key combinations" section below.

Everything below "Adding key combinations" is Android-specific (it relies on Android apps like Key Mapper and Automate, and on `adb`), and won't apply to a Free Ink SDK device. What key-remapping or shortcut options exist on the reader side is currently unknown/untested — if you figure something out for a Free Ink SDK device, that's worth documenting here or upstream in [freeink-sdk](https://github.com/Free-Ink/freeink-sdk).

# Keyboard layout on Android phones

The Fairberry requires you to set the layout to be set to "English (USA), international".

The reasons are
- It allows for accented letters that should cover many languages quite well.
- Some Android versions (at least up to Android 12) have a bug that the system forces this layout if you use accessibility services.

This section doesn't apply outside of Android — a Free Ink SDK device's keyboard layout handling (if any) hasn't been documented here yet.

# Built-in key combinations

These key combinations work out of the box and don't require any other apps.

- Both shifts at the same time: Enter cursor mode, where WASD are cursor keys and Q is ESC
- Both shifts when in cursor mode: Exit cursor mode
- The microphone key (same key as 0) is mapped to Ctrl
- For all modifier keys (Alt, Shift, Ctrl, Sym):
  - Hold while typing other keys: Modifier applies to all keys while held
  - Click the modifier once and let go: The next keystroke will be affected by the modifier
  - Click the modifier twice: This modifier is locked (like caps lock)
  - Click the modifier another time: This modifier is deactivated again (same as not pressed at all
  - Clicking a different modifier key will deactivate the first.
- Alt + U: Dead key ¨. For example: Alt + U followed by O results in ö
- Alt + I: Deat key ˆ
- Alt + S: ß
- Alt + C: ç
- All of these can be capitalized by pressing e.g. Alt + U followed by Shift + O.
- Sym + Alt: Tab
- Ctrl + Sym: Increase keyboad backlight brightness
- Ctrl + Right Shift: Decrease keyboard backlight brightness
- Sym + Enter: Put the keyboard into sleep mode (with keyboard backlight turned off)

# Adding key combinations

Using the Key Mapper app by sds100 you can add more advanced key combinations that an USB keyboard can't just do on it's own. I currently use the following shortcuts, but feel free to do whatever you want.

- Alt + P: Recents
- Alt + Backspace: Back
- Alt + Enter: Home
- Alt + O: Split screen
- Alt + L: Flashlight
- Alt + $: Rotate screen

You can also use the "Key pressed" block in the Automate app by llamalab to trigger more complex actions using the keyboard.

# Customizable software keyboard for missing characters

Since the Blackberry Q10 keyboard doesn't contain all possible characters I use [Keyboard Designer](https://play.google.com/store/apps/details?id=de.humbergsoftware.keyboarddesigner) to add an one-row software keyboard containing characters that I frequently use but are not on the BBQ10 keyboard.


