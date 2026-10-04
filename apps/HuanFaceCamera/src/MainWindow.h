#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QImage>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <vector>

#include "CameraManager.h"
#include "CameraDevice.h"
#include "HuanFaceAdapter.h"
#include "CameraRenderer.h"
#include "CaptureManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent=nullptr);
    ~MainWindow();

private slots:
    void OnCameraChanged(int index);
    void OnResolutionChanged(int index);
    void OnMirrorToggled(bool checked);
    void OnCaptureClicked();
    void OnBeautySliderChanged();
    void OnMakeupSliderChanged();
    void OnUpdateTimer();

private:
    void SetupUI();
    void StartCamera();
    void StopCamera();
    void ProcessingThreadFunc();

    // UI
    QLabel* previewLabel_ = nullptr;
    QComboBox* cameraCombo_ = nullptr;
    QComboBox* resolutionCombo_ = nullptr;
    QCheckBox* mirrorCheck_ = nullptr;
    QPushButton* captureButton_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* fpsLabel_ = nullptr;
    QLabel* facesLabel_ = nullptr;

    QSlider* smoothSlider_ = nullptr;
    QSlider* brightSlider_ = nullptr;
    QSlider* contrastSlider_ = nullptr;
    QSlider* retouchSlider_ = nullptr;

    QSlider* lipSlider_ = nullptr;
    QSlider* blushSlider_ = nullptr;
    QSlider* eyebrowSlider_ = nullptr;

    QTimer* updateTimer_ = nullptr;

    // Camera
    CameraManager cameraManager_;
    CameraDevice cameraDevice_;
    HuanFaceAdapter adapter_;
    CameraRenderer renderer_;
    CaptureManager captureManager_;

    std::vector<CameraInfo> cameras_;
    bool cameraRunning_ = false;

    // Threading
    std::queue<CameraFrame> frameQueue_;
    std::mutex frameMutex_;
    std::thread processingThread_;
    std::atomic<bool> processing_{false};
    std::atomic<bool> stopProcessing_{false};

    // Latest processed
    QImage latestQImage_;
    std::mutex imageMutex_;
    std::atomic<int> faceCount_{0};
    std::atomic<double> cameraFps_{0.0};
    std::atomic<double> renderFps_{0.0};

    // For D3D11 rendering, we use HWND from winId()
    bool d3dInitialized_ = false;
};
