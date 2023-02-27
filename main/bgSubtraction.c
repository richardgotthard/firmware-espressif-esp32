/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "bgSubtraction.h"
#include "constants.h"

#define MAXIMUM(x, y) (((x) > (y)) ? (x) : (y))
#define MINIMUM(x, y) (((x) < (y)) ? (x) : (y))

static const char *TAG = "MAIN_FUNCTIONS";
uint16_t *prev_frame;
uint16_t *current_frame;
uint8_t *bg_image;

void crop_image_center(uint8_t *src, uint8_t *dst)
{
  int center_x = WIDTH / 2;

  int i_p1_x = center_x - HEIGHT / 2;
  int i_p1_y = 0;

  int i_p2_x = center_x + HEIGHT / 2;
  int i_p2_y = HEIGHT;

  int counter = 0;

  for (int i = 0; i < WIDTH * HEIGHT; i++)
  {
    uint16_t x = i % WIDTH;
    uint16_t y = floor(i / WIDTH);

    if (x >= i_p1_x && x < i_p2_x && y >= i_p1_y && y < i_p2_y)
    {
      dst[counter] = src[i];
      counter++;
    }
  }
}

void crop_image(uint8_t *src, uint8_t *dst, uint16_t changes, uint32_t *cropped_len, uint16_t *accumelated_x, uint16_t *accumelated_y)
{
  uint16_t diff_sum_x = 0;
  uint16_t diff_sum_y = 0;
  float mean_x = (float)*accumelated_x / changes;
  float mean_y = (float)*accumelated_y / changes;

  //uint8_t *bg_mask_binary = (uint8_t *)heap_caps_malloc(W * H, MALLOC_CAP_SPIRAM);

  for (int j = 0; j < W * H; j++)
  {
    if (bg_image[j] > 0)
    {
      //bg_mask_binary[j] = 255;
      diff_sum_x += abs(j % W - mean_x);
      diff_sum_y += abs(floor(j / W) - mean_y);
    }
    else
    {
      //bg_mask_binary[j] = 0;
    }
  }

  float variance_x = ((float)diff_sum_x / changes);
  float variance_y = ((float)diff_sum_y / changes);

  float half_width = MAXIMUM(variance_x, variance_y) * CROP_FACTOR;

  // Mult by BLOCK_SIZE to get pixel in original img
  mean_x *= BLOCK_SIZE;
  mean_y *= BLOCK_SIZE;
  half_width *= BLOCK_SIZE;
  half_width = MINIMUM(half_width, (float)(HEIGHT / 2));

  // Shift crop towards head
  mean_y -= 0.5 * half_width;

  float p1_x = (mean_x - half_width);
  float p1_y = (mean_y - half_width);

  float p2_x = (mean_x + half_width);
  float p2_y = (mean_y + half_width);

  // Shift if square is outside image border
  if (p1_x < 0)
  {
    p1_x = 0;
    p2_x = 2 * half_width;
  }

  if (p1_y < 0)
  {
    p1_y = 0;
    p2_y = 2 * half_width;
  }

  if (p2_x > WIDTH)
  {
    p2_x = WIDTH;
    p1_x = WIDTH - 2 * half_width;
  }

  if (p2_y > HEIGHT)
  {
    p2_y = HEIGHT;
    p1_y = HEIGHT - 2 * half_width;
  }

  int i_p1_x = (int)p1_x;
  int i_p1_y = (int)p1_y;

  int i_p2_x = (int)p2_x;
  int i_p2_y = (int)p2_y;

  if (i_p2_x - i_p1_x > i_p2_y - i_p1_y)
  {
    i_p2_y++;
  }
  else if (i_p2_x - i_p1_x < i_p2_y - i_p1_y)
  {
    i_p2_x++;
  }

  for (int i = 0; i < WIDTH * HEIGHT; i++)
  {
    uint16_t x = i % WIDTH;
    uint16_t y = floor(i / WIDTH);

    if (x >= i_p1_x && x < i_p2_x && y >= i_p1_y && y < i_p2_y)
    {
      dst[*cropped_len] = src[i];
      ++(*cropped_len);
    }
  }

  //heap_caps_free(bg_mask_binary);
}

uint16_t bg_subtraction()
{
  // *changes = 0;

  // *accumelated_x = 0;
  // *accumelated_y = 0;
	uint16_t changes = 0;
	uint16_t accumelated_x = 0;
	uint16_t accumelated_y = 0;

  for (int y = 0; y < H; y++)
  {
    for (int x = 0; x < W; x++)
    {
      uint16_t i = x + y * W;
      float current = current_frame[i];
      float prev = prev_frame[i];
      float delta = abs(current - prev) / prev;

      if (delta >= BLOCK_DIFF_THRESHOLD)
      {
        //changes += 1;
				changes++;
				// ++(*changes);
        bg_image[i] = DECREMENT_START;
        accumelated_x += i % W;
      	accumelated_y += (uint16_t)floor(i / W);
      }
      else
      {
        if (bg_image[i] > 0)
          bg_image[i] = bg_image[i] - 1;
      }
    }
  }
	return changes;
  // ESP_LOGI(TAG, "Changed %d", changes);
  // ESP_LOGI(TAG, "out of %d blocks", BLOCKS);
}

void update_frame()
{
  for (int y = 0; y < H; y++)
  {
    for (int x = 0; x < W; x++)
    {
      uint16_t i = x + y * W;
      prev_frame[i] = current_frame[i];
    }
  }
}

bool downscale(uint8_t *image)
{
  // set all 0s in current frame
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
    {
      uint16_t i = x + y * W;
      current_frame[i] = 0;
    }

  // down-sample image in blocks
  for (uint32_t i = 0; i < WIDTH * HEIGHT; i++)
  {
    const uint16_t x = i % WIDTH;
    const uint16_t y = floor(i / WIDTH);
    const uint8_t block_x = floor(x / BLOCK_SIZE);
    const uint8_t block_y = floor(y / BLOCK_SIZE);
    uint16_t j = block_x + block_y * W;
    const uint8_t pixel = image[i];

    // average pixels in block (accumulate)
    current_frame[j] += pixel;
  }

  // average pixels in block (rescale)
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
    {
      uint16_t i = x + y * W;
      current_frame[i] /= BLOCK_SIZE * BLOCK_SIZE;
    }

  return true;
}

void setup_mf() {
  ESP_ERROR_CHECK(nvs_flash_init());

  prev_frame = (uint16_t *)heap_caps_malloc(W * H * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  current_frame = (uint16_t *)heap_caps_malloc(W * H * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  bg_image = (uint8_t *)heap_caps_malloc(W * H * sizeof(uint8_t), MALLOC_CAP_SPIRAM);

  for (int i = 0; i < W * H; i++)
  {
    prev_frame[i] = 0;
    current_frame[i] = 0;
    bg_image[i] = 0;
  }
}
