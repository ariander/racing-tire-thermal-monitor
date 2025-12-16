# CAN Bus Daisy Chain Wiring Guide

Complete guide for wiring 4 tire temperature satellites to the master dashboard via CAN bus.

## Overview

CAN bus uses a **daisy chain** topology - all devices connect to two shared wires (CAN-H and CAN-L). Termination resistors (120Ω) are needed **only at the two physical ends** of the chain.

## Wiring Diagram - 4 Satellites + 1 Master

```
[120Ω]                                                     [120Ω]
  |                                                          |
  |----[Satellite FL]----[Satellite FR]----[Master]----[Satellite RL]----[Satellite RR]
       (0x10)            (0x11)            (ESP32-S3)     (0x12)            (0x13)

       All connected via:
       • CAN-H (yellow/orange wire)
       • CAN-L (green/white wire)
       • GND (common ground - CRITICAL!)
```

## Step-by-Step Wiring

### 1. CAN Transceiver Connections

Each device (4 satellites + 1 master) needs a CAN transceiver (MCP2551 or SN65HVD230).

**On each ESP32 (satellite or master):**

```
ESP32          CAN Transceiver
TX pin   ────► TX
RX pin   ◄──── RX
GND      ────► GND
3.3V     ────► VCC (check your module - some need 5V)
```

**Pin assignments:**
- **Satellites (WROOM-32)**: TX=GPIO5, RX=GPIO4
- **Satellites (C3 Super Mini)**: TX=GPIO2, RX=GPIO3
- **Master (ESP32-S3)**: TX=GPIO17, RX=GPIO18

### 2. CAN Bus Backbone Wiring

Connect all CAN transceivers together using two wires:

```
CAN-H: All transceiver CAN-H pins connected together (single bus)
CAN-L: All transceiver CAN-L pins connected together (single bus)
GND:   All device grounds connected together (CRITICAL!)
```

**Example physical wiring:**

```
Device          CAN-H Wire    CAN-L Wire    GND Wire
─────────────────────────────────────────────────────
Satellite FL ──┬─ orange ─────┬─ green ─────┬─ black
               │               │             │
Satellite FR ──┤               ├─────────────┤
               │               │             │
Master S3 ─────┤               ├─────────────┤
               │               │             │
Satellite RL ──┤               ├─────────────┤
               │               │             │
Satellite RR ──┴───────────────┴─────────────┴
```

### 3. Termination Resistors

Add 120Ω resistors **only at the two physical ends** of the CAN bus chain.

**Location options:**
- **Option A**: First satellite + Last satellite
- **Option B**: First satellite + Master (if master is at the end)

**How to add termination:**

Connect 120Ω resistor between CAN-H and CAN-L on the transceiver board at each end:

```
First Device:                Last Device:
   CAN-H ──┬──              CAN-H ──┬──
           │                        │
         [120Ω]                   [120Ω]
           │                        │
   CAN-L ──┴──              CAN-L ──┴──
```

**⚠️ CRITICAL:** Do NOT add terminators on middle devices! Only the two ends!

### 4. Power Supply

Each device needs its own power:
- **Satellites**: 3.3V or 5V (depending on ESP32 board regulator)
- **Master**: USB or external 5V (display needs more current)

**All devices must share common ground!** Connect all GND pins together.

## Example: Testing with 1 Satellite + Master

Start simple to verify everything works:

```
[120Ω] ──── [Satellite FL] ──── [Master S3] ──── [120Ω]
             (GPIO4/5)            (GPIO17/18)

Wiring:
• Satellite CAN-H ──► Master CAN-H (orange wire)
• Satellite CAN-L ──► Master CAN-L (green wire)
• Satellite GND ────► Master GND (black wire)
• 120Ω at satellite transceiver (CAN-H to CAN-L)
• 120Ω at master transceiver (CAN-H to CAN-L)
```

## Adding More Satellites

Once one satellite works, add others one at a time:

1. **Remove** terminator from previous last device
2. Wire new satellite to the bus (CAN-H, CAN-L, GND)
3. Change `WHEEL_ID` in code (0x10, 0x11, 0x12, 0x13)
4. Add terminator to the **new last device**
5. Power on and verify on master display

## Typical Wire Lengths

- **Short runs** (<2m per segment): Any wire works
- **Long runs** (2-5m): Use twisted pair (Ethernet cable works great!)
- **Racing environment**: Use shielded twisted pair

## Common Wiring Mistakes

### ❌ WRONG: Terminator on every device
```
[120Ω]──[SAT]──[120Ω]──[SAT]──[120Ω]──[MASTER]──[120Ω]  ← TOO MANY!
```

### ✅ CORRECT: Terminators only at ends
```
[120Ω]──[SAT]──[SAT]──[MASTER]──[SAT]──[120Ω]  ← PERFECT!
```

### ❌ WRONG: Star topology
```
        ┌─ [SAT 1]
        │
[MASTER]├─ [SAT 2]  ← DON'T DO THIS!
        │
        └─ [SAT 3]
```

