#include <algorithm>
#include <future>
#include <iostream>
#include <vector>

using namespace std;

class interlacing_thread {
private:
    int values[10][10];
    pair<int, int> positions[100];
    int num_rows;
    int max_num;

    void set_value(pair<int, int> position, int value) {
        this->values[position.first][position.second] = value;
        this->positions[value] = position;
    }

    void set_value(int row, int col, int value) {
        this->values[row][col] = value;
        this->positions[value] = pair<int, int>{row, col};
    }

    pair<int, int> find_move(int to_move) {
        pair<int, int> max_pos{-1, -1};
        pair<int, int> current_pos = this->positions[to_move];
        for (int replaces = 2; replaces < to_move; ++replaces) {
            pair<int, int> new_pos = this->positions[replaces];
            if (new_pos > current_pos) continue;
            if (new_pos.first == this->num_rows - 1) {
                max_pos = max(max_pos, new_pos);
            }
            else {
                if ((to_move >= this->values[new_pos.first + 1][new_pos.second])
                 != (to_move >= this->values[new_pos.first + 1][new_pos.second + 1])) {
                    max_pos = max(max_pos, new_pos);
                }
            }
        }
        return max_pos;
    }

    bool next_candidate() {
        int to_move = -1;
        pair<int, int> new_pos;
        for (int candidate = 3; candidate <= this->max_num - 2; ++candidate) {
            new_pos = this->find_move(candidate);
            if (new_pos.first != -1) {
                to_move = candidate;
                break;
            }
        }
        if (to_move == -1) return false;
        
        pair<int, int> positions[100];
        int num_positions = 0;

        for (int i = 2; i <= to_move; ++i) {
            if (this->positions[i] == new_pos) continue;
            positions[num_positions++] = this->positions[i];
        }
        sort(positions, positions + num_positions);

        this->set_value(new_pos, to_move);
        for (int num = 2; num < to_move; ++num) {
            this->set_value(positions[num - 2], num);
        }

        return true;
    }

    bool valid() {
        for (int row = 0; row < this->num_rows - 1; ++row) {
            for (int col = 0; col <= row; ++col) {
                if ((this->values[row][col] > this->values[row + 1][col])
                 == (this->values[row][col] > this->values[row + 1][col + 1])) {
                    return false;
                }
            }
        }
        return true;
    }

public:
    interlacing_thread(int num_rows, pair<int, int> fixed_positions[4]) {
        this->num_rows = num_rows;
        const int max_num = (num_rows * (num_rows + 1)) / 2 - 1;
        this->max_num = max_num;

        int num = max_num;

        this->set_value(fixed_positions[0], 0);
        this->set_value(fixed_positions[1], 1);
        this->set_value(fixed_positions[2], num--);
        this->set_value(fixed_positions[3], num--);

        sort(fixed_positions, fixed_positions + 4);

        int fixed_index = 3;
        pair<int, int> fixed_position = fixed_positions[fixed_index];

        for (int row = num_rows - 1; row >= 0; --row) {
            for (int col = row; col >= 0; --col) {
                if (row == fixed_position.first && col == fixed_position.second) {
                    --fixed_index;
                    if (fixed_index >= 0) fixed_position = fixed_positions[fixed_index];
                    continue;
                }        
                this->set_value(row, col, num--);
            }
        }
    }

    void operator()(promise<int64_t> result_promise) {
        int64_t total = this->valid() ? 1 : 0;
        while (this->next_candidate()) {
            if (this->valid()) ++total;
        }
        result_promise.set_value(total);
    }
};

int main() {
    cout << "Enter the number of rows in the triangle: ";
    int n; cin >> n;

    vector<future<int64_t>> futures;
    vector<thread> threads;
    vector<int> multipliers;
    
    for (int min_col = 0; min_col < n; ++min_col) {
        for (int max_col = 0; max_col < n; ++max_col) {
            if (max_col == min_col) continue;
            for (int low_type = 0; low_type < 2; ++low_type) {
                int low_start_col = low_type == 0 ? 0 : max(min_col - 1, 0);
                int low_end_col = low_type == 0 ? min_col - 2 : min(min_col, n - 2);
                for (int low_col = low_start_col; low_col <= low_end_col; ++low_col) {
                    if (low_type == 0 && low_col == min_col) continue;
                    if (low_type == 0 && low_col == max_col) continue;
                    for (int high_type = 0; high_type < 2; ++high_type) {
                        int high_start_col = high_type == 0 ? 0 : max(max_col - 1, 0);
                        int high_end_col = high_type == 0 ? max_col - 2 : min(max_col, n - 2);
                        for (int high_col = high_start_col; high_col <= high_end_col; ++high_col) {
                            if (high_type == 0 && high_col == min_col) continue;
                            if (high_type == 0 && high_col == max_col) continue;
                            if (high_type == low_type && high_col == low_col) continue;
                            pair<int, int> fixed_positions[] = {
                                pair<int, int>{n - 1, min_col},
                                pair<int, int>{n - 1 - low_type, low_col},
                                pair<int, int>{n - 1, max_col},
                                pair<int, int>{n - 1 - high_type, high_col}
                            };

                            int multiplier = 1;
                            if (low_type == 0) multiplier *= 2;
                            if (high_type == 0) multiplier *= 2;
                            multipliers.push_back(multiplier);
                            
                            promise<int64_t> instance_promise;
                            futures.push_back(instance_promise.get_future());
                            threads.push_back(thread(interlacing_thread(n, fixed_positions), move(instance_promise)));
                        }
                    }
                }
            }
        }
    }

    int64_t num_triangles = 0;
    
    for (int i = 0; i < threads.size(); ++i) {
        num_triangles += multipliers[i] * futures[i].get();
        threads[i].join();
    }

    cout << "Number of triangles: " << num_triangles << endl;
}
