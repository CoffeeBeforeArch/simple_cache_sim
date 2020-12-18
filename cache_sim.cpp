// A simple cache simulator implemented in C++
// By: Nick from CoffeeBeforeArch

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

class CacheSim {
 public:
  // Constructor
  CacheSim(std::string input, int bs, int a, int c) {
    // Initialize of input file stream object
    infile.open(input);

    // Set all of our cache settings
    block_size = bs;
    associativity = a;
    capacity = c;

    // Calculate the number of blocks
    // Assume this divides evenly
    auto num_blocks = capacity / block_size;

    // Create our cache based on the number of blocks
    tags.resize(num_blocks);
    dirty.resize(num_blocks);
    valid.resize(num_blocks);
    priority.resize(num_blocks);
  }

  // Run the simulation
  void run() {}

  // Destructor
  ~CacheSim() { infile.close(); }

 private:
  // Input trace file
  std::ifstream infile;

  // Cache settings
  int block_size;
  int associativity;
  int capacity;

  // The actual cache state
  std::vector<std::uint64_t> tags;
  std::vector<bool> dirty;
  std::vector<bool> valid;
  std::vector<int> priority;

  // Cache statistics
  std::int64_t writes = 0;
  std::int64_t reads = 0;
  std::int64_t misses = 0;
  std::int64_t dirty_wb = 0;
  std::int64_t instructions = 0;
  std::int64_t cycles = 0;

  // Dump the statistics from simulation
  void dump_stats() {
    // Print the access breakdown
    auto total_accesses = writes + reads;
    std::cout << "TOTAL ACCESSES: " << total_accesses << '\n';
    std::cout << "   READS: " << writes + reads << '\n';
    std::cout << "  WRITES: " << writes + reads << '\n';

    // Print the miss-rate breakdown
    double miss_rate = misses / total_accesses * 100.0;
    auto hits = total_accesses - misses;
    std::cout << "MISS-RATE: " << miss_rate << '\n';
    std::cout << "  MISSES: " << misses << '\n';
    std::cout << "    HITS: " << hits << '\n';

    // Print the instruction breakdown
    double ipc = instructions / cycles;
    std::cout << "IPC: " << ipc << '\n';
    std::cout << "  INSTRUCTIONS: " << instructions << '\n';
    std::cout << "        CYCLES: " << cycles << '\n';
    std::cout << "      DIRTY WB: " << dirty_wb << '\n';
  }

  // Get memory access from the trace file
  std::tuple<bool, std::int64_t, int> parse_line(std::string access) {
    return {0, 0, 0};
  }
};

int main(int argc, char *argv[]) {
  // Kill the program if we didn't get an input file
  assert(argc < 2);

  // File location
  std::string location(argv[1]);

  // Hard coded cache settings
  int block_size = 1 << 6;
  int associativity = 1 << 3;
  int capacity = 1 << 15;

  // Create our simulator
  CacheSim simulator(location, block_size, associativity, capacity);

  return 0;
}
