# Architecture

Current prototype architecture for the autonomous chessboard.

---

## Physical Stack

```mermaid
graph TD
    subgraph Physical["Physical Stack"]
        direction TB
        A["Chess Piece<br/><i>magnetic base</i>"]
        B["Chessboard<br/><i>thin 8×8 passive surface</i>"]
        C["Carriage<br/><i>single Hall sensor + electromagnet</i>"]
        D["XY Gantry<br/><i>Cartesian: V-slot + V-wheels + GT2 belts</i>"]
        E["ESP32-S3<br/><i>motor control + sensor I/O</i>"]
    end

    A -->|"sits on"| B
    B -->|"mounted above"| C
    C -->|"moves via"| D
    D -->|"driven by"| E

    style A fill:#f9f9f9,stroke:#333,color:#000
    style B fill:#deb887,stroke:#8b7355,color:#000
    style C fill:#87ceeb,stroke:#4682b4,color:#000
    style D fill:#90ee90,stroke:#228b22,color:#000
    style E fill:#dda0dd,stroke:#8b008b,color:#000
```

The board surface is passive — every sensor and actuator rides on a carriage underneath. The electromagnet drags pieces on the surface via magnetic coupling through the wood; the Hall sensor verifies occupancy.

---

## System Architecture

```mermaid
graph TB
    subgraph Host["Host PC"]
        PY["Python / Stockfish<br/>chess_bridge"]
    end

    subgraph ESP32["ESP32-S3"]
        FW["Firmware"]
        TMCX["TMC2209 X"]
        TMCY["TMC2209 Y"]
        NX["NEMA17 X"]
        NY["NEMA17 Y"]
    end

    subgraph Mech["Mechanical"]
        GAN["XY gantry<br/>(Cartesian)"]
        CARR["Carriage"]
        HALL["Hall sensor"]
        ACT["Electromagnet"]
    end

    subgraph Board["Board"]
        SURF["Chessboard surface"]
        PIECE["Chess piece with magnet"]
    end

    PY -->|"USB serial 115200<br/>UCI moves / DONE / logs"| FW
    FW --> TMCX
    FW --> TMCY
    TMCX --> NX
    TMCY --> NY
    NX --> GAN
    NY --> GAN
    GAN --> CARR
    CARR --> HALL
    CARR --> ACT
    CARR -->|"scans"| SURF
    ACT -.->|"couples through surface"| PIECE
    HALL -.->|"detects magnet"| PIECE
    PIECE --- SURF
```

---

## Responsibility Split

### ESP32-S3 Firmware (current)

- Sensor reading and debouncing (single Hall sensor)
- Motor step generation with acceleration ramps (both axes)
- Chess-square kinematics (square ⇄ steps, sensor offset compensation)
- Electromagnet actuation (pickup/place)
- Serial command protocol (UCI moves, positioning, diagnostics)
- Homing and calibration (planned — endstops not yet installed)

### Python / Host Software (current)

- Chess rules and move validation (python-chess)
- Stockfish integration (optional, with fallback selector)
- Game state management, move history, results
- Serial orchestration of physical moves (chess_bridge)
- User interface (terminal)
- Physical path-planning policy inputs (piece-specific strategies in firmware)

ESP32-S3 decides *how* to move. Python decides *what* to move.

---

## Sensing (single Hall sensor)

Per [ADR-006](decisions/ADR-006-single-hall-sensor.md), one Hall sensor detects magnet presence only:

- Magnet detected → square occupied
- No magnet → square empty
- Piece *color* is not measured by hardware — it is tracked in software game state

The earlier dual-sensor polarity design ([ADR-002](decisions/ADR-002-dual-hall-polarity-detection.md)) was validated experimentally (EXP-002) but retired as unnecessary complexity.

---

## Motion Chain (built)

```
ESP32-S3 → TMC2209 X → NEMA17 X ─┐
                                   ├→ XY gantry (Cartesian) → Carriage
ESP32-S3 → TMC2209 Y → NEMA17 Y ─┘
```

Two independent belt-driven axes (GT2 belts and pulleys on V-slot aluminium extrusions with V-wheel carriages). Each motor owns one axis; X and Y move independently, or in interleaved lockstep for diagonal paths. Motion validation: [EXP-004](experiments/EXP-004-motor-control-validation.md); calibration: [EXP-006](experiments/EXP-006-movement-calibration.md). CoreXY kinematics were considered early in the project but the built prototype is a Cartesian XY gantry.
