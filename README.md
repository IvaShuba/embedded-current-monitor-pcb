# Embedded Current Monitor Pcb

## Overview

The goal of this project was to design, develop, and assemble a printed circuit board (PCB) for a microcontroller system. The system includes an extension board and a connected Grove LCD display.

To monitor the current consumption of different components, a four-channel amplifier and shunt resistors were used.

For visualization, a dashboard application was developed to receive data from the microcontroller via a USB connection.

---

## Implementation

### PCB Design

The PCB was designed using KiCad. The board dimensions, as well as the placement of the extension board and USB port, were defined by project requirements and constrained by the enclosure size.

The PCB is a four-layer design:
- Top layer: surface-mount components and signal routing  
- Inner layers: 3.3V VCC and GND planes  

Component placement requires careful attention. In this case, all routing was successfully completed on the top layer.

![KiCad PCB design](docs/images/kicad.png)

The design was verified using the manufacturer’s online tools (JLCPCB), and the board was sent for fabrication.

While waiting for manufacturing, a prototype was assembled on a breadboard. This allowed early debugging of the firmware and validation of the data flow from the microcontroller to the visualization panel.

![Breadboard prototype](docs/images/prototype.png)

After receiving the fabricated PCB, it was inspected for manufacturing defects. No issues were found.

![PCB](docs/images/pcb.png)


---

### Soldering and Assembly

Soldering was performed in parallel on two boards to enable comparative testing and design validation.

- Board A — assembled by me  
- Board B — assembled by a teammate  
- The third team member worked on the visualization dashboard

![Dashboard Demo](docs/images/dashboard.png)

After assembly, initial tests were performed using a multimeter to detect soldering defects such as cold joints and short circuits.

---

## Results

The design can be considered functional:
- The PCB and microcontroller operate correctly  
- Current measurements for external loads are displayed  
- The sum of measured currents is close to the value observed on the laboratory power supply  

![PCB A](docs/images/pcba.png)

Measured data:
- Voltage drop across the main shunt resistor: ~4 mV  
- Shunt resistance: 0.1 Ω  
- Calculated current: ~38 mA (close to ~35 mA observed on the lab power supply)

![Result dashboard](docs/images/result_dash.png)

However:
- After amplification and filtering, the microcontroller reports ~62 mA instead of ~35 mA

![Currents display board A](docs/images/currentsA.png)

Further verification is required:
- Measure amplifier output voltage  
- Measure post-filter signal  

---

## Issues and Challenges

1. **USB Type-B Connector Failure**  
   The USB Type-B connector on board A was mechanically damaged during cable insertion, which also damaged PCB traces.  
   **Solution:**  
   - Use an enclosure  
   - Improve mechanical fixation of the connector  
   - Replace SMD connector with through-hole version  
   - Consider switching to USB Type-C  

2. **5V Line Connector Design**  
   A connector was implemented to break the +5V line.  
   **Improvement:**  
   It would be more practical to use this as a dedicated external power input (5V and GND).

3. **Assembly Errors**  
   During inspection, incorrect component values were found on board B.

4. **Current Measurement Mismatch**  
   The total measured current does not match the lab power supply reading, although the voltage drop across the shunt resistor is consistent with expectations.

5. **Startup Issue on Board B**  
   Board B failed to start unless contacts around the crystal were probed in diode test mode.  
   **Root cause:**  
   Capacitors in the crystal oscillator circuit had mismatched capacitance values.
