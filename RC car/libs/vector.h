#include <stdint.h>
#include <math.h>

typedef struct {
    int32_t x;
    int32_t y;
} Vec_32_t;

Vec_32_t transform(Vec_32_t v) {
    Vec_32_t sq_map = {(int16_t)(v.x + v.y), (int16_t)(v.x - v.y)};
    
    int16_t R_new = abs(sq_map.x) + abs(sq_map.y);
    int16_t R = int16_t(sqrt(pow(sq_map.x, 2) + pow(sq_map.y, 2)));

    return {R_new*sq_map.x/R, R_new*sq_map.y/R};
}
