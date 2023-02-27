#include "som_bgs.hh"

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <tuple>

int min(int a, int b) {
  if (a < b) return a;
  return b;
}

std::tuple<uint16_t, uint16_t, uint16_t, uint16_t, uint16_t> SOM_BGS::process_frame(
  uint8_t image[]) {

  frame_subdivision(image);

  if (!initialized) {
    init();
  }
  else {
    analyze_new_block();
  }

  ++time_step;

  return { fg_length, x_min, x_max, y_min, y_max };
}

void SOM_BGS::init() {
  int b_idx, n_idx;

  for (auto row{ 0 }; row < b_rows; ++row)
    for (auto col{ 0 }; col < b_cols; ++col) {
      b_idx = block_idx_to_flat(row, col, 0);

      for (auto neuron{ 0 }; neuron < neurons_per_block; ++neuron) {
        n_idx = neuron_idx_to_flat(row, col, neuron, 0);

        for (auto ch{ 0 }; ch < CHANNELS; ++ch)
          neurons[n_idx + ch] = new_block[b_idx + ch];
      }
    }

  initialized = true;
}

void SOM_BGS::frame_subdivision(const uint8_t frame[]) {
  for (auto i{ 0 }; i < b_rows * b_cols * CHANNELS; ++i)
    new_block[i] = 0;

  uint8_t b_row, b_col;
  int b_idx, im_idx;

  for (auto row{ 0 }; row < i_rows; ++row)
    for (auto col{ 0 }; col < i_cols; ++col) {

      std::tie(b_row, b_col) = image_to_block_coords(row, col);

      b_idx = block_idx_to_flat(b_row, b_col, 0);

      for (int ch{ 0 }; ch < CHANNELS; ++ch) {
        im_idx = ch + col * CHANNELS + row * i_cols * CHANNELS; 
        new_block[b_idx + ch] += frame[im_idx];
      }
    }

  // average out each channel in each block
  for (auto row{ 0 }; row < b_rows; ++row)
    for (auto col{ 0 }; col < b_cols; ++col) {


      b_idx = block_idx_to_flat(row, col, 0);

      for (auto ch{ 0 }; ch < CHANNELS; ++ch) {
        new_block[b_idx + ch] *= INT32_MAX / (UINT8_MAX * pix_per_block);
      }
    }
}

void SOM_BGS::analyze_new_block() {
  x_min = UINT16_MAX, y_min = UINT16_MAX;
  x_max = y_max = 0;
  fg_length = 0;

  int64_t qk;

  //Needs uint64 else it could overflow
  uint64_t diff;

  int b_idx, n_idx, winner_idx = 0;

  for (auto row{ 0 }; row < b_rows; ++row)
    for (auto col{ 0 }; col < b_cols; ++col) {
      qk = INT_FAST32_MAX;

      b_idx = block_idx_to_flat(row, col, 0);

      for (auto neuron{ 0 }; neuron < neurons_per_block; ++neuron) {
        n_idx = neuron_idx_to_flat(row, col, neuron, 0);
        diff = 0;

        for (auto ch{ 0 }; ch < CHANNELS; ++ch) {
          int64_t d = new_block[b_idx + ch] - neurons[n_idx + ch];
          diff += d * d;
        }
        diff = sqrt(diff);
        if (diff < qk) {
          qk = diff;
          winner_idx = neuron;
        }
      }

      // mark block as fg
      if (qk > THRESHOLD) {
        uint16_t upscaled_col = col * FACTOR;
        uint16_t upscaled_row = row * FACTOR;

        // Keep track of a bounding box which enclose every block with movement in it 
        
        x_min = std::min(x_min, upscaled_col);
        x_max = std::max(x_max, (uint16_t)(upscaled_col + FACTOR));

        y_min = std::min(y_min, upscaled_row);
        y_max = std::max(y_max, (uint16_t)(upscaled_row + FACTOR));

        ++fg_length;
      }

      // Update model
      update_model(row, col, winner_idx);
    }
}

void SOM_BGS::update_model(int row, int col, int winner_idx) {
  int b_idx = block_idx_to_flat(row, col, 0);

  int n_idx, n_val;

  for (auto i{ 0 }; i < neurons_per_block; ++i) {
    n_idx = neuron_idx_to_flat(row, col, i, 0);

    for (auto ch{ 0 }; ch < CHANNELS; ++ch) {
      n_val = neurons[n_idx + ch];

      neurons[n_idx + ch] = update_neuron(n_val, i, winner_idx, new_block[b_idx + ch]);
    }
  }
}

int32_t SOM_BGS::update_neuron(int32_t current, int current_idx, int winner_idx, int32_t block_value) {
  return current + (block_value - current) * learning_rate() * neighbour_func(current_idx, winner_idx);
}

double look_up_exponential(double val) {
  // TODO implement real look up
  return exp(val);
}

double SOM_BGS::neighbour_func(int neuron_idx, int winner_idx) {
  double dist = distance(neuron_idx, winner_idx) / neighbour_radius();
  dist *= dist;

  double ex = look_up_exponential(-dist);
  return ex;
}

double SOM_BGS::distance(int neuron1, int neuron2) {
  auto [n1r, n1c] = neuron_flat_to_lattice(neuron1);
  auto [n2r, n2c] = neuron_flat_to_lattice(neuron2);

  int r_diff = n1r - n2r;
  int c_diff = n1c - n2c;

  double dist = sqrt(r_diff * r_diff + c_diff * c_diff);

  return dist;
}

std::tuple<int, int> SOM_BGS::neuron_flat_to_lattice(int neuron) {
  int row = neuron / n_cols;
  int col = neuron % n_cols;

  return { row, col };
}
