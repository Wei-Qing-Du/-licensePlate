[![Windows](https://github.com/G-Motivation/licensePlate/actions/workflows/windows.yml/badge.svg)](https://github.com/G-Motivation/licensePlate/actions/workflows/windows.yml)

# licensePlate — Parking Lot Management System with License Plate Recognition

A Windows desktop application that manages a parking lot by capturing a vehicle
with a camera, recognizing the license plate with a **YOLO** model, and storing
entry/exit records in a **SQLite** database. The plate is read **twice — once on
entry and once on exit** — and the parking fee is calculated from the duration.

Built with **Qt 5 + OpenCV + Darknet (YOLO)**.

## Features

- **Entry mode (IN)**: recognize the plate, then press `OK` to store the plate
  number and entry time in the database.
- **Exit mode (OUT)**: recognize the plate, then press `OK` to look up the entry
  record, compute the fee from the parking duration, and remove the record.
- **YOLO recognition**: the captured frame is run through the Darknet detector;
  the detected character boxes are sorted left-to-right and concatenated into a
  plate string (the `plate` bounding-box class itself is skipped).
- **Manual correction**: a `0-9 / A-Z` on-screen keypad lets you fix the result
  when recognition is wrong.
- **Camera fallback**: if no camera is active, a bundled test image
  (`googletest/TestLicense.jpg`) is used so the flow can still be demonstrated.

## Workflow

1. On the camera page, choose `Entry (IN)` / `Exit (OUT)`.
2. Press `Camera Start` → `Capture`.
   - The frame is converted to an OpenCV image and passed to YOLO.
   - The recognized plate is filled into the text box on the payment page.
3. Confirm / edit the plate, then press `OK`.
   - Entry: the record is inserted into the database.
   - Exit: the fee is calculated and shown.

## Fee Configuration

Defined at the top of `licenseplatedialog.cpp`:

```cpp
static const int FEE_PER_HOUR = 30; // fee per hour
static const int FEE_MINIMUM  = 30; // minimum fee (partial hours are rounded up)
```

## Database

SQLite file: `database/LICENSE.db`

```sql
CREATE TABLE license (
    id          int,
    licenseNum  varchar(20),
    dateandtime DATE
);
```

- Entry inserts a row (`id = MAX(id)+1`, plate, ISO timestamp).
- Exit selects the latest row for the plate, computes the fee, then deletes it.

## Model

Files live in `model/`:

- `yolov2.cfg`
- `yolov3-tiny_140000.weights`
- `voc.names` (classes: `plate`, `0-9`, `A-Z`, and Chinese province characters)

> **Note:** the weights file is `yolov3-tiny` but the config is `yolov2.cfg`.
> These two architectures do not match — load may fail or detection may be
> wrong. Use a matching `yolov3-tiny.cfg` if you hit problems.

## Build

- Qt 5 (modules: `core gui multimedia multimediawidgets widgets sql`)
- OpenCV (bundled under `OpenCV/`)
- Darknet YOLO C++ wrapper: `yolo_v2_class.hpp` + `yolov3Lib/yolo_cpp_dll.lib`
- CUDA libraries are linked in `licensePlate.pro` (see the `win32` block)

Open `licensePlate.pro` in Qt Creator (MSVC x64) and build.

## Project Layout

```
licensePlate/
├─ main.cpp
├─ licenseplatedialog.{h,cpp,ui}   # main dialog: camera, YOLO, keypad, payment
├─ ui_licenseplatedialog.h         # generated UI (keypad built in code)
├─ yolo_v2_class.hpp               # Darknet C++ wrapper
├─ yolov3Lib/                      # yolo_cpp_dll import library
├─ model/                          # YOLO cfg / weights / class names
├─ database/                       # LICENSE.db (SQLite)
├─ OpenCV/                         # bundled OpenCV headers & libs
├─ testQT/                         # Google Test project
└─ Doc/                            # ALPR reference papers
```
