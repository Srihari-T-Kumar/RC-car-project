#include <stdint.h>
#include <math.h>

typedef struct {
    int16_t x;
    int16_t y;
} Vec_16_t;

Vec_16_t transform(Vec_16_t v) {
    Vec_16_t sq_map = {(int16_t)(v.x + v.y), (int16_t)(v.x - v.y)};
    return sq_map;
}
