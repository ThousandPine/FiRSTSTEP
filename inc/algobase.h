#pragma once

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ALIGN_UP(x, align) (((x) + (align) - 1) / (align) * (align))
#define ALIGN_DOWN(x, align) ((x) / (align) * (align))