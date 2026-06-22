[![Windows](https://github.com/G-Motivation/licensePlate/actions/workflows/windows.yml/badge.svg)](https://github.com/G-Motivation/licensePlate/actions/workflows/windows.yml)

# licensePlate — Parking Lot Management System with License Plate Recognition

A Windows desktop application that manages a parking lot by capturing a vehicle
with a camera, recognizing the license plate with a **YOLOv8** model loaded via
**OpenCV DNN (ONNX)**, and storing entry/exit records in a **SQLite** database.
The plate is read **twice — once on entry and once on exit** — and the parking
fee is calculated from the duration.

Built with **Qt 5 + OpenCV 4 (DNN module)**. No Darknet, no CUDA toolkit
dependency at build time.

## Features

- **Entry mode (IN)**: recognize the plate, then press `OK` to store the plate
  number and entry time in the database.
- **Exit mode (OUT)**: recognize the plate, then press `OK` to look up the entry
  record, compute the fee from the parking duration, and remove the record.
- **YOLOv8 recognition**: the captured frame is fed to `cv::dnn::readNetFromONNX`;
  the detected character boxes are filtered by NMS, sorted left-to-right, and
  concatenated into a plate string (the `plate` bounding-box class itself is
  skipped).
- **Manual correction**: a `0-9 / A-Z` on-screen keypad lets you fix the result
  when recognition is wrong.
- **Camera fallback**: if no camera is active, a bundled test image
  (`googletest/TestLicense.jpg`) is used so the flow can still be demonstrated.

## Workflow

1. On the camera page, choose `Entry (IN)` / `Exit (OUT)`.
2. Press `Camera Start` → `Capture`.
   - The frame is converted to an OpenCV `cv::Mat` and passed to the ONNX model.
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

The application loads a single ONNX file. Place these files under `model/`:

- `best.onnx` — exported YOLOv8 model (input `1x3x640x640`, output `[1, 4+nc, 8400]`)
- `classes.txt` — one class name per line, matching the training order

### Expected class layout for Taiwan plates

```
plate
0
1
2
...
9
A
B
C
...
Z
```

The first class **must** be `plate` (the whole-plate bounding box). It is
skipped when concatenating the plate string — only character classes are
joined, sorted by their `x` coordinate.

### Training a Taiwan-plate model (quick recipe)

```bash
pip install ultralytics
yolo detect train data=tw_plate.yaml model=yolov8n.pt epochs=100 imgsz=640
yolo export model=runs/detect/train/weights/best.pt format=onnx opset=12
cp runs/detect/train/weights/best.onnx model/best.onnx
```

`tw_plate.yaml` describes the dataset (paths to `train`/`val`, class names).

## Build

- **Qt 5** (modules: `core gui multimedia multimediawidgets widgets sql`)
- **OpenCV 4.x** with the **DNN module enabled** (bundled under `OpenCV/` on
  Windows; on macOS/Linux it is picked up from the system install — see
  `licensePlate.pro`)

Open `licensePlate.pro` in Qt Creator (MSVC x64 on Windows) and build.

### Runtime DLLs (Windows)

Make sure `opencv_world430.dll` (release) or `opencv_world430d.dll` (debug)
is next to the built executable or on `PATH`.

## Project Layout

```
licensePlate/
├─ main.cpp
├─ licenseplatedialog.{h,cpp,ui}   # main dialog: camera, ONNX inference, keypad, payment
├─ ui_licenseplatedialog.h         # generated UI (keypad built in code)
├─ model/                          # best.onnx + classes.txt
├─ database/                       # LICENSE.db (SQLite)
├─ OpenCV/                         # bundled OpenCV 4 headers & libs (Windows)
├─ testQT/                         # Google Test project
└─ Doc/                            # ALPR reference papers
```

## Migration notes (from the Darknet version)

This codebase used to load a `.cfg + .weights` pair through a Darknet C++ DLL
and required CUDA 11.7 at build time. That path has been removed:

- `yolo_v2_class.hpp`, `yolov3Lib/` — deleted
- `licensePlate.pro` — no more `yolov3Lib`, no more hard-coded CUDA paths
- `licenseplatedialog.{h,cpp}` — `Detector` / `bbox_t` replaced with
  `cv::dnn::Net` and a local `Detection` struct; `detect()` does its own NMS

If you want to keep the GPU path, build OpenCV with CUDA support and set
`_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA)` /
`_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA)` in the constructor.
