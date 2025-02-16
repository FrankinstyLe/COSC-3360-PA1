#include <iostream>
#include <vector>
#include <pthread.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <map>

using namespace std;

// Structure to hold thread arguments
struct ThreadData {
    int row_index;                   // Which row this thread is processing
    int start_idx;                   // Starting index in dataPos for this row
    int end_idx;                     // Ending index (non-inclusive) in dataPos for this row
    int image_width;                 // Number of columns in the image
    const vector<int> *dataPos;      // Pointer to the vector of column positions (dataPos)
    const map<char, vector<pair<int, int>>> *ranges_map; // Pointer to map storing ranges for each symbol
    string *output_line;             // Pointer to the output line string (to be filled with symbols)
};

// Thread function to decode one row
void *decode_row(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    // Loop over the indices for the row in dataPos
    for (int pos_idx = data->start_idx; pos_idx < data->end_idx; ++pos_idx) {
        int pos = (*data->dataPos)[pos_idx];
        // Only process if pos is within valid column boundaries
        if (pos < 0 || pos >= data->image_width)
            continue;

        // For each symbol, check if pos falls within any of its ranges
        for (const auto &entry : *data->ranges_map) {
            char symbol = entry.first;
            const vector<pair<int, int>> &ranges = entry.second;
            for (const auto &range : ranges) {
                if (pos >= range.first && pos <= range.second) {
                    // Place the symbol in the output line at position pos.
                    (*data->output_line)[pos] = symbol;
                    break;
                }
            }
        }
    }
    pthread_exit(nullptr);
    return nullptr;
}

int main() {
    int image_width, image_length;
    cin >> image_width >> image_length;
    cin.ignore(); // Ignore the remaining newline

    // Validate image dimensions
    if (image_width <= 0 || image_length <= 0) {
        cerr << "Error: Invalid image dimensions." << endl;
        return 1;
    }

    // Read the symbol and range information (e.g., "U 0 10,H 15 25")
    string symbol_line;
    getline(cin, symbol_line);
    map<char, vector<pair<int, int>>> ranges_map;

    // Parse the symbol_line
    stringstream ss(symbol_line);
    string token;
    while (getline(ss, token, ',')) {
        char symbol = token[0];
        string range_part = token.substr(2); // Skip "<symbol> "
        stringstream range_stream(range_part);
        int start, end;
        while (range_stream >> start >> end) {
            if (start < 0 || end < start) {
                cerr << "Error: Invalid symbol range." << endl;
                return 1;
            }
            ranges_map[symbol].emplace_back(start, end);
        }
    }

    // Read headPos array (one integer per row)
    vector<int> headPos(image_length);
    for (int &x : headPos) {
        cin >> x;
    }
    cin.ignore(); // VERY IMPORTANT: Clear the newline after headPos input

    vector<int> dataPos;
    int val;
    while (cin >> val) {
        dataPos.push_back(val);
        if (cin.peek() == '\n') {
            break;
        }
    }
    // Validate headPos: ensure they are non-negative and in non-decreasing order
    for (size_t i = 0; i < headPos.size(); ++i) {
        if (headPos[i] < 0 || (i > 0 && headPos[i] < headPos[i - 1])) {
            cerr << "Error: Invalid headPos values." << endl;
            return 1;
        }
    }

    // Prepare vectors for threads, thread data, and output strings.
    // Reserve capacity to avoid reallocations that could invalidate pointers.
    vector<pthread_t> threads;
    threads.reserve(image_length);
    vector<ThreadData> thread_data;
    thread_data.reserve(image_length);
    vector<string> output(image_length, string(image_width, ' '));

    // Create one thread per row (for all image_length rows)
    for (int i = 0; i < image_length; ++i) {
        // Calculate start and end indices in dataPos for row i.
        int start_idx = headPos[i];
        int end_idx = (i == image_length - 1) ? (int)dataPos.size() : headPos[i + 1];

        // Even if a row has no data (start_idx == end_idx), we still want an output line.
        thread_data.push_back(ThreadData{i, start_idx, end_idx, image_width, &dataPos, &ranges_map, &output[i]});

        pthread_t thread;
        // Create a thread and pass a pointer to the thread_data element.
        if (pthread_create(&thread, nullptr, decode_row, &thread_data.back()) != 0) {
            cerr << "Error: Failed to create thread." << endl;
            return 1;
        }
        threads.push_back(thread);
    }

    // Join all threads that were created
    for (size_t i = 0; i < threads.size(); ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Print the decoded image (each row on a new line)
    for (size_t i = 0; i < output.size(); ++i) {
        cout << output[i] << endl;
    }

    return 0;
}
