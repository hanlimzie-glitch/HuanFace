#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include "MainWindow.h"
#undef min
#undef max
#undef OPAQUE
#undef TRANSPARENT

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QResizeEvent>

#include <chrono>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("HuanFace Camera 1.0.0 — Beauty Camera");
    resize(1280, 800);

    adapter_.Initialize();
    cameraManager_.Initialize();

    SetupUI();

    cameras_ = cameraManager_.EnumerateDevices();
    cameraCombo_->clear();
    if (cameras_.empty()) {
        cameraCombo_->addItem("No camera detected");
        statusLabel_->setText("No camera detected — check connection");
    } else {
        for (auto& cam : cameras_) {
            cameraCombo_->addItem(QString::fromStdString(cam.name));
        }
        statusLabel_->setText(QString("Found %1 cameras").arg(cameras_.size()));
    }

    // Resolutions
    resolutionCombo_->clear();
    resolutionCombo_->addItem("640x480");
    resolutionCombo_->addItem("1280x720");
    resolutionCombo_->addItem("1920x1080");
    resolutionCombo_->setCurrentIndex(1); // 720p default

    mirrorCheck_->setChecked(true);
    adapter_.SetMirror(true);

    // Timer for UI update ~60 FPS
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::OnUpdateTimer);
    updateTimer_->start(16);

    // Processing thread
    stopProcessing_ = false;
    processingThread_ = std::thread(&MainWindow::ProcessingThreadFunc, this);

    // Auto start if camera exists
    if (!cameras_.empty()) {
        StartCamera();
    }
}

MainWindow::~MainWindow() {
    stopProcessing_ = true;
    if (processingThread_.joinable()) processingThread_.join();
    StopCamera();
    adapter_.Shutdown();
    renderer_.Shutdown();
    cameraManager_.Shutdown();
}

