#include "licenseplatedialog.h"
#include "ui_licenseplatedialog.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <QtWidgets/QMessageBox>
#include <QtSql/QSqlError>
#define cameraPage 0
#define payPage 1
namespace fs = std::filesystem;

// Fee policy: 30 per hour, minimum charge 30 (partial hours are rounded up)
static const int FEE_PER_HOUR = 30;
static const int FEE_MINIMUM  = 30;

licensePlateDialog::licensePlateDialog(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::licensePlateDialog), _resultStr("")
{
  ui->setupUi(this);

  _camera = new QCamera(this);
  _cameraViewFinder = new QCameraViewfinder(this);
  _layout = new QVBoxLayout;

  _camera->setViewfinder(_cameraViewFinder);
  _layout->addWidget(_cameraViewFinder);
  _layout->setMargin(0);
  ui->m_camArea->setLayout(_layout);
  ui->stackedWidget->setCurrentIndex(cameraPage);

  // Capture image from camera
  _imageCapture = new QCameraImageCapture(_camera);
  connect(_imageCapture,
          &QCameraImageCapture::imageCaptured,
          this,
          &licensePlateDialog::ProcessCapturedImage);
  connect(ui->m_btnStart,
          &QPushButton::clicked,
          this,
          &licensePlateDialog::StartCamera);
  connect(ui->m_btnStop,
          &QPushButton::clicked,
          this,
          &licensePlateDialog::StopCamera);
  connect(ui->m_btnCapture,
          &QPushButton::clicked,
          this,
          &licensePlateDialog::CaptureImage);
  connect(ui->m_btnBack,
          &QPushButton::clicked,
          this,
          &licensePlateDialog::backSpaceforTXT);
  connect(ui->m_btnClear,
          &QPushButton::clicked,
          this,
          &licensePlateDialog::clearTXT);

  for (auto& btn : ui->m_buttons)
    {
      connect(btn,
              &QPushButton::clicked,
              this,
              &licensePlateDialog::PaymentBtnClicked);
    }

  connect(ui->m_btnOK, &QPushButton::clicked, this, &licensePlateDialog::OK2Pay);

  // --- Entry/Exit mode selector and result label, added in code ---
  _modeCombo = new QComboBox(ui->cameraPage);
  _modeCombo->addItem(QStringLiteral("Entry (IN)"));
  _modeCombo->addItem(QStringLiteral("Exit (OUT)"));
  _modeCombo->setGeometry(120, 320, 200, 30);
  connect(_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &licensePlateDialog::onModeChanged);

  _infoLabel = new QLabel(ui->payPage);
  _infoLabel->setGeometry(10, 85, 740, 60);
  _infoLabel->setWordWrap(true);
  _infoLabel->setStyleSheet(QStringLiteral("font-size:14px; color:#1f2937;"));

  // --- YOLOv8 (OpenCV DNN + ONNX) initialization ---
  _cur_path = fs::current_path().u8string() + "/../licensePlate";

  if (fs::exists(_cur_path))
    {
      _modelPath = _cur_path + "/model/best.onnx";
      _labels    = _cur_path + "/model/classes.txt";

      try
        {
          _net = cv::dnn::readNetFromONNX(_modelPath);
          _net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
          _net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
          _netReady = !_net.empty();
        }
      catch (const cv::Exception& e)
        {
          qWarning() << "ONNX model load failed:" << e.what();
          _netReady = false;
        }

      // Load class names once at startup
      std::ifstream labelfile(_labels);
      if (labelfile.is_open())
        {
          std::string line;
          while (std::getline(labelfile, line))
            {
              while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
                line.pop_back();
              if (!line.empty())
                _classnames.push_back(line);
            }
        }
    }

  // Open the SQLite connection once and reuse it in OK2Pay
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(QDir::currentPath() + "/database/LICENSE.db");
  if (!db.open())
    qWarning() << "DB open failed:" << db.lastError().text();

  onModeChanged(0);
}

void licensePlateDialog::onModeChanged(int index)
{
  _mode = (index == 1) ? Mode::Exit : Mode::Entry;
  if (_mode == Mode::Entry)
    _infoLabel->setText(QStringLiteral("[Entry mode] Verify the plate and press [OK] to record the entry."));
  else
    _infoLabel->setText(QStringLiteral("[Exit mode] Verify the plate and press [OK] to calculate the fee."));
}