### ✅ CORRECT: Daisy chain
```
[SAT 1]──[SAT 2]──[MASTER]──[SAT 3]  ← LINEAR CHAIN!
```

## Detailed Transceiver Pinout

### MCP2551 CAN Transceiver

```
      MCP2551
   ┌──────────┐
TX │1       8│ VCC (5V)
RX │2       7│ CAN-H  ────► To CAN bus
VSS│3       6│ CAN-L  ────► To CAN bus
GND│4       5│ GND
   └──────────┘
```

### SN65HVD230 CAN Transceiver

```
    SN65HVD230
   ┌──────────┐
 3V│1       8│ N/C
GND│2       7│ CAN-H  ────► To CAN bus
 TX│3       6│ CAN-L  ────► To CAN bus
 RX│4       5│ N/C
   └──────────┘
```

## Verifying CAN Communication

### On Each Satellite - Serial Monitor

```
=== TIRE TEMP: FL ===

OK: MLX90640 initialized
OK: CAN initialized

Streaming data...

Frames: 10 | Temp: 22.3 - 26.7 C
Frames: 20 | Temp: 22.1 - 27.2 C
```

### On Master - Serial Monitor

```
=== TIRE TEMP DASHBOARD ===

OK: Display initialized
OK: CAN initialized

Waiting for data...

Uptime: 10s | Packets: 290
FL: OK (age: 0s)
FR: NO SIGNAL (age: 10s)
RL: NO SIGNAL (age: 10s)
RR: NO SIGNAL (age: 10s)
```

### On Master - Display

- Active wheels show thermal heatmap (blue → red → white)
- Inactive wheels show "NO SIGNAL"
- Temperature range displayed below each wheel
- Color scale bar at bottom

## Troubleshooting

### No CAN Communication

**Symptoms:**
- Master shows "NO SIGNAL" for all wheels
- Satellites transmitting but master not receiving

**Solutions:**
1. ✅ Verify CAN-H and CAN-L not swapped
2. ✅ Check 120Ω terminators at **both ends only**
3. ✅ Confirm all grounds connected together
4. ✅ Verify TX/RX pins correct on each device
5. ✅ Check CAN transceiver power (3.3V or 5V)

### Intermittent Drops

**Symptoms:**
- Display shows data then "NO SIGNAL" repeatedly
- Incomplete thermal images

**Solutions:**
1. ✅ Check wire quality (use twisted pair for long runs)
2. ✅ Reduce wire length if possible
3. ✅ Verify solid connections (no loose wires)
4. ✅ Check for electromagnetic interference
5. ✅ Increase inter-packet delay in satellite code (already 1ms)

### Only Some Satellites Work

**Symptoms:**
- Some wheels show data, others show "NO SIGNAL"
- All satellites transmitting according to Serial Monitor

**Solutions:**
1. ✅ Verify each satellite has **unique** WHEEL_ID
2. ✅ Check that non-working satellites are on CAN bus
3. ✅ Test each satellite individually with master
4. ✅ Verify CAN transceiver connections on problem satellites

### Corrupted Display

**Symptoms:**
- Ghost images or streaks on display
- Partial thermal images

**Solutions:**
1. ✅ Already solved in current code (50-packet RX buffer)
2. ✅ Verify master code is latest version
3. ✅ Check satellite inter-packet delay is 1ms (not 500μs)

## Physical Installation Tips

### For Racing Car Installation

1. **Satellite mounting**: Mount near each wheel hub
2. **Wire routing**: Use cable ties, avoid moving parts
3. **Connector types**: Use Molex or JST-XH connectors for reliability
4. **Strain relief**: Add slack at each satellite for vibration
5. **Master placement**: Mount dashboard in driver's view
6. **Power**: Use car's 12V with buck converter to 5V

### Recommended Connectors

- **CAN bus**: Screw terminals or JST-XH (detachable)
- **I2C sensor**: JST-XH 4-pin (VCC, GND, SDA, SCL)
- **Power**: XT30 or barrel jack (depending on source)

### Cable Color Convention

```
CAN-H:    Orange or Yellow
CAN-L:    Green or White
GND:      Black
VCC:      Red
```

## Testing Checklist

Before full installation:

- [ ] Each satellite tested individually with master
- [ ] All four satellites configured with unique WHEEL_IDs
- [ ] CAN bus terminators only at two physical ends
- [ ] All grounds connected together
- [ ] Serial Monitor shows successful communication
- [ ] Master display shows all four wheels updating
- [ ] System runs stable for >5 minutes
- [ ] No "NO SIGNAL" timeouts during testing

## Wire Shopping List

For complete 4-satellite system:

- **CAN bus wire**: 10m twisted pair (22 AWG)
- **Power wire**: 10m red+black (18-22 AWG)
- **Connectors**: 20× JST-XH 2-pin, 5× JST-XH 4-pin
- **Terminators**: 2× 120Ω resistors (1/4W)
- **Heat shrink**: Various sizes
- **Cable ties**: For securing wires

## License

MIT License - See main project LICENSE file