void MainWindow::SetupUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);

    // Left: preview
    QVBoxLayout* leftLayout = new QVBoxLayout();
    previewLabel_ = new QLabel(this);
    previewLabel_->setMinimumSize(640, 480);
    previewLabel_->setStyleSheet("background-color: #222; border: 1px solid #444;");
    previewLabel_->setAlignment(Qt::AlignCenter);
    previewLabel_->setText("Camera Preview\nMirror ON");
    previewLabel_->setScaledContents(false);
    leftLayout->addWidget(previewLabel_, 1);

    QHBoxLayout* statusLayout = new QHBoxLayout();
    statusLabel_ = new QLabel("Status: Idle", this);
    fpsLabel_ = new QLabel("FPS: 0", this);
    facesLabel_ = new QLabel("Faces: 0", this);
    statusLayout->addWidget(statusLabel_);
    statusLayout->addWidget(fpsLabel_);
    statusLayout->addWidget(facesLabel_);
    leftLayout->addLayout(statusLayout);

    mainLayout->addLayout(leftLayout, 3);

    // Right: controls
    QVBoxLayout* rightLayout = new QVBoxLayout();

    QGroupBox* camGroup = new QGroupBox("Camera", this);
    QVBoxLayout* camLayout = new QVBoxLayout(camGroup);
    camLayout->addWidget(new QLabel("Device:", this));
    cameraCombo_ = new QComboBox(this);
    camLayout->addWidget(cameraCombo_);
    camLayout->addWidget(new QLabel("Resolution:", this));
    resolutionCombo_ = new QComboBox(this);
    camLayout->addWidget(resolutionCombo_);
    mirrorCheck_ = new QCheckBox("Mirror (Selfie) ON", this);
    camLayout->addWidget(mirrorCheck_);
    captureButton_ = new QPushButton("📸 Capture Photo", this);
    captureButton_->setStyleSheet("background-color: #e74c3c; color: white; padding: 10px; font-weight: bold;");
    camLayout->addWidget(captureButton_);
    rightLayout->addWidget(camGroup);

    QGroupBox* beautyGroup = new QGroupBox("Beauty", this);
    QVBoxLayout* beautyLayout = new QVBoxLayout(beautyGroup);
    auto addSlider = [&](const QString& name, QSlider*& slider, int def=50){
        beautyLayout->addWidget(new QLabel(name, this));
        slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(0,100);
        slider->setValue(def);
        beautyLayout->addWidget(slider);
    };
    addSlider("Smoothing", smoothSlider_, 50);
    addSlider("Brightness", brightSlider_, 50);
    addSlider("Contrast", contrastSlider_, 50);
    addSlider("Retouch", retouchSlider_, 50);
    rightLayout->addWidget(beautyGroup);

    QGroupBox* makeupGroup = new QGroupBox("Makeup", this);
    QVBoxLayout* makeupLayout = new QVBoxLayout(makeupGroup);
    auto addMakeupSlider = [&](const QString& name, QSlider*& slider){
        makeupLayout->addWidget(new QLabel(name, this));
        slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(0,100);
        slider->setValue(0);
        makeupLayout->addWidget(slider);
    };
    addMakeupSlider("Lip", lipSlider_);
    addMakeupSlider("Blush", blushSlider_);
    addMakeupSlider("Eyebrow", eyebrowSlider_);
    rightLayout->addWidget(makeupGroup);

    rightLayout->addStretch();
    mainLayout->addLayout(rightLayout, 1);

    connect(cameraCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::OnCameraChanged);
    connect(resolutionCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::OnResolutionChanged);
    connect(mirrorCheck_, &QCheckBox::toggled, this, &MainWindow::OnMirrorToggled);
    connect(captureButton_, &QPushButton::clicked, this, &MainWindow::OnCaptureClicked);

    connect(smoothSlider_, &QSlider::valueChanged, this, &MainWindow::OnBeautySliderChanged);
    connect(brightSlider_, &QSlider::valueChanged, this, &MainWindow::OnBeautySliderChanged);
    connect(contrastSlider_, &QSlider::valueChanged, this, &MainWindow::OnBeautySliderChanged);
    connect(retouchSlider_, &QSlider::valueChanged, this, &MainWindow::OnBeautySliderChanged);

    connect(lipSlider_, &QSlider::valueChanged, this, &MainWindow::OnMakeupSliderChanged);
    connect(blushSlider_, &QSlider::valueChanged, this, &MainWindow::OnMakeupSliderChanged);
    connect(eyebrowSlider_, &QSlider::valueChanged, this, &MainWindow::OnMakeupSliderChanged);
}

void MainWindow::OnCameraChanged(int index) {
    if (index<0 || index >= (int)cameras_.size()) return;
    StopCamera();
    StartCamera();
}

void MainWindow::OnResolutionChanged(int index) {
    Q_UNUSED(index)
    if (cameraRunning_) {
        StopCamera();
        StartCamera();
    }
}

void MainWindow::OnMirrorToggled(bool checked) {
    adapter_.SetMirror(checked);
}

void MainWindow::OnCaptureClicked() {
    std::lock_guard<std::mutex> lock(imageMutex_);
    if (latestQImage_.isNull()) {
        QMessageBox::warning(this, "Capture", "No image to capture");
        return;
    }
    // Convert QImage to RGBA
    QImage img = latestQImage_.convertToFormat(QImage::Format_RGBA8888);
    std::string err;
    std::string path = captureManager_.SavePhoto(img.bits(), img.width(), img.height(), err);
    if (path.empty()) {
        QMessageBox::warning(this, "Capture Failed", QString::fromStdString(err));
    } else {
        statusLabel_->setText(QString("Saved: %1").arg(QString::fromStdString(path)));
    }
}

void MainWindow::OnBeautySliderChanged() {
    BeautyParams p;
    p.smoothing = smoothSlider_->value() / 100.0f;
    p.brightness = (brightSlider_->value() - 50) / 50.0f; // -1 to 1
    p.contrast = (contrastSlider_->value() - 50) / 50.0f;
    p.retouch = retouchSlider_->value() / 100.0f;
    adapter_.SetBeautyParams(p);
}

