#include <algorithm>
#include <future>
#include <iostream>
#include <mutex>
#include <vector>

using namespace std;

constexpr int factorials[11] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800};

mutex output_mutex;

void print_triangle(int64_t num_valid, int num_rows, int values[10][10]) {
   output_mutex.lock();
   cout << num_valid << endl;
   for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col <= row; ++col) {
            cout << " " << values[row][col];
        }
        cout << endl;
   } 
   cout << endl;
   output_mutex.unlock();
}

class interlacing_thread {
private:
    int values[10][10];
    pair<int, int> positions[100];
    int num_rows;
    int max_num;
    int max_movable;
    bool all_smaller_in_last_row[10];

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

        if (to_move < this->num_rows && this->all_smaller_in_last_row[to_move]) {
            return max_pos;
        }

        pair<int, int> current_pos = this->positions[to_move];
        pair<int, int> neighbour_pos = this->positions[to_move + 1];

        for (int replaces = 0; replaces < to_move; ++replaces) {
            pair<int, int> new_pos = this->positions[replaces];
            if (new_pos > current_pos) continue;
            if (new_pos.first == this->num_rows - 1) {
                if (neighbour_pos.first == this->num_rows - 1) {
                    if (new_pos.second > neighbour_pos.second - 2) continue;
                }
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
        for (int candidate = 1; candidate <= this->max_movable; ++candidate) {
            new_pos = this->find_move(candidate);
            if (new_pos.first != -1) {
                to_move = candidate;
                break;
            }
        }
        if (to_move == -1) return false;
        
        pair<int, int> positions[100];
        int num_positions = 0;

        for (int i = 0; i <= to_move; ++i) {
            if (this->positions[i] == new_pos) continue;
            positions[num_positions++] = this->positions[i];
        }
        sort(positions, positions + num_positions);

        this->set_value(new_pos, to_move);
        for (int num = 0; num < to_move; ++num) {
            this->set_value(positions[num], num);
        }

        if (new_pos.first == this->num_rows - 1) {
            if (this->positions[to_move - 1].first == this->num_rows - 1) {
                if (this->positions[to_move - 1].second > new_pos.second) {
                    for (int num = to_move - 1; num >= 0; --num) {
                        if (this->positions[num].first < new_pos.first) {
                            this->set_value(positions[num], to_move - 1);
                            for (int i = num + 1; i < to_move; ++i) {
                                this->set_value(positions[i], i - 1);
                            }
                            break;
                        }
                    }
                }
            }
        }

        this->update_last_row();

        return true;
    }

    int64_t num_valid() {
        for (int row = 0; row < this->num_rows - 1; ++row) {
            for (int col = 0; col <= row; ++col) {
                if ((this->values[row][col] > this->values[row + 1][col])
                 == (this->values[row][col] > this->values[row + 1][col + 1])) {
                    return 0;
                }
            }
        }

        int last_row[10];
        for (int i = 0; i < this->num_rows; ++i) {
            last_row[i] = this->values[this->num_rows - 1][i];
        }
        sort(last_row, last_row + this->num_rows);

        int64_t result = 1;
        int consecutive = 1;
        int value = last_row[0];

        for (int i = 1; i < this->num_rows; ++i) {
            if (last_row[i] == value + 1) ++consecutive;
            else {
                result = result * factorials[consecutive];
                consecutive = 1;
            }
            value = last_row[i];
        }

        return result * factorials[consecutive];
    }

    void update_last_row() {
        all_smaller_in_last_row[0] = true;
        for (int i = 1; i < this->num_rows; ++i) {
            this->all_smaller_in_last_row[i] = this->all_smaller_in_last_row[i - 1]
                && (this->positions[i - 1].first == this->num_rows - 1);
        }
    }

public:
    interlacing_thread(int num_rows, vector<pair<int, int>> fixed_positions) {
        this->num_rows = num_rows;
        const int max_num = (num_rows * (num_rows + 1)) / 2 - 1;
        this->max_num = max_num;
        this->max_movable = max_num - fixed_positions.size();

        int num = max_num;

        for (pair<int, int> position : fixed_positions) {
            this->set_value(position, num--);
        }

        sort(fixed_positions.begin(), fixed_positions.end());

        int fixed_index = fixed_positions.size() - 1;
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

        this->update_last_row();
    }

    void operator()(promise<int64_t> result_promise) {
        int64_t total = this->num_valid();
        print_triangle(this->num_valid(), this->num_rows, this->values);
        while (this->next_candidate()) {
            print_triangle(this->num_valid(), this->num_rows, this->values);
            total += this->num_valid();
        }
        result_promise.set_value(total);
    }
};

int main() {
    cout << "Enter the number of rows in the triangle: ";
    int n; cin >> n;

    vector<future<int64_t>> futures;
    vector<thread> threads;
    
    for (int max_pos = 0; max_pos < n; ++max_pos) {
        for (int second_col = 0; second_col < max_pos - 1; ++second_col) {
            promise<int64_t> instance_promise;
            futures.push_back(instance_promise.get_future());
            threads.push_back(thread(interlacing_thread(n, vector<pair<int, int>>{pair<int, int>{n - 1, max_pos}, pair<int, int>{n - 1, second_col}}), move(instance_promise)));
        }
        
        if (max_pos < n - 1) {
            promise<int64_t> left_promise;
            futures.push_back(left_promise.get_future());
            threads.push_back(thread(interlacing_thread(n, vector<pair<int, int>>{pair<int, int>{n - 1, max_pos}, pair<int, int>{n - 2, max_pos}}), move(left_promise)));
        }

        if (max_pos > 0) {
            promise<int64_t> right_promise;
            futures.push_back(right_promise.get_future());
            threads.push_back(thread(interlacing_thread(n, vector<pair<int, int>>{pair<int, int>{n - 1, max_pos}, pair<int, int>{n - 2, max_pos - 1}}), move(right_promise)));
        }
    }

    int64_t num_triangles = 0;
    
    for (int i = 0; i < threads.size(); ++i) {
        num_triangles += futures[i].get();
        threads[i].join();
    }

    cout << "Number of triangles: " << num_triangles << endl;
}
