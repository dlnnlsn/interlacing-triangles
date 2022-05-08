#include <atomic>
#include <boost/multiprecision/cpp_int.hpp>
#include <future>
#include <iostream>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace boost::multiprecision;
using namespace std;

const int max_threads = max<int>(thread::hardware_concurrency(), 1);
atomic<int> num_threads;

constexpr int min_batch_size = 4;

constexpr int digraph_index(int row, int col) {
    return (row * (row + 1)) / 2 + col;
}

void update_indegrees(int num_rows, int row, int col, bool increment, uint64_t digraph, int in_degree[10][10]) {
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
            if ((digraph & (1 << digraph_index(row - 1, col))) == 0) {
                in_degree[row - 1][col] += to_add;
            }
        }
    }
}

bool has_any_topological_orders(int num_rows, uint64_t digraph, int left_to_visit, bool visited[10][10], int in_degree[10][10]) {
    if (left_to_visit == 0) return true;

    for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col <= row; ++col) {
            if (visited[row][col]) continue;
            if (in_degree[row][col] > 0) continue;

            visited[row][col] = true;
            update_indegrees(num_rows, row, col, false, digraph, in_degree);
            bool result = has_any_topological_orders(num_rows, digraph, left_to_visit - 1, visited, in_degree);
            update_indegrees(num_rows, row, col, true, digraph, in_degree);
            visited[row][col] = false;
            return result; 
        }
    }
    return false;
}

int128_t count(int num_rows, uint64_t digraph, int left_to_visit, bool visited[10][10], int in_degree[10][10]) {
    if (left_to_visit == 0) {
        return 1;
    }
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

    if (!has_any_topological_orders(num_rows, digraph, (num_rows * (num_rows + 1)) / 2, visited, in_degree)) {
        return 0;
    }
    return count(num_rows, digraph, (num_rows * (num_rows + 1)) / 2, visited, in_degree);
}

void digraph_thread(int num_rows, uint64_t start, uint64_t end, promise<int128_t> result_promise) {
    num_threads++;
    int128_t total = 0;

    for (uint64_t digraph = start; digraph < end; ++digraph) {
        if ((end - digraph >= (2 * min_batch_size)) && (num_threads < max_threads)) {
            promise<int128_t> left_promise;
            promise<int128_t> right_promise;
            future<int128_t> left_future = left_promise.get_future();
            future<int128_t> right_future = right_promise.get_future();

            num_threads--;

            uint64_t midpoint = (digraph + end) / 2;
            thread left_thread = thread(digraph_thread, num_rows, digraph, midpoint, move(left_promise));
            thread right_thread = thread(digraph_thread, num_rows, midpoint, end, move(right_promise));

            total += left_future.get();
            total += right_future.get();

            left_thread.join();
            right_thread.join();

            result_promise.set_value(total);
            return;
        } 
        total += count(num_rows, digraph);
    }

    num_threads--;
    result_promise.set_value(total);
}

int main() {

    int num_rows; cin >> num_rows;

    uint64_t end = 1ull << ((num_rows * (num_rows - 1)) / 2 - 1);

    promise<int128_t> result_promise;
    future<int128_t> result_future = result_promise.get_future();

    num_threads = 0;
    thread computation_thread = thread(digraph_thread, num_rows, 0, end, move(result_promise));

    int128_t triangles = 2 * result_future.get();
    computation_thread.join();

    cout << triangles << endl;

    return 0;
}