void qimageToMat(const QImage& image, cv::OutputArray out)
{
  cv::Mat mat;
  cv::Size size(image.width(), image.height());

  switch (image.format())
    {
    case QImage::Format_Invalid:
    {
      mat.copyTo(out);
      break;
    }

    // 8 bit 4 channels
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
    {
      mat = cv::Mat(size,
                    CV_8UC4,
                    const_cast<uchar *>(image.constBits()),
                    static_cast<size_t>(image.bytesPerLine()));
      mat.copyTo(out);
      break;
    }

    // 8 bit 3 channels
    case QImage::Format_RGB888:
    {
      mat = cv::Mat(size,
                    CV_8UC3,
                    const_cast<uchar *>(image.constBits()),
                    static_cast<size_t>(image.bytesPerLine()));
      mat.copyTo(out);
      break;
    }

    // 8 bit 1 channel
    case QImage::Format_Indexed8:
    {
      mat = cv::Mat(size,
                    CV_8UC1,
                    const_cast<uchar *>(image.constBits()),
                    static_cast<size_t>(image.bytesPerLine()));
      mat.copyTo(out);
      break;
    }

    default:
    {
      qWarning() << "QImage to OpenCV format not handled:" << image.format();
      break;
    }
    }
}

void licensePlateDialog::clearTXT()
{
  ui->m_txtResult->clear();
  _resultStr = ui->m_txtResult->toPlainText();
}

void licensePlateDialog::backSpaceforTXT()
{
  QTextCursor cursor = ui->m_txtResult->textCursor();

  cursor.deletePreviousChar();
  _resultStr = ui->m_txtResult->toPlainText();
  ui->m_txtResult->setTextCursor(cursor);
}

void licensePlateDialog::PaymentBtnClicked()
{
  QString btnName = QObject::sender()->objectName();
  QStringList bottom_parts = btnName.split('_');
  QString lastPart = bottom_parts.last();
  QChar c = lastPart[lastPart.size() - 1];

  _resultStr += c;

  ui->m_txtResult->setText(_resultStr);
  QTextCursor cursor = QTextCursor(ui->m_txtResult->document());

  cursor.movePosition(QTextCursor::End);
  ui->m_txtResult->setTextCursor(cursor);
}

void licensePlateDialog::ProcessCapturedImage(int requestId, QImage img)
{
  Q_UNUSED(requestId);
  // Convert the captured camera frame to an OpenCV BGR Mat
  const QImage rgb = img.convertToFormat(QImage::Format_RGB888);
  cv::Mat tmp;
  qimageToMat(rgb, tmp);                       // RGB Mat
  if (!tmp.empty())
    cv::cvtColor(tmp, _testImg, cv::COLOR_RGB2BGR);

  runDetectionAndShow();
}

void licensePlateDialog::StartCamera()
{
  const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();

  for (const QCameraInfo& cameraInfo : cameras)
    {
      qDebug() << "cameraInfo.deviceName: " << cameraInfo.deviceName();
    }

  qDebug() << "error: " << _camera->error() <<
    "\n state:" << _camera->state() <<
    "\n status: " << _camera->status() <<
    "\n errorstring: " << _camera->errorString() <<
    "\n camptureMode: " << _camera->captureMode() <<
    "\n camera.lockStatus: " << _camera->lockStatus() <<
    "\n availableMetaData: " << _camera->availableMetaData() <<
    "\n camera.isAvailable: " << _camera->isAvailable() <<
    "\n viewfinder.isEnabled: " << _cameraViewFinder->isEnabled() <<
    "\n viewfinder.isVisible: " << _cameraViewFinder->isVisible();

  _camera->setViewfinder(_cameraViewFinder);
  _camera->setCaptureMode(QCamera::CaptureViewfinder);
  _camera->start();
}

void licensePlateDialog::StopCamera()
{
  _camera->stop();
}

cv::Scalar licensePlateDialog::obj_id_to_color(int obj_id)
{
  int const colors[6][3] = { { 1, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 } };
  int const offset = obj_id * 123457 % 6;
  int const color_scale = 150 + (obj_id * 123457) % 100;
  cv::Scalar color(colors[offset][0], colors[offset][1], colors[offset][2]);
  color *= color_scale;
  return color;
}

