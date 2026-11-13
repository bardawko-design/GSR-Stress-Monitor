# Galvanic Skin Response (GSR) Stress Monitor

An ESP32-based experimental Electrodermal Activity (EDA) monitor. The project measures a voltage-divider signal, estimates skin conductance, logs timestamped samples, and compares sampling rates to study the trade-off between stress-response capture and energy use.

> **Important:** This is an educational prototype, not a medical device or a validated stress detector. EDA reflects sympathetic sudomotor activity, but it is affected by temperature, movement, hydration, electrode contact, and individual differences. It cannot identify a person's emotional state on its own.

## Research question

**What skin-conductance sampling rate provides sufficient detection of an EDA response while minimizing energy consumption?**

## Project objectives

1. Build an EDA voltage-divider interface.
2. Read and calibrate the ESP32 ADC.
3. Convert divider voltage into estimated resistance and conductance.
4. Log samples as CSV through the serial port.
5. Compare 1, 2, 4, 8, and 16 samples/second.
6. Compare response amplitude and timing with a lower-rate reference, then measure or estimate energy use.
7. Document limitations and propose improvements.

## System architecture

Skin electrodes → voltage divider → ESP32 ADC (GPIO34) → filtering and conversion → CSV serial logging → offline analysis.

## Hardware

- ESP32 DevKit V1
- 100 kΩ, 1% resistor (fixed divider resistor)
- Two skin electrodes / conductive finger electrodes
- Breadboard and jumper wires
- USB cable and computer

For a Wokwi-only demonstration, use a potentiometer as an adjustable analog signal source. The potentiometer is a **signal emulator**, not a biological skin model.

## Circuit (physical prototype)

Use a low-voltage, battery-powered or USB-powered ESP32 only. Never connect electrodes to mains-powered circuits or to a person while the board is connected to any unsafe equipment.

Recommended divider topology:

```text
ESP32 3V3 ---- skin/electrode resistance (Rskin) ----+---- GPIO34 (ADC)
                                                    |
                                                  100 kΩ
                                                    |
ESP32 GND ------------------------------------------+
```

The fixed 100 kΩ resistor limits current. With 3.3 V and a short-circuit fault, the theoretical current is at most about 33 µA through this resistor. This is not a formal electrical-safety certification. Do not use this circuit on people if there is any uncertainty about isolation, wiring, or power source. For human testing, prefer a battery-powered, electrically isolated design and obtain instructor approval.

The divider voltage is:

`Vout = Vcc * Rfixed / (Rskin + Rfixed)`

Therefore:

`Rskin = Rfixed * (Vcc / Vout - 1)`

`Conductance (µS) = 1,000,000 / Rskin (Ω)`

The formula is valid for the topology shown and when Vout is above zero. Readings near ADC rails are unreliable; the firmware marks them invalid.

## Wokwi simulation

1. Create a new **ESP32 DevKit V1** project at https://wokwi.com/.
2. Use the included root-level `sketch.ino` (it is the same source as `src/gsr_monitor.ino`).
3. Replace `diagram.json` with the repository's `diagram.json`.
4. Add the `wokwi.toml` file if using the Wokwi VS Code extension; for the web editor, select the ESP32 board and paste the diagram and code.
5. Start the simulation and open Serial Monitor at 115200 baud.
6. Rotate the potentiometer to vary the ADC signal. In this simulation, the potentiometer output is a convenient adjustable signal representing the divider node voltage. It does not reproduce skin physiology or real electrode behavior.

## Physical setup and calibration

1. Assemble the divider exactly as shown above.
2. Verify the voltage at GPIO34 with a multimeter before connecting electrodes.
3. Measure the actual supply voltage (`Vcc`) and actual fixed resistor (`Rfixed`); enter these values in the sketch.
4. With no electrodes connected, confirm the ADC reading is stable and not floating. Do not interpret an open circuit as a valid skin measurement.
5. Use a known resistor in place of the skin to check the conversion. Example: with Rskin = 100 kΩ, Rfixed = 100 kΩ, and Vcc = 3.3 V, expected Vout is approximately 1.65 V.
6. Only conduct human measurements after instructor approval, using a battery-powered isolated setup, clean electrodes, consent, and a non-invasive protocol.

## Serial commands

