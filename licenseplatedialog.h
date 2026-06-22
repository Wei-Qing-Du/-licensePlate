#ifndef LICENSEPLATEDIALOG_H
#define LICENSEPLATEDIALOG_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtMultimedia/QCamera>
#include <QtMultimediaWidgets/QCameraViewfinder>
#include <QtWidgets/QVBoxLayout>
#include <QtMultimedia/QCameraInfo>
#include <QtMultimedia/QCameraImageCapture>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtCore/QDateTime>
#include <qdir.h>

QT_BEGIN_NAMESPACE
namespace Ui { class licensePlateDialog; }
QT_END_NAMESPACE

enum class Mode { Entry, Exit };

// Detection result produced by YOLOv8 ONNX inference
struct Detection
{
  int   classId = 0;
  float confidence = 0.f;
  cv::Rect box;
};

class licensePlateDialog : public QDialog
{
Q_OBJECT

public:
explicit licensePlateDialog(QWidget *parent = nullptr);
~licensePlateDialog();
private slots:
void ProcessCapturedImage(int requestId, QImage img);
private:
void StartCamera();
void StopCamera();
void CaptureImage();
void PaymentBtnClicked();
void backSpaceforTXT();
void clearTXT();
void OK2Pay();
void draw_boxes(cv::Mat& mat_img, const std::vector<Detection>& result_vec,
                const std::vector<std::string>& obj_names);
cv::Scalar obj_id_to_color(int obj_id);

void onModeChanged(int index);
void runDetectionAndShow();
QString platesFromBoxes(const std::vector<Detection>& boxes) const;
int  calcFee(qint64 minutes) const;
QString findLatestEntryTime(const QString& plate);

// YOLOv8 ONNX inference
std::vector<Detection> detect(const cv::Mat& image,
                              float confThreshold = 0.25f,
                              float nmsThreshold  = 0.45f);

private:
Ui::licensePlateDialog *ui = nullptr;
QCamera* _camera = nullptr;
QCameraViewfinder* _cameraViewFinder = nullptr;
QVBoxLayout* _layout = nullptr;
QCameraImageCapture* _imageCapture = nullptr;
QString _resultStr = "";

cv::dnn::Net _net;
bool         _netReady = false;
int          _inputSize = 640;        // YOLOv8 default input size
std::string  _cur_path = "";
std::string  _modelPath = "";
std::string  _labels = "";
std::vector<std::string> _classnames {};
cv::Mat _testImg;

QComboBox* _modeCombo = nullptr;
QLabel*    _infoLabel = nullptr;
Mode       _mode = Mode::Entry;
};
#endif // LICENSEPLATEDIALOG_H
