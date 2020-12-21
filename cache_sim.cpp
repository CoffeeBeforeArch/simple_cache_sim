// A simple cache simulator implemented in C++
// By: Nick from CoffeeBeforeArch

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <vector>

// Our cache simulator
class CacheSim {
 public:
  // Constructor
  CacheSim(std::string input, unsigned bs, unsigned a, unsigned c, unsigned mp,
           unsigned wbp) {
    // Initialize of input file stream object
    infile.open(input);

    // Set all of our cache settings
    block_size = bs;
    associativity = a;
    capacity = c;
    miss_penalty = mp;
    dirty_wb_penalty = wbp;

    // Calculate the number of blocks
    // Assume this divides evenly
    auto num_blocks = capacity / block_size;

    // Create our cache based on the number of blocks
    tags.resize(num_blocks);
    dirty.resize(num_blocks);
    valid.resize(num_blocks);
    priority.resize(num_blocks);

    // Calculate values for traversal
    // Cache lines come in the following format:
    // |****** TAG ******|**** SET ****|** OFFSET **|
    // Calculate the number of offset bits
    auto block_bits = std::__popcount(block_size - 1);

    // Calculate the number of set bits, and create a mask of 1s
    set_offset = block_bits;
    auto sets = capacity / (block_size * associativity);
    set_mask = sets - 1;
    auto set_bits = std::__popcount(set_mask);

    // Calculate the bit-offset for the tag and create a mask of 1s
    // Always use 64-bit addresses
    tag_offset = block_bits + set_bits;
  }

  // Run the simulation
  void run() {
    // Keep reading data from a file
    std::string line;
    while (std::getline(infile, line)) {
      // Get the data for the access
      auto [type, address, instructions] = parse_line(line);

      // Probe the cache
      auto [hit, dirty_wb, cycles] = probe(type, address);

      // Update the cache statistics
      update_stats(instructions, type, hit, dirty_wb, cycles);
    }
  }

  // Destructor
  ~CacheSim() {
    infile.close();
    dump_stats();
  }

 private:
  // Input trace file
  std::ifstream infile;

  // Cache settings
  unsigned block_size;
  unsigned associativity;
  unsigned capacity;
  unsigned miss_penalty;
  unsigned dirty_wb_penalty;

  // Access settings
  int set_offset;
  int tag_offset;
  int set_mask;

  // The actual cache state
  std::vector<std::uint64_t> tags;
  std::vector<char> dirty;
  std::vector<char> valid;
  std::vector<int> priority;

  // Cache statistics
  std::int64_t writes_ = 0;
  std::int64_t mem_accesses_ = 0;
  std::int64_t misses_ = 0;
  std::int64_t dirty_wb_ = 0;
  std::int64_t instructions_ = 0;
  std::int64_t cycles_ = 0;

  // Dump the statistics from simulation
  void dump_stats() {
    // Print the cache settings
    std::cout << "CACHE SETTINGS\n";
    std::cout << "       Cache Size (Bytes): " << capacity << '\n';
    std::cout << "           Associativity : " << associativity << '\n';
    std::cout << "       Block Size (Bytes): " << block_size << '\n';
    std::cout << "    Miss Penalty (Cycles): " << miss_penalty << '\n';
    std::cout << "Dirty WB Penalty (Cycles): " << dirty_wb_penalty << '\n';
    std::cout << '\n';

    // Print the access breakdown
    std::cout << "CACHE STATS\n";
    std::cout << "TOTAL ACCESSES: " << mem_accesses_ << '\n';
    std::cout << "         READS: " << mem_accesses_ - writes_ << '\n';
    std::cout << "        WRITES: " << writes_ << '\n';

    // Print the miss-rate breakdown
    double miss_rate = (double)misses_ / (double)mem_accesses_ * 100.0;
    auto hits = mem_accesses_ - misses_;
    std::cout << "     MISS-RATE: " << miss_rate << '\n';
    std::cout << "        MISSES: " << misses_ << '\n';
    std::cout << "          HITS: " << hits << '\n';

    // Print the instruction breakdown
    double ipc = (double)instructions_ / (double)cycles_;
    std::cout << "           IPC: " << ipc << '\n';
    std::cout << "  INSTRUCTIONS: " << instructions_ << '\n';
    std::cout << "        CYCLES: " << cycles_ << '\n';
    std::cout << "      DIRTY WB: " << dirty_wb_ << '\n';
  }