void licensePlateDialog::draw_boxes(cv::Mat& mat_img, const std::vector<Detection>& result_vec,
                                    const std::vector<std::string>& obj_names)
{
  for (const auto& d : result_vec)
    {
      cv::Scalar color = obj_id_to_color(d.classId);
      cv::rectangle(mat_img, d.box, color, 2);
      if (d.classId >= 0 && d.classId < static_cast<int>(obj_names.size()))
        {
          std::string label = obj_names[d.classId];
          cv::Size const text_size = cv::getTextSize(label, cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, 2, 0);
          int max_width = std::max(text_size.width, d.box.width + 2);
          cv::rectangle(mat_img,
                        cv::Point(std::max(d.box.x - 1, 0), std::max(d.box.y - 35, 0)),
                        cv::Point(std::min(d.box.x + max_width, mat_img.cols - 1),
                                  std::min(d.box.y, mat_img.rows - 1)),
                        color, cv::FILLED);
          cv::putText(mat_img, label, cv::Point(d.box.x, d.box.y - 16),
                      cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);
        }
    }
}

// YOLOv8 ONNX inference
// Input: a BGR cv::Mat / Output: detections after NMS
std::vector<Detection> licensePlateDialog::detect(const cv::Mat& image,
                                                  float confThreshold,
                                                  float nmsThreshold)
{
  std::vector<Detection> detections;
  if (!_netReady || image.empty())
    return detections;

  // Simple resize (no letterboxing); scale boxes back with per-axis ratios afterwards
  cv::Mat blob;
  cv::dnn::blobFromImage(image, blob, 1.0 / 255.0,
                         cv::Size(_inputSize, _inputSize),
                         cv::Scalar(), /*swapRB=*/true, /*crop=*/false);
  _net.setInput(blob);

  std::vector<cv::Mat> outs;
  try
    {
      _net.forward(outs, _net.getUnconnectedOutLayersNames());
    }
  catch (const cv::Exception& e)
    {
      qWarning() << "DNN forward failed:" << e.what();
      return detections;
    }

  if (outs.empty())
    return detections;

  // YOLOv8 ONNX output shape is [1, 4 + num_classes, 8400].
  // Reshape to [4+nc, 8400], then transpose to [8400, 4+nc] for row-wise iteration.
  cv::Mat out = outs[0];
  if (out.dims == 3)
    out = out.reshape(1, out.size[1]);   // [4+nc, 8400]
  cv::Mat outT;
  cv::transpose(out, outT);              // [8400, 4+nc]

  const int rows = outT.rows;
  const int dims = outT.cols;
  const int numClasses = dims - 4;
  if (numClasses <= 0)
    return detections;

  const float xScale = static_cast<float>(image.cols) / static_cast<float>(_inputSize);
  const float yScale = static_cast<float>(image.rows) / static_cast<float>(_inputSize);

  std::vector<int>      classIds;
  std::vector<float>    confidences;
  std::vector<cv::Rect> boxes;

  for (int r = 0; r < rows; ++r)
    {
      const float* row = outT.ptr<float>(r);
      const float* scores = row + 4;
      cv::Mat scoreMat(1, numClasses, CV_32F, const_cast<float*>(scores));
      cv::Point classIdPoint;
      double maxScore = 0.0;
      cv::minMaxLoc(scoreMat, nullptr, &maxScore, nullptr, &classIdPoint);
      if (maxScore < confThreshold)
        continue;

      float cx = row[0], cy = row[1], w = row[2], h = row[3];
      int left   = static_cast<int>((cx - w / 2.f) * xScale);
      int top    = static_cast<int>((cy - h / 2.f) * yScale);
      int width  = static_cast<int>(w * xScale);
      int height = static_cast<int>(h * yScale);

      classIds.push_back(classIdPoint.x);
      confidences.push_back(static_cast<float>(maxScore));
      boxes.emplace_back(left, top, width, height);
    }

  std::vector<int> keep;
  cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, keep);

  detections.reserve(keep.size());
  for (int idx : keep)
    {
      Detection d;
      d.classId    = classIds[idx];
      d.confidence = confidences[idx];
      d.box        = boxes[idx] & cv::Rect(0, 0, image.cols, image.rows);
      detections.push_back(d);
    }
  return detections;
}

// Sort character boxes left-to-right, skip the whole-plate box, and concatenate the result
QString licensePlateDialog::platesFromBoxes(const std::vector<Detection>& boxes) const
{
  std::vector<Detection> chars;
  for (const auto& b : boxes)
    {
      if (b.classId < 0 || b.classId >= static_cast<int>(_classnames.size()))
        continue;
      if (_classnames[b.classId] == "plate")
        continue;
      chars.push_back(b);
    }

  std::sort(chars.begin(), chars.end(),
            [](const Detection& a, const Detection& b) { return a.box.x < b.box.x; });

  QString plate;
  for (const auto& b : chars)
    plate += QString::fromLocal8Bit(_classnames[b.classId].c_str());
  return plate.toUpper();
}

