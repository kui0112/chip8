#ifndef APP_H
#define APP_H

#include <map>
#include <Qt>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QCloseEvent>
#include "Chip8Interpreter.h"




class App : public QWidget
{
    Q_OBJECT
public:
    const static int canvasWidth = 640;
    const static int canvasHeight = 320;

    //
    // Original               Emulator
    //
    // 1 2 3 C                1 2 3 4
    // 4 5 6 D                Q W E R
    // 7 8 9 E                A S D F
    // A 0 B F                Z X C V
    //
    std::map<int, int> keyMap {
        {Qt::Key_1, 0x1},
        {Qt::Key_2, 0x2},
        {Qt::Key_3, 0x3},
        {Qt::Key_4, 0xC},

        {Qt::Key_Q, 0x4},
        {Qt::Key_W, 0x5},
        {Qt::Key_E, 0x6},
        {Qt::Key_R, 0xD},

        {Qt::Key_A, 0x7},
        {Qt::Key_S, 0x8},
        {Qt::Key_D, 0x9},
        {Qt::Key_F, 0xE},

        {Qt::Key_Z, 0xA},
        {Qt::Key_X, 0x0},
        {Qt::Key_C, 0xB},
        {Qt::Key_V, 0xF},
    };

    explicit App(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent * event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
private:
    QPixmap pixmap;
    Chip8Interpreter* inter;
    QThread* thread;

signals:
    void KeyDown(int key);
    void KeyUp(int key);

public slots:
    void draw(std::array<std::array<bool, Chip8Interpreter::screenWidth>, Chip8Interpreter::screenHeight> buffer);
    void beep(int milliseconds);
};

#endif // APP_H
