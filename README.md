# ESP32 ECG Signal Acquisition & Pan-Tompkins Visualizer

A hardware + software project for ECG signal acquisition, processing and visualization:

- **ESP32 firmware** that samples an **AD8232** ECG front-end at 200 Hz and streams the samples over serial.
- **ECGPROJECTOR**, a C++/SFML desktop application (Windows) that opens recorded ECG files, runs them through the **Pan-Tompkins** QRS-detection pipeline and visualizes the result together with heart-rate statistics.

---

## Quick start (no compiler needed)

1. Download `ECGPROJECTOR-win64.zip` from the [Releases](../../releases) page.
2. Unzip it anywhere.
3. Run `ECGPROJECTOR.exe`.

Requirements: Windows 10/11 x64 and the [Microsoft Visual C++ Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe). Most PCs already have it installed.

The zip already contains a sample recording in `data/`. To view your own recordings, copy them into the `data/` folder next to the `.exe`.

## Building from source

Requirements: **Visual Studio 2022** with the *Desktop development with C++* workload. Nothing else is needed: the required parts of **SFML 2.6.1** (64-bit, VC17) are bundled in `DESKTOP-APP/SFML`.

1. Clone the repository.
2. Open `DESKTOP-APP/VS/ECGPROJECTOR/ECGPROJECTOR.sln`.
3. Select **Debug | x64** or **Release | x64**.
4. Press **F5**.

The build output goes to `DESKTOP-APP/VS/Build/<Configuration>/`. A post-build step copies the SFML DLLs, `assets/` and `data/` there, so that folder is self-contained and can be zipped and shared as-is.

> The code uses the SFML 2 API and does not compile against SFML 3.

## Using the application

- **File browser:** every `.csv` / `.ekg` file found in `data/` is shown as a tile. Click a tile to open the recording.
- **Konwertuj** (on `.csv` tiles): converts the CSV file to the binary `.ekg` format. The new file appears as an additional tile.
- **Signal view:**
  - green trace: raw ECG signal
  - red trace: output of the Pan-Tompkins pipeline (moving-window integration)
  - orange vertical lines: detected QRS complexes
  - top-right panel: average / minimum / maximum heart rate and number of detected beats
- **← / →** scroll the signal, **< Wroc** returns to the file browser.

The application reads `assets/font.ttf` and `data/` relative to its working directory, so start it from the folder that contains the `.exe`. Double-clicking the exe or pressing F5 in Visual Studio both do this.

---

## Pan-Tompkins DSP pipeline

The QRS detection engine processes the signal through sequential stages (the filter coefficients assume a 200 Hz sampling rate):

1. **Band-pass filter (low-pass + high-pass):** classic Pan-Tompkins integer-coefficient filters (low-pass ≈ 11 Hz, high-pass ≈ 5 Hz) that remove muscle noise, power-line interference and baseline wander.
2. **5-point derivative:** highlights the steep slopes of the QRS complex.
3. **Squaring:** makes all values positive and non-linearly emphasizes large slopes.
4. **Moving-window integration:** 30-sample (150 ms) window producing a smoothed energy envelope.
5. **Adaptive thresholding:** signal/noise peak estimates (SPKI / NPKI) initialised from the first 2 s of data, a dynamic threshold and a 200 ms refractory period to prevent double-counting beats.

Heart-rate statistics are computed from the RR intervals between detected beats; values outside 30–220 BPM are discarded.

---

## Hardware setup (ESP32 + AD8232)

### Pinout

| AD8232 pin | ESP32 pin | Description |
| :--- | :--- | :--- |
| **OUTPUT** | **GPIO36** (ADC1_CH0) | Analog ECG output signal |
| **LO+** | **GPIO25** | Lead-off detection, positive electrode |
| **LO-** | **GPIO26** | Lead-off detection, negative electrode |
| **SDN** | **GPIO27** | Shutdown control (driven HIGH = device on) |
| **3.3V / GND** | **3.3V / GND** | Power supply |

### Electrode placement (Lead I)

- **RA (Right Arm):** positive terminal (`IN+`), below the right clavicle.
- **LA (Left Arm):** negative terminal (`IN-`), symmetrically below the left clavicle.
- **RL (Right Leg):** reference (`RLD`), lower right torso or hip.

### Firmware

`ESP32/arduino_ECG/arduino_ECG.ino` (Arduino IDE, ESP32 board package). It reads the ADC directly through `driver/adc.h` (12-bit) every 5000 µs (200 Hz) and prints one value per line over serial at **115200 baud**. When an electrode is disconnected (LO+ or LO-) it prints `0`.

---

## Data formats

### CSV (`.csv`)

The first line is a header and is skipped. Each following line contains a timestamp and a raw 12-bit ADC value:

```text
timestamp_ms,adc_value
0,592
0,464
0,606
```

Values `<= 0` (lead-off) or `> 4095` are replaced with the last valid sample.

### Binary (`.ekg`)

The **Konwertuj** button converts a CSV file into a compact binary file that loads faster:

```text
[4 bytes]                    magic number "EKGB"
[4 bytes]                    sample count N (uint32, little-endian)
[N * 16 bytes]               samples: int64 timestamp, double value
```

---

## Repository structure

```text
ESP32/arduino_ECG/            ESP32 firmware (Arduino sketch)
DESKTOP-APP/SFML/             bundled SFML 2.6.1 (headers, import libs, DLLs: graphics/window/system)
DESKTOP-APP/VS/ECGPROJECTOR/  Visual Studio 2022 solution
  ECGPROJECTOR/src/           application source code
  ECGPROJECTOR/assets/        font used by the UI
  ECGPROJECTOR/data/          sample ECG recording (.csv and .ekg)
```

## Known limitations

- Visualization is **offline**: the desktop app works on recorded files, not on a live serial stream.
- The firmware outputs raw values only. The tool used to save the serial stream as a CSV file with timestamps is **not** part of this repository; any serial logger producing the CSV format above will work.
- A 200 Hz sampling rate is assumed throughout (filters, time axis, heart-rate calculation).
- Windows x64 only (Visual Studio project, bundled Windows build of SFML).

## License

This project is released under the [MIT License](LICENSE).

Bundled third-party components keep their own licenses:

- [SFML](https://www.sfml-dev.org) 2.6.1: zlib/png license, see [`DESKTOP-APP/SFML/license.md`](DESKTOP-APP/SFML/license.md)
- [Roboto](https://github.com/googlefonts/roboto-3-classic) font: SIL Open Font License 1.1, see [`assets/OFL-Roboto.txt`](DESKTOP-APP/VS/ECGPROJECTOR/ECGPROJECTOR/assets/OFL-Roboto.txt)