- `RATE 1` — sample at 1 Hz
- `RATE 2` — sample at 2 Hz
- `RATE 4` — sample at 4 Hz
- `RATE 8` — sample at 8 Hz
- `RATE 16` — sample at 16 Hz
- `BASELINE` — record the current conductance as the baseline
- `MARK REST` — add an event marker
- `MARK TASK` — add an event marker
- `MARK RECOVERY` — add an event marker
- `HELP` — print commands

CSV columns:

`millis,rate_hz,adc_raw,voltage_mV,resistance_ohm,conductance_uS,baseline_uS,delta_uS,event`

## Experiment protocol

### Recommended classroom protocol

Use a fixed, repeatable cognitive task such as a timed Stroop color-word task, subject to instructor approval. A Stroop task is a cognitive interference task; do not describe it as a clinical diagnosis or claim that it is equivalent to the Trier Social Stress Test.

For each participant/session:

1. Explain the procedure, obtain voluntary consent, and allow withdrawal at any time.
2. Seat the participant comfortably. Keep the same room, posture, hand, electrode placement, and approximate temperature.
3. Record 2 minutes of quiet baseline (`MARK REST`).
4. Run a 2-minute timed Stroop task (`MARK TASK`).
5. Record 2–3 minutes of quiet recovery (`MARK RECOVERY`).
6. Ask the participant to rate perceived task stress from 0 to 10. This is a subjective rating, not a clinical scale.
7. Repeat sessions at each sampling rate in randomized order, allowing recovery between runs. If the same person repeats the task, learning/fatigue effects are possible; note this limitation.
8. Keep electrode pressure and skin preparation consistent. Avoid movement and speaking during measurement.

Do not collect names or identifiable personal information. Store participant codes only. Do not publish raw personal data without permission. Stop if the participant feels uncomfortable.

### Sampling-rate comparison

Test 1, 2, 4, 8, and 16 Hz. Use the same task and protocol for each rate. Compare:
- Peak change from baseline (µS)
- Response latency from task start (seconds)
- Signal variability / noise during rest
- Whether the task-related response is visible
- Energy per unit time or average current, measured with a suitable USB power meter or current meter

A useful method is to compare each rate against a higher-rate reference (e.g. 16 Hz) after aligning timestamps. Do not assume 4 Hz is optimal before collecting data. The best rate is the lowest rate that preserves the response features you define, within a pre-set tolerance.

Suggested decision rule (state this before analyzing): choose the lowest sampling rate whose peak response differs by no more than 10% from the 16 Hz reference and whose response timing differs by no more than 1 second, provided rest noise remains acceptable. These thresholds are project design choices, not universal physiological standards.

## Data analysis

Use the CSV logs in Python, Excel, or Google Sheets. Plot conductance versus time and mark REST / TASK / RECOVERY periods. For each sampling rate calculate:
- Rest mean and standard deviation
- Task peak conductance
- Peak change: task peak minus rest mean
- Response latency
- Number of samples per minute
- Measured average current / power, if available

If you do not have a power meter, report the expected sampling-related trade-off qualitatively and clearly label it as an estimate. Sampling less often does not automatically reduce total ESP32 power proportionally because the microcontroller and ADC may remain powered between samples. A real low-power comparison requires sleep-mode firmware and current measurement.

## Known limitations

- Wokwi's potentiometer is only an analog signal emulator.
- The simple divider is sensitive to electrode contact and motion artifacts.
- ESP32 ADC readings vary by chip, attenuation, supply, and board; `analogReadMilliVolts()` is preferable to assuming raw ADC counts are perfectly linear.
- A simple divider is not a research-grade EDA front end. A future revision should use a dedicated, current-limited, isolated EDA analog front end and validated electrodes.
- EDA alone cannot prove that a participant is stressed.
- Small sample sizes and repeated-task learning effects limit conclusions.

## Repository structure

```text
GSR_Stress_Monitor/
├── README.md
├── diagram.json
├── wokwi.toml
├── src/
│   └── gsr_monitor.ino
├── docs/
│   ├── original_proposal.md
│   └── final_analysis_template.md
└── data/
    └── sample_log_template.csv
```

## Results

Do not invent results. Add your actual photos, serial logs, plots, measured power values, and final conclusions after testing. The final analysis template in `docs/` includes prompts for reporting achieved and unachieved goals.

## License

Educational project. Add a license if your instructor requires one.

https://wokwi.com/projects/480551079094211585
