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
#include <yolo_v2_class.hpp>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtCore/QDateTime>
#include <qdir.h>

QT_BEGIN_NAMESPACE
namespace Ui { class licensePlateDialog; }
QT_END_NAMESPACE

// 入場 / 出場 モード
enum class Mode { Entry, Exit };

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
void draw_boxes(cv::Mat mat_img, std::vector<bbox_t> result_vec, std::vector<std::string> obj_names,
                int current_det_fps = -1, int current_cap_fps = -1);
cv::Scalar obj_id_to_color(int obj_id);

// 追加: 駐車場ロジック
void onModeChanged(int index);
void runDetectionAndShow();                                      // _testImg を YOLO 認識 -> 車牌文字列 -> 表示
QString platesFromBoxes(const std::vector<bbox_t>& boxes) const; // 検出枠を x 座標順に並べて文字列化
int  calcFee(qint64 minutes) const;                              // 駐車時間(分) -> 料金
QString findLatestEntryTime(const QString& plate);               // 入場記録の検索

private:
Ui::licensePlateDialog *ui = nullptr;
QCamera* _camera = nullptr;
QCameraViewfinder* _cameraViewFinder = nullptr;
QVBoxLayout* _layout = nullptr;
QCameraImageCapture* _imageCapture = nullptr;
QString _resultStr = "";
Detector* _detector = nullptr;
std::string _cur_path = "";
std::string _cfg = "";
std::string _weight = "";
std::string _labels = "";
std::vector<std::string> _classnames {};
cv::Mat _testImg;

// 追加: 駐車場システム用
QComboBox* _modeCombo = nullptr;            // 入場 / 出場 切替
QLabel*    _infoLabel = nullptr;            // 案内・結果表示
Mode       _mode = Mode::Entry;
};
#endif // LICENSEPLATEDIALOG_H
