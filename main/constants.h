#ifndef CONSTANTS_H
#define CONSTANTS_H

#define WIDTH 1280
#define HEIGHT 720
// #define WIDTH 1600
// #define HEIGHT 1200
#define NUM_CHANNELS 3
#define BLOCK_SIZE 40
#define BLOCKS ((WIDTH * HEIGHT) / (BLOCK_SIZE * BLOCK_SIZE))
#define W (WIDTH / BLOCK_SIZE)
#define H (HEIGHT / BLOCK_SIZE)
#define BLOCK_DIFF_THRESHOLD 0.20
#define IMAGE_DIFF_THRESHOLD 0.002


#define DECREMENT_START 2
#define CROP_FACTOR 2.0

#define SLEEP_DURATION_SECONDS 43200 // 12 hours
#define WAKE_UP_HOUR 7
#define SLEEP_HOUR 19 // Time in Kenya

// Sweden timezone
// #define TIME_ZONE "GMT-2"
//Kenya timezone
#define TIME_ZONE "GMT-3"
#endif // CONSTANTS_H

// 2048x1536
