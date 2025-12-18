# M5StickC Plus2 Gait Logger

A self-contained hip-worn gait logger with a built-in web interface.

## Features

- **Real-time Gait Analysis**: Step count, Cadence, Speed, Momentum, Limping Index.
- **Web Interface**: Live charts and metrics via WiFi AP.
- **Data Logging**: CSV recording to internal storage (LittleFS).
- **Standalone**: Battery powered, wearable.

## Hardware

- **Device**: M5StickC Plus2
- **Mounting**: Lower back / hip, screen facing out, USB-C to the left.

## Project Structure

- `firmware/main.cpp`: Main firmware source code.
- `data/index.html`: Web interface (must be uploaded to LittleFS).
- `platformio.ini`: PlatformIO configuration.

## How to Upload (Arduino IDE - Recommended)

Since your command-line environment has dependency issues, the **Arduino IDE** is the most reliable way to upload.

### 1. Setup Arduino IDE

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software).
2. Open Arduino IDE.
3. Go to **File > Preferences**.
4. In "Additional Boards Manager URLs", add:
    `https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json`
5. Go to **Tools > Board > Boards Manager**.
6. Search for `M5Stack` and install **M5Stack by M5Stack**.

### 2. Install Libraries

1. Go to **Tools > Manage Libraries**.
2. Search for and install:
    - `M5Unified`
    - `ESPAsyncWebServer` (by ESPHome or Me-No-Dev)
    - `AsyncTCP` (by ESPHome or Me-No-Dev)

### 3. Upload Filesystem (Web UI)

*Note: Arduino IDE 2.x doesn't support the old "Sketch Data Upload" plugin easily. We will use a workaround or you can just put the HTML in the code if needed. BUT, for now, let's try to compile the firmware first.*

**Alternative for Web UI**:
If you cannot upload the filesystem, you can convert `index.html` to a C string string in `main.cpp`.
**I have updated the firmware to include the HTML directly** so you don't need to worry about filesystem uploading!

### 4. Upload Firmware

1. Open `firmware/main.cpp` in Arduino IDE.
    - *Note*: You might need to rename `main.cpp` to `gait.ino` and put it in a folder named `gait`.
2. Select Board: **M5StickC Plus2** (or `M5StickCPlus2`).
3. Connect your device via USB.
4. Select the correct Port.
5. Click **Upload** (Right Arrow icon).

## Usage

1. Power on the device.
2. Wait for the screen to show:

   ```
   SSID: GAIT-LOGGER
   IP: 192.168.4.1
   ```

3. Connect your phone/laptop to WiFi `GAIT-LOGGER` (password: `circumduct123`).
4. Open browser to `http://192.168.4.1`.
5. Use the Web UI to view live data or start recording.
