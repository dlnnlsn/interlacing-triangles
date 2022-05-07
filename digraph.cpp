#include <boost/multiprecision/cpp_int.hpp>
#include <future>
#include <iostream>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace boost::multiprecision;
using namespace std;

const int num_threads = max<int>(thread::hardware_concurrency(), 1);

constexpr int digraph_index(int row, int col) {
    return (row * (row + 1)) / 2 + col;
}

void update_indegrees(int num_rows, int row, int col, bool increment, uint64_t digraph, int (*in_degree)[10]) {
    int to_add = increment ? 1 : -1;
    if (row < num_rows - 1) {
        if (digraph & (1 << digraph_index(row, col))) {
            in_degree[row + 1][col] += to_add;
        }
        else {
            in_degree[row + 1][col + 1] += to_add;
        }
    }
    if (row > 0) {
        if (col > 0) {
            if (digraph & (1 << digraph_index(row - 1, col - 1))) {
                in_degree[row - 1][col - 1] += to_add;
            }
        }
        if (col < row) {
            if ((digraph & (1 < digraph_index(row - 1, col))) == 0) {
                in_degree[row - 1][col] += to_add;
            }
        }
    }
}

int128_t count(int num_rows, uint64_t digraph, int left_to_visit, bool (*visited)[10], int (*in_degree)[10]) {
    if (left_to_visit == 0) return 1;
    int128_t total = 0;
    for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col <= row; ++col) {
            if (visited[row][col]) continue;
            if (in_degree[row][col] > 0) continue;

            visited[row][col] = true;
            update_indegrees(num_rows, row, col, false, digraph, in_degree);

            total += count(num_rows, digraph, left_to_visit - 1, visited, in_degree);

            update_indegrees(num_rows, row, col, true, digraph, in_degree);
            visited[row][col] = false;
        }
    }

    return total;
}

int128_t count(int num_rows, uint64_t digraph) {
    bool visited[10][10];
    int in_degree[10][10];
    for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col <= row; ++col) {
            visited[row][col] = false;
            in_degree[row][col] = 0;
        }
    }

    for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col <= row; ++col) {
            update_indegrees(num_rows, row, col, true, digraph, in_degree);
        }
    }

    return count(num_rows, digraph, (num_rows * (num_rows + 1)) / 2, visited, in_degree);
}

void digraph_thread(int num_rows, uint64_t start, uint64_t end, promise<int128_t> result_promise) {
    int128_t total = 0;

    for (uint64_t digraph = start; digraph < end; ++digraph) {
        total += count(num_rows, digraph);
    }

    result_promise.set_value(total);
}

int main() {

    int num_rows; cin >> num_rows;

    uint64_t end = 1ull << (num_rows - 1);
    uint64_t block_size = max<uint64_t>(end / num_threads, 1);

    vector<future<int128_t>> futures;
    vector<thread> threads;

    for (uint64_t start = 0; start < end; start += block_size) {
        promise<int128_t> result_promise;
        futures.push_back(result_promise.get_future());
        threads.push_back(thread(digraph_thread, num_rows, start, start + block_size, move(result_promise))); 
    }

    int128_t triangles = 0;
    for (int i = 0; i < futures.size(); ++i) {
        triangles += futures[i].get();
        threads[i].join();
    }

    cout << triangles << endl;

    return 0;
}