void MainWindow::OnMakeupSliderChanged() {
    MakeupParams p;
    p.lip = lipSlider_->value() / 100.0f;
    p.blush = blushSlider_->value() / 100.0f;
    p.eyebrow = eyebrowSlider_->value() / 100.0f;
    adapter_.SetMakeupParams(p);
}

void MainWindow::StartCamera() {
    if (cameras_.empty()) return;
    int camIdx = cameraCombo_->currentIndex();
    if (camIdx<0) camIdx=0;
    int resIdx = resolutionCombo_->currentIndex();
    int w=1280,h=720;
    if (resIdx==0) { w=640; h=480; }
    else if (resIdx==1) { w=1280; h=720; }
    else if (resIdx==2) { w=1920; h=1080; }

    cameraDevice_.SetFrameCallback([this](const CameraFrame& frame){
        std::lock_guard<std::mutex> lock(frameMutex_);
        if (frameQueue_.size() >= 2) {
            // Drop old
            std::queue<CameraFrame> empty;
            std::swap(frameQueue_, empty);
        }
        frameQueue_.push(frame);
    });

    if (cameraDevice_.Open(camIdx, w, h)) {
        cameraRunning_ = true;
        statusLabel_->setText(QString("Camera %1 running %2x%3").arg(camIdx).arg(w).arg(h));

        // Init D3D11 if not yet, using winId as HWND
        if (!d3dInitialized_) {
            // Need to ensure previewLabel has winId
            WId winId = previewLabel_->winId();
            if (renderer_.Initialize((void*)winId, previewLabel_->width(), previewLabel_->height())) {
                d3dInitialized_ = true;
            }
        }
    } else {
        statusLabel_->setText("Failed to open camera");
    }
}

void MainWindow::StopCamera() {
    if (cameraRunning_) {
        cameraDevice_.Close();
        cameraRunning_ = false;
        statusLabel_->setText("Camera stopped");
    }
}

void MainWindow::ProcessingThreadFunc() {
    while (!stopProcessing_) {
        CameraFrame frame;
        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            if (frameQueue_.empty()) {
                // No frame, sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            frame = frameQueue_.front();
            frameQueue_.pop();
        }

        // Process via adapter
        bool mirror = mirrorCheck_ ? mirrorCheck_->isChecked() : true;
        auto processed = adapter_.ProcessFrame(frame.data.data(), frame.width, frame.height, (int)frame.format, mirror);

        // Update QImage for preview
        {
            std::lock_guard<std::mutex> lock(imageMutex_);
            // RGBA to QImage
            QImage img(processed.rgba.data(), processed.width, processed.height, QImage::Format_RGBA8888);
            latestQImage_ = img.copy(); // deep copy

            // D3D11 render if initialized
            if (d3dInitialized_) {
                renderer_.UpdateInputTexture(processed.rgba.data(), processed.width, processed.height);
                renderer_.RenderFrame(mirror);
            }
        }

        faceCount_ = processed.faceCount;
        cameraFps_ = cameraDevice_.GetFPS();
        // Render FPS approx
        static auto last = std::chrono::steady_clock::now();
        static int cnt=0;
        cnt++;
        auto now = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now-last).count();
        if (diff>=1000) {
            renderFps_ = cnt*1000.0/diff;
            cnt=0;
            last=now;
        }
    }
}

void MainWindow::OnUpdateTimer() {
    // Update preview QLabel from latestQImage_ if D3D not used or as fallback
    std::lock_guard<std::mutex> lock(imageMutex_);
    if (!latestQImage_.isNull()) {
        QImage scaled = latestQImage_.scaled(previewLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (mirrorCheck_ && mirrorCheck_->isChecked()) {
            scaled = scaled.mirrored(true,false);
        }
        previewLabel_->setPixmap(QPixmap::fromImage(scaled));
    }
    fpsLabel_->setText(QString("Cam: %1 FPS Ren: %2 FPS").arg((int)cameraFps_.load()).arg((int)renderFps_.load()));
    facesLabel_->setText(QString("Faces: %1").arg(faceCount_.load()));
}
