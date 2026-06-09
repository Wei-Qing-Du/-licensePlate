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

// 料金設定: 1時間ごとに 30元、最低料金 30元(端数は切り上げ)
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

  // --- 入場 / 出場 切替 と 結果表示ラベルをコードで追加 ---
  _modeCombo = new QComboBox(ui->cameraPage);
  _modeCombo->addItem(QStringLiteral("入場 (IN)"));
  _modeCombo->addItem(QStringLiteral("出場 (OUT)"));
  _modeCombo->setGeometry(120, 320, 200, 30);
  connect(_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &licensePlateDialog::onModeChanged);

  _infoLabel = new QLabel(ui->payPage);
  _infoLabel->setGeometry(10, 85, 740, 60);
  _infoLabel->setWordWrap(true);
  _infoLabel->setStyleSheet(QStringLiteral("font-size:14px; color:#1f2937;"));

  // --- YOLO(Darknet) の初期化 ---
  _cur_path = fs::current_path().u8string() + "/../licensePlate";

  if (fs::exists(_cur_path))
    {
      _cfg = _cur_path + "/model/yolov2.cfg";
      _weight = _cur_path + "/model/yolov3-tiny_140000.weights";
      _labels = _cur_path + "/model/voc.names";
      _detector = new Detector(_cfg, _weight);

      // クラス名は起動時に一度だけ読み込む
      std::ifstream labelfile(_labels);
      if (labelfile.is_open())
        {
          std::string line;
          while (std::getline(labelfile, line))
            {
              while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
                line.pop_back();
              _classnames.push_back(line);
            }
        }
    }
  else
    _detector = nullptr;

  // SQLite 接続を一度だけ用意する(OK2Pay で再利用)
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
    _infoLabel->setText(QStringLiteral("【入場モード】ナンバーを確認して [OK] を押すと入場を記録します。"));
  else
    _infoLabel->setText(QStringLiteral("【出場モード】ナンバーを確認して [OK] を押すと料金を計算します。"));
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
  // カメラで撮影した実画像を OpenCV(BGR) に変換して使う
  const QImage rgb = img.convertToFormat(QImage::Format_RGB888);
  cv::Mat tmp;
  qimageToMat(rgb, tmp);                       // RGB の Mat
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

void licensePlateDialog::draw_boxes(cv::Mat mat_img, std::vector<bbox_t> result_vec, std::vector<std::string> obj_names,
                                    int current_det_fps, int current_cap_fps)
{
  int const colors[6][3] = { { 1, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 } };
//https://zhuanlan.zhihu.com/p/135380256
  for (auto &i : result_vec)
    {
      cv::Scalar color = obj_id_to_color(i.obj_id);
      cv::rectangle(mat_img, cv::Rect(i.x, i.y, i.w, i.h), color, 2);
      if (obj_names.size() > i.obj_id)
        {
          std::string obj_name = obj_names[i.obj_id];
          if (i.track_id > 0)
            obj_name += " - " + std::to_string(i.track_id);
          cv::Size const text_size = getTextSize(obj_name, cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, 2, 0);
          int max_width = (text_size.width > i.w + 2) ? text_size.width : (i.w + 2);
          max_width = std::max(max_width, (int)i.w + 2);
          //max_width = std::max(max_width, 283);
          std::string coords_3d;
          if (!std::isnan(i.z_3d))
            {
              std::stringstream ss;
              ss << std::fixed << std::setprecision(2) << "x:" << i.x_3d << "m y:" << i.y_3d << "m z:" << i.z_3d << "m ";
              coords_3d = ss.str();
              cv::Size const text_size_3d = getTextSize(ss.str(), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, 1, 0);
              int const max_width_3d = (text_size_3d.width > i.w + 2) ? text_size_3d.width : (i.w + 2);
              if (max_width_3d > max_width)
                max_width = max_width_3d;
            }
          cv::rectangle(mat_img, cv::Point2f(std::max((int)i.x - 1, 0), std::max((int)i.y - 35, 0)),
                        cv::Point2f(std::min((int)i.x + max_width, mat_img.cols - 1), std::min((int)i.y, mat_img.rows - 1)),
                        color, cv::FILLED, 8, 0);
          putText(mat_img, obj_name, cv::Point2f(i.x, i.y - 16), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);
          if (!coords_3d.empty())
            putText(mat_img, coords_3d, cv::Point2f(i.x, i.y - 1), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(0, 0, 0), 1);
        }
    }
}

