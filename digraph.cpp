#include <algorithm>
#include <atomic>
#include <boost/multiprecision/cpp_int.hpp>
#include <future>
#include <iostream>
#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace boost::multiprecision;
using namespace std;

constexpr int min_batch_size = 4;
const int max_threads = max<int>(thread::hardware_concurrency(), 1);
atomic<int> num_threads;

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

bool find_linear_extension(int num_rows, uint64_t digraph, int num_visited, bool visited[10][10], int in_degree[10][10], int linear_extension[100]) {
    if (num_visited == ((num_rows * (num_rows + 1)) / 2)) return true;
    for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col <= row; ++col) {
            if (visited[row][col]) continue;
            if (in_degree[row][col]) continue;

            visited[row][col] = true;
            update_indegrees(num_rows, row, col, false, digraph, in_degree);
            linear_extension[digraph_index(row, col)] = num_visited;
            return find_linear_extension(num_rows, digraph, num_visited + 1, visited, in_degree, linear_extension);
        }
    }
    return false;
}

int128_t sage_count_linear_extensions(int num_rows, uint64_t digraph) {
    int in_degree[10][10];
    bool visited[10][10];
    int linear_extension[100];

    for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col <= row; ++col) {
            in_degree[row][col] = 0;
            visited[row][col] = false;
        }
    }

    for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col <= row; ++col) {
            update_indegrees(num_rows, row, col, true, digraph, in_degree);
        }
    }

    if (!find_linear_extension(num_rows, digraph, 0, visited, in_degree, linear_extension)) {
        return 0;
    }

    int n = (num_rows * (num_rows + 1)) / 2;
    vector<vector<int>> up(n, vector<int>{});

    int num = 0;
    for (int row = 0; row < num_rows - 1; ++row) {
        for (int col = 0; col <= row; ++col) {
            if (digraph & (1 << num)) {
                up[linear_extension[num]].push_back(linear_extension[num + row + 1]);
                up[linear_extension[num + row + 2]].push_back(linear_extension[num]);
            }
            else {
                up[linear_extension[num]].push_back(linear_extension[num + row + 2]);
                up[linear_extension[num + row + 1]].push_back(linear_extension[num]);
            }
            ++num;
        }
    }
    
    for (int i = n - 1; i >= 0; --i) {
        set<int> new_up(up[i].begin(), up[i].end());
        for (const int x : up[i]) {
            new_up.insert(up[x].begin(), up[x].end());
        }
        up[i] = vector<int>(new_up.begin(), new_up.end());
    }

    unordered_map<int, vector<int>> Jup;
    Jup[1] = vector<int>{};

    vector<int> loc(n, 1);

    int m = 1;

    for (int x = 0; x < n; ++x) {
        set<int> curr_k{loc[x]};
        set<int> flat_k_set{loc[x]};
        while (curr_k.size() > 0) {
            set<int> new_k;
            for (const int a : curr_k) {
                new_k.insert(Jup[a].begin(), Jup[a].end());
            }
            flat_k_set.insert(new_k.begin(), new_k.end());
            curr_k = move(new_k);
        }
        vector<int> flat_k = vector<int>(flat_k_set.begin(), flat_k_set.end());
        for (int j = 0; j < flat_k.size(); ++j) {
            int i = m + j + 1;
            vector<int> jup_i;
            jup_i.reserve(Jup[flat_k[j]].size());
            for (const int a : Jup[flat_k[j]]) {
                jup_i.push_back(m + lower_bound(flat_k.begin(), flat_k.end(), a) - flat_k.begin() + 1);
            }
            Jup[i] = jup_i;
            Jup[flat_k[j]].push_back(i);
        }
        for (const int y : up[x]) {
            loc[y] = lower_bound(flat_k.begin(), flat_k.end(), loc[y]) - flat_k.begin() + m + 1;
        }
        m += flat_k.size();
    }
    int128_t ct = 0;
    unordered_map<int, int128_t> Jup_ct;
    Jup_ct[m] = 1;
    while (m > 1) {
        m -= 1;
        ct = 0;
        for (const int j : Jup[m]) {
            ct += Jup_ct[j];
        }
        Jup_ct[m] = ct;
    }
    return ct;
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
        if (end - digraph >= 2 * min_batch_size && num_threads < max_threads) {
            promise<int128_t> left_promise;
            promise<int128_t> right_promise;

            future<int128_t> left_future = left_promise.get_future();
            future<int128_t> right_future = right_promise.get_future();

            uint64_t midpoint = (digraph + end) / 2;

            num_threads--;
            thread left_thread = thread(digraph_thread, num_rows, digraph, midpoint, move(left_promise));
            thread right_thread = thread(digraph_thread, num_rows, midpoint, end, move(right_promise));

            total += left_future.get();
            total += right_future.get();

            left_thread.join();
            right_thread.join();

            result_promise.set_value(total);
            return;
        }
        total += sage_count_linear_extensions(num_rows, digraph);   
    }

    result_promise.set_value(total);
    num_threads--;
}

int main() {

    int num_rows; cin >> num_rows;

    uint64_t end = 1ull << ((num_rows * (num_rows - 1)) / 2 - 1);

    num_threads = 0;

    promise<int128_t> result_promise;
    future<int128_t> result_future = result_promise.get_future();
    thread computation_thread(digraph_thread, num_rows, 0, end, move(result_promise));

    int128_t triangles = 2 * result_future.get();
    computation_thread.join();

    cout << triangles << endl;

    return 0;
}


