#include <stdint.h>
#include <tuple>

class SOM_BGS {
public:
  SOM_BGS(int _i_rows, int _i_cols, int _b_rows, int _b_cols, int _n_rows,
    int _n_cols, float _threshold)
    : i_rows{ _i_rows },
    i_cols{ _i_cols },
    b_rows{ _b_rows },
    b_cols{ _b_cols },
    n_rows{ _n_rows },
    n_cols{ _n_cols },
    THRESHOLD{ static_cast<int32_t>(_threshold * INT32_MAX) },
    FACTOR { static_cast<uint8_t>(i_rows / b_rows) } {

    int blocks_per_image = b_rows * b_cols;
    
    pix_per_block = (i_rows * i_cols) / (blocks_per_image);
    
    neurons_per_block = n_rows * n_cols;

    new_block = new int32_t[blocks_per_image * CHANNELS];
    neurons = new int32_t[blocks_per_image * CHANNELS * neurons_per_block];
  };

  ~SOM_BGS() {
    delete new_block;
    new_block = nullptr;

    delete neurons;
    neurons = nullptr;

  };

  // returns number of foreground blocks, x_min, x_max, y_min, y_max
  // of smallest enclosing bounding box
  std::tuple<uint16_t, uint16_t, uint16_t, uint16_t, uint16_t> process_frame(
    uint8_t image[]);

  void set_learning_rate(double _lr) {
    lr = _lr;
  }



private:
  int i_rows, i_cols;

  int pix_per_block, b_rows, b_cols, row_scale, col_scale;
  int neurons_per_block, n_rows, n_cols;
  int32_t* neurons;

  int32_t* new_block;

  uint16_t fg_length = 0, x_min, x_max, y_min, y_max;

  // max value of 32bit int as scaling factor
  const int32_t FIXED_POINT_SCALING_FACTOR{ INT32_MAX };

  // color channels
  const uint8_t CHANNELS{ 3 };

  const int32_t THRESHOLD;

  const uint8_t FACTOR;

  double lr;

  bool initialized{ false };
  int time_step = 0;

  void init();
  // return block from the buffer
  void frame_subdivision(const uint8_t frame[]);

  // return which blocks has anomalies/movement in them
  void analyze_new_block();

  // update the neurons from the new blocks
  void update_model(int row, int col, int winner_neuron);

  // neighbor calc thing
  double neighbour_func(int neuron_idx, int winner_neuron);

  // neighbor radius thing
  // TODO set a real neighbour radius decay f(n + 1) <= f(n)
  double neighbour_radius() {
    // std::cout << "neighbour_radius: " << 1 / time_step << "\n";
    // std::cout << "neighbour_radius: " << 1 / time_step << "\n";
    // return 1.0e-2 * INT32_MAX;
    return 1;
    // return 1 + INT32_MAX / (1 + time_step);
  }

  // TODO set a real learning rate
  double learning_rate() {
    return lr;
    // return 1.0e-6 * INT32_MAX;
    // return (INT32_MAX / 2) - time_step;
  }

  double distance(int neuron1, int neuron2);

  int32_t update_neuron(int32_t current, int neuron_idx, int winner_idx, int32_t block_value);

  std::tuple<uint8_t, uint8_t> image_to_block_coords(int r, int c) {
    return { r / (i_rows / b_rows), c / (i_cols / b_cols) };
  }

  int image_idx_to_flat(int row, int col, int channel) {
    return channel + col * CHANNELS +
      row * CHANNELS * i_cols;  //+ row * i_cols;
  }

  int block_idx_to_flat(int row, int col, int channel) {
    return channel + col * CHANNELS + row * CHANNELS * b_cols;  //* b_cols;
  }

  int neuron_idx_to_flat(int row, int col, int neur, int channel) {
    return channel + neur * CHANNELS + col * CHANNELS * neurons_per_block +
      row * CHANNELS * neurons_per_block * b_cols;
  }

  std::tuple<int, int> neuron_flat_to_lattice(int neuron);
};
