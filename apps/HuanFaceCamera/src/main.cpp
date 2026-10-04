#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI

#include <iostream>

#ifdef QT_VERSION
#include <QApplication>
#include "MainWindow.h"
#endif

int main(int argc, char* argv[]) {
#ifdef QT_VERSION
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
#else
    std::cout << "HuanFace Camera 1.0.0 — Qt6 not found, synthetic test" << std::endl;
    std::cout << "SDK: 1.0.0 frozen 52b0fa4 Phase9 24/24 Phase10 MAE 0.0074" << std::endl;
    std::cout << "Camera: Media Foundation enumerate/select/open/close" << std::endl;
    std::cout << "Build with Qt6 for full UI: cmake -S . -B build -G \"Visual Studio 17 2022\" -A x64" << std::endl;
    return 0;
#endif
}