// Run YOLO on _testImg, build the plate string, and switch to the payment page
void licensePlateDialog::runDetectionAndShow()
{
  if (_testImg.empty() || !_netReady)
    {
      QMessageBox::warning(this, QStringLiteral("Recognition error"),
                           QStringLiteral("Image or model is not ready."));
      return;
    }

  std::vector<Detection> boxes = detect(_testImg, 0.25f, 0.45f);
  draw_boxes(_testImg, boxes, _classnames);

  _resultStr = platesFromBoxes(boxes);
  ui->m_txtResult->setText(_resultStr);

  ui->stackedWidget->setCurrentIndex(payPage);
}

void licensePlateDialog::CaptureImage()
{
  // If the camera is active, take a real shot (result arrives in ProcessCapturedImage)
  if (_camera && _camera->status() == QCamera::ActiveStatus)
    {
      _imageCapture->capture();
      return;
    }

  // Demo fallback (bundled test image) when no camera is available
  _testImg = cv::imread((QDir::currentPath() + "/googletest/TestLicense.jpg").toStdString());
  runDetectionAndShow();
}

int licensePlateDialog::calcFee(qint64 minutes) const
{
  if (minutes <= 0)
    return FEE_MINIMUM;
  int hours = static_cast<int>((minutes + 59) / 60);   // round any partial hour up
  return std::max(hours * FEE_PER_HOUR, FEE_MINIMUM);
}

QString licensePlateDialog::findLatestEntryTime(const QString& plate)
{
  QSqlQuery q;
  q.prepare("SELECT dateandtime FROM license WHERE licenseNum = :p ORDER BY dateandtime DESC LIMIT 1");
  q.bindValue(":p", plate);
  if (q.exec() && q.next())
    return q.value(0).toString();
  return QString();
}

void licensePlateDialog::OK2Pay()
{
  const QString plate = ui->m_txtResult->toPlainText().trimmed().toUpper();
  if (plate.isEmpty())
    {
      QMessageBox::warning(this, QStringLiteral("Input error"),
                           QStringLiteral("Plate number is empty."));
      return;
    }

  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isOpen())
    {
      QMessageBox::critical(this, QStringLiteral("Database error"),
                            QStringLiteral("Failed to open the database."));
      return;
    }

  const QDateTime now = QDateTime::currentDateTime();
  const QString nowStr = now.toString(Qt::ISODate);

  if (_mode == Mode::Entry)
    {
      // Entry: insert a row using MAX(id)+1
      QSqlQuery idq("SELECT IFNULL(MAX(id), 0) + 1 FROM license");
      int nextId = 1;
      if (idq.next())
        nextId = idq.value(0).toInt();

      QSqlQuery query;
      query.prepare("INSERT INTO license (id, licenseNum, dateandtime) "
                    "VALUES (:id, :licenseNum, :dateandtime)");
      query.bindValue(":id", nextId);
      query.bindValue(":licenseNum", plate);
      query.bindValue(":dateandtime", nowStr);
      if (!query.exec())
        {
          QMessageBox::critical(this, QStringLiteral("Database error"), query.lastError().text());
          return;
        }
      _infoLabel->setText(QStringLiteral("Entry recorded\nPlate: %1\nEntry time: %2")
                          .arg(plate, now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
    }
  else // Exit
    {
      const QString entryStr = findLatestEntryTime(plate);
      if (entryStr.isEmpty())
        {
          QMessageBox::warning(this, QStringLiteral("No record"),
              QStringLiteral("No entry record found for plate %1.").arg(plate));
          return;
        }
      const QDateTime entryTime = QDateTime::fromString(entryStr, Qt::ISODate);
      const qint64 minutes = entryTime.secsTo(now) / 60;
      const int fee = calcFee(minutes);

      // Remove the settled record
      QSqlQuery del;
      del.prepare("DELETE FROM license WHERE licenseNum = :p");
      del.bindValue(":p", plate);
      del.exec();

      _infoLabel->setText(QStringLiteral("Exit\nPlate: %1\nEntry: %2\nExit: %3\nDuration: %4 min\nFee: %5")
                          .arg(plate,
                               entryTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                               now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                               QString::number(minutes),
                               QString::number(fee)));
    }

  // Reset the input and go back to the camera page for the next vehicle
  _resultStr.clear();
  ui->m_txtResult->clear();
  ui->stackedWidget->setCurrentIndex(cameraPage);
}

licensePlateDialog::~licensePlateDialog()
{
  delete ui;
  delete  _imageCapture;
}
