#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include<QString>


QString ToHex(uint16_t num) {
    return QString("%1").arg(num, 4, 16, QChar('0')).toUpper();
}

#endif // UTILS_H