// 検出枠を左から右へ並べ、車牌領域(class "plate")を除いた文字を連結する
QString licensePlateDialog::platesFromBoxes(const std::vector<bbox_t>& boxes) const
{
  std::vector<bbox_t> chars;
  for (const auto& b : boxes)
    {
      if (b.obj_id >= _classnames.size())
        continue;
      if (_classnames[b.obj_id] == "plate")  // ナンバープレートの枠自体は文字ではない
        continue;
      chars.push_back(b);
    }

  std::sort(chars.begin(), chars.end(),
            [](const bbox_t& a, const bbox_t& b) { return a.x < b.x; });

  QString plate;
  for (const auto& b : chars)
    plate += QString::fromLocal8Bit(_classnames[b.obj_id].c_str()); // 省略名(中国語)も考慮
  return plate.toUpper();
}

// _testImg を YOLO 認識し、車牌文字列を求めて精算画面へ
void licensePlateDialog::runDetectionAndShow()
{
  if (_testImg.empty() || !_detector)
    {
      QMessageBox::warning(this, QStringLiteral("認識エラー"),
                           QStringLiteral("画像またはモデルが準備できていません。"));
      return;
    }

  std::vector<bbox_t> boxes = _detector->detect(_testImg, 0.5f);
  draw_boxes(_testImg, boxes, _classnames);

  _resultStr = platesFromBoxes(boxes);
  ui->m_txtResult->setText(_resultStr);

  ui->stackedWidget->setCurrentIndex(payPage);
}

void licensePlateDialog::CaptureImage()
{
  // カメラが動いていれば実撮影(結果は ProcessCapturedImage に届く)
  if (_camera && _camera->status() == QCamera::ActiveStatus)
    {
      _imageCapture->capture();
      return;
    }

  // カメラが無い場合のデモ用フォールバック(テスト画像)
  _testImg = cv::imread((QDir::currentPath() + "/googletest/TestLicense.jpg").toStdString());
  runDetectionAndShow();
}

int licensePlateDialog::calcFee(qint64 minutes) const
{
  if (minutes <= 0)
    return FEE_MINIMUM;
  int hours = static_cast<int>((minutes + 59) / 60);   // 1時間未満は切り上げ
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
      QMessageBox::warning(this, QStringLiteral("入力エラー"),
                           QStringLiteral("ナンバーが空です。"));
      return;
    }

  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isOpen())
    {
      QMessageBox::critical(this, QStringLiteral("DBエラー"),
                            QStringLiteral("データベースを開けません。"));
      return;
    }

  const QDateTime now = QDateTime::currentDateTime();
  const QString nowStr = now.toString(Qt::ISODate);

  if (_mode == Mode::Entry)
    {
      // 入場: 次の id を求めてレコードを追加
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
          QMessageBox::critical(this, QStringLiteral("DBエラー"), query.lastError().text());
          return;
        }
      _infoLabel->setText(QStringLiteral("✅ 入場を記録しました\nナンバー: %1\n入場時刻: %2")
                          .arg(plate, now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
    }
  else // 出場
    {
      const QString entryStr = findLatestEntryTime(plate);
      if (entryStr.isEmpty())
        {
          QMessageBox::warning(this, QStringLiteral("記録なし"),
              QStringLiteral("ナンバー %1 の入場記録が見つかりません。").arg(plate));
          return;
        }
      const QDateTime entryTime = QDateTime::fromString(entryStr, Qt::ISODate);
      const qint64 minutes = entryTime.secsTo(now) / 60;
      const int fee = calcFee(minutes);

      // 精算済みのレコードを削除
      QSqlQuery del;
      del.prepare("DELETE FROM license WHERE licenseNum = :p");
      del.bindValue(":p", plate);
      del.exec();

      _infoLabel->setText(QStringLiteral("💰 出場\nナンバー: %1\n入場: %2\n出場: %3\n駐車時間: %4 分\n料金: %5 元")
                          .arg(plate,
                               entryTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                               now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                               QString::number(minutes),
                               QString::number(fee)));
    }

  // 次の車のために入力をクリアし、カメラ画面へ戻す
  _resultStr.clear();
  ui->m_txtResult->clear();
  ui->stackedWidget->setCurrentIndex(cameraPage);
}

licensePlateDialog::~licensePlateDialog()
{
  delete ui;
  delete  _imageCapture;
}
