#include "app.h"

#include <QPixmap>
#include <QKeyEvent>
#include <QPainter>
#include "windows.h"

#include <iostream>


App::App(QWidget *parent) : QWidget{parent}
{
    setFixedSize(canvasWidth, canvasHeight);

    pixmap = QPixmap(canvasWidth, canvasHeight);
    pixmap.fill(Qt::white);

    inter = new Chip8Interpreter();
    connect(this, &App::KeyDown, inter, &Chip8Interpreter::KeyDown);
    connect(this, &App::KeyUp, inter, &Chip8Interpreter::KeyUp);
    connect(inter, &Chip8Interpreter::draw, this, &App::draw);
    connect(inter, &Chip8Interpreter::beep, this, &App::beep);


    std::cout << "Loading ROM." << std::endl;
    // inter->Load("E:\\Temp\\Temp\\IBM Logo.ch8");
    inter->Load("E:\\Temp\\Temp\\Breakout (Brix hack) [David Winter, 1997].ch8");
    std::cout << "ROM Loaded." << std::endl;

    inter->start();
}

void App::closeEvent(QCloseEvent *event) {
    inter->exit();
}

void App::keyPressEvent(QKeyEvent* event) {
    auto k = event->key();
    if(keyMap.find(k) != keyMap.end()) {
        emit KeyDown(keyMap[k]);
        std::cout << "key pressed: " << k << std::endl;
    }
}

void App::keyReleaseEvent(QKeyEvent* event) {
    auto k = event->key();
    if(keyMap.find(k) != keyMap.end()) {
        emit KeyUp(keyMap[k]);
        // std::cout << "key released: " << k << std::endl;
    }
}

void App::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);
}

void App::draw(std::array<std::array<bool, Chip8Interpreter::screenWidth>, Chip8Interpreter::screenHeight> buffer) {
    int height = Chip8Interpreter::screenHeight;
    int width = Chip8Interpreter::screenWidth;

    QPainter painter(&pixmap);
    Qt::GlobalColor color = Qt::white;
    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            if (buffer[x][y]) {
                color = Qt::black;
            } else {
                color = Qt::white;
            }
            QRect rect((y*10) % canvasWidth, (x*10) % canvasHeight, 10, 10);
            painter.fillRect(rect, color);
        }
    }

    update();
}

void App::beep(int milliseconds) {
    Beep(500, milliseconds);
}
