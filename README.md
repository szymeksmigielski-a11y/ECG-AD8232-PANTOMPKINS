\# ESP32 ECG Signal Acquisition \& Pan-Tompkins Visualizer



A complete hardware and software solution for ECG biopotential signal acquisition, processing, and real-time visualization. The project consists of an ESP32 firmware module paired with an AD8232 front-end sensor, and a high-performance C++ desktop application built with SFML for offline visual signal processing and QRS complex detection via the Pan-Tompkins algorithm.



\---



\## Features



\- \*\*Hardware Signal Acquisition:\*\* High-precision, non-blocking sampling at 200 Hz using low-level ESP32 ADC drivers (`driver/adc.h`) to guarantee timing accuracy.

\- \*\*Interactive Visualizer:\*\* Multi-channel ECG trace desktop application built with C++ and SFML, featuring smooth timeline scrolling and dynamic statistics.

\- \*\*Pan-Tompkins DSP Pipeline:\*\* Modular signal processing pipeline including low-pass and high-pass filtering, 5-point differentiation, squaring, moving-window integration, and adaptive thresholding.

\- \*\*Binary Format Optimization (`.ekg`):\*\* Built-in converter transforms raw CSV text files into a fast binary structure for optimized load times and smaller file sizes.

\- \*\*Statistical Analytics:\*\* Real-time computation of average Heart Rate (HR), minimum/maximum HR, and total detected beat count.



\---



\## Pan-Tompkins DSP Pipeline Steps



The QRS detection engine processes the input signal through sequential stages:



1\. \*\*Bandpass Filter (Low-Pass + High-Pass):\*\* Removes high-frequency muscle artifacts and low-frequency baseline wander (0.5 Hz - 15 Hz passband).

2\. \*\*5-Point Derivative:\*\* Highlights high-slope regions corresponding to QRS complexes.

3\. \*\*Squaring:\*\* Non-linearly amplifies the QRS frequency components while making all values positive.

4\. \*\*Moving Window Integration:\*\* Generates a smoothed energy envelope of the wave.

5\. \*\*Adaptive Thresholding:\*\* Uses dynamic dual-threshold classification (signal floor SPKI and noise floor NPKI) with a strict refractory period to prevent double-counting beats.



\---



\## Hardware Setup (ESP32 + AD8232)



\### Pinout Configuration

| AD8232 Pin | ESP32 Pin | Description |

| :--- | :--- | :--- |

| \*\*OUTPUT\*\* | \*\*GPIO34\*\* (ADC1\_CH6) | Analog biopotential output signal |

| \*\*LO+\*\* | \*\*GPIO25\*\* | Lead-Off positive electrode disconnect indicator |

| \*\*LO-\*\* | \*\*GPIO26\*\* | Lead-Off negative electrode disconnect indicator |

| \*\*SDN\*\* | \*\*GPIO27\*\* | Shutdown control pin (Active High) |

| \*\*3.3V / GND\*\* | \*\*3.3V / GND\*\* | Power supply |



\### Electrode Placement (Standard Lead I Configuration)

\- \*\*RA (Right Arm):\*\* Positive terminal (`IN+`) placed below the right clavicle.

\- \*\*LA (Left Arm):\*\* Negative terminal (`IN-`) placed symmetrically below the left clavicle.

\- \*\*RL (Right Leg):\*\* Reference ground (`RLD`) placed on the lower right torso or hip.



\---



\## Custom Binary Format (`.ekg`)



To reduce file size and optimize parsing speed, the application supports conversion from `.csv` to `.ekg`:



```text

\[4 Bytes] Header Magic Number: "EKGB"

\[4 Bytes] Sample Count (uint32\_t)

\[N \* sizeof(ECGSample)] Payload Array (int64 timestamp, double value)

