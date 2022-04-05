/**
 * @file sieve_direct_fun.cpp
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2022 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * Demo program for benchmarking async frameworks: sieve of Eratosthenes,
 * free function direct calling version.
 *
 * The functions in this implementation are chained together by essentially
 * calling each other directly in a chain.  See the definition of "task" below.
 */

#include <atomic>
#include <future>
#include "sieve.hpp"
#include "sieve_fun.hpp"
#include "timer.hpp"

/**
 * Main sieve function
 *
 * @brief Generate primes from 2 to n using sieve of Eratosthenes.
 * @tparam bool_t the type to use for the "bitmap"
 * @param n upper bound of sieve
 * @param block_size how many primes to search for given a base set of primes
 */
template <class bool_t>
auto sieve_direct_block(size_t n, size_t block_size) {
  size_t sqrt_n = static_cast<size_t>(std::ceil(std::sqrt(n)));

  /* Generate base set of sqrt(n) primes to be used for subsequent sieving */
  auto first_sieve = sieve_seq<bool_t>(sqrt_n);
  std::vector<size_t> base_primes = sieve_to_primes(first_sieve);

  /* Store vector of list of primes (each list generated by separate task chain) */
  std::vector<std::shared_ptr<std::vector<size_t>>> prime_list(n / block_size + 2);
  prime_list[0] = std::make_shared<std::vector<size_t>>(base_primes);

  /**
   * Task containing chain of coroutine calls for generating primes
   * @param cc::executor_tag tells the runtime system to launch in parallel
   */
  input_body gen{};

#if 0
  auto task = [&]() {
    output_body(
      sieve_to_primes_part<bool_t>(
        range_sieve<bool_t>(
	  gen_range<bool_t>(
            gen(), block_size, sqrt_n, n
			    ), base_primes),), prime_list); };

#else
  auto task = [&]() {
    auto a = gen();
    auto b = gen_range<bool_t>(a, block_size, sqrt_n, n);
    auto c = range_sieve<bool_t>(b, base_primes);
    auto d = sieve_to_primes_part<bool_t>(c);
    output_body(d, prime_list);
  };
#endif

  /**
   * Launch tasks, each of which computes a  block of primes
   */
  std::vector<std::future<void>> tasks;
  for (size_t i = 0; i < n / block_size + 1; ++i) {
    tasks.emplace_back(std::async(std::launch::async, task));
  }

  /**
   * Wait for tasks to complete
   */
  for (size_t i = 0; i < n / block_size + 1; ++i) {
    tasks[i].wait();
  }

  return prime_list;
}

int main(int argc, char* argv[]) {
  size_t number = 100'000'000;
  size_t block_size = 1'000;

  if (argc >= 2) {
    number = std::stol(argv[1]);
  }
  if (argc >= 3) {
    block_size = std::stol(argv[2]);
  }

  auto using_bool_direct_block = timer_2(sieve_direct_block<bool>, number, block_size * 1024);
  auto using_char_direct_block = timer_2(sieve_direct_block<char>, number, block_size * 1024);

  std::cout << "Time using bool direct block: " << duration_cast<std::chrono::milliseconds>(using_bool_direct_block).count()
            << "\n";
  std::cout << "Time using char direct block: " << duration_cast<std::chrono::milliseconds>(using_char_direct_block).count()
            << "\n";

  return 0;
}