  // Get memory access from the trace file
  std::tuple<bool, std::uint64_t, int> parse_line(std::string access) {
    // What we want to parse
    int type;
    std::uint64_t address;
    int instructions;

    // Parse from the string we read from the file
    sscanf(access.c_str(), "# %d %lx %d", &type, &address, &instructions);

    return {type, address, instructions};
  }

  // Probe the cache
  std::tuple<bool, bool, std::int64_t> probe(bool type, std::uint64_t address) {
    // Calculate the set from the address
    auto set = get_set(address);
    auto tag = get_tag(address);

    // Create a spans for our set
    auto base = set * associativity;
    std::span local_tags{tags.data() + base, associativity};
    std::span local_dirty{dirty.data() + base, associativity};
    std::span local_valid{valid.data() + base, associativity};
    std::span local_priority{priority.data() + base, associativity};

    // Check each cache line in the set
    auto hit = false;
    int invalid_index = -1;
    int index;
    for (auto i = 0u; i < local_valid.size(); i++) {
      // Check if the block is invalid
      if (!valid[base + i]) {
        // Keep track of invalid entries in case we need them
        invalid_index = i;
        continue;
      }

      // Check if the tag matches
      if (tag != tags[base + i]) continue;

      // We found the line, so mark it as a hit and exit the loop
      hit = true;
      index = i;
      break;
    }

    // Find an element to replace if it wasn't a hit
    auto dirty_wb = false;
    if (!hit) {
      // Use an open cache line (if available)
      if (invalid_index >= 0) index = invalid_index;
      // Otherwise, use the lowest-priority cache block (highest priority value)
      else {
        auto max_element = std::ranges::max_element(local_priority);
        index = std::distance(begin(local_priority), max_element);
      }

      // Update the tag and see if it was a dirty writeback
      local_tags[index] = tag;
      dirty_wb = local_dirty[index];
    }

    // Update dirty flag and priority
    local_dirty[index] = type;
    std::transform(begin(local_priority), end(local_priority),
                   begin(local_priority), [&](int p) {
                     if (p <= local_priority[index] && p < associativity)
                       return p + 1;
                     else
                       return p;
                   });
    local_priority[index] = 0;

    // Calculate the cycles for this access
    // Each memory access is one cycle
    auto cycles = 0;
    // Add miss penalty
    if (!hit) cycles += miss_penalty;
    // Add dirty writeback penalty
    if (dirty_wb) cycles += dirty_wb_penalty;

    return {hit, dirty_wb, cycles};
  }

  // Extract the set number
  // Shift the set to the bottom then extract the set bits
  int get_set(std::uint64_t address) {
    auto shifted_address = address >> set_offset;
    return shifted_address & set_mask;
  }

  // Extract the tag
  // Shift the tag to the bottom
  // No need to use mask (tag is all upper remaining bits)
  std::uint64_t get_tag(std::uint64_t address) {
    auto shifted_address = address >> tag_offset;
    return shifted_address;
  }

  // Update the stats
  void update_stats(int instructions, bool type, bool hit, bool dirty_wb,
                    int cycles) {
    mem_accesses_++;
    writes_ += type;
    misses_ += !hit;
    instructions_ += instructions;
    dirty_wb_ += dirty_wb;
    cycles_ += cycles;
  }
};

int main(int argc, char *argv[]) {
  // Kill the program if we didn't get an input file
  assert(argc == 2);

  // File location
  std::string location(argv[1]);

  // Hard coded cache settings
  unsigned block_size = 1 << 4;
  unsigned associativity = 1 << 0;
  unsigned capacity = 1 << 14;
  unsigned miss_penalty = 30;
  unsigned dirty_wb_penalty = 2;

  // Create our simulator
  CacheSim simulator(location, block_size, associativity, capacity,
                     miss_penalty, dirty_wb_penalty);
  simulator.run();

  return 0;
}
