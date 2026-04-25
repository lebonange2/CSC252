// =============================================================================
// Program:     Binary File Analyzer
// Description: Creates a binary file of 1000 random integers (0-999), reads
//              them back, then runs four independent analyses:
//                1. StatisticsAnalyzer  - min, max, mean, median, mode
//                2. DuplicatesAnalyzer  - count values that appear more than once
//                3. MissingAnalyzer     - count values in [0,999] absent from data
//                4. SearchAnalyzer      - binary-search 100 random keys; report hits
//
// Students: Matthew Maniscalco, Lebon Harelimana
// 
// =============================================================================

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace std;

const int SIZE = 1000;

// =============================================================================
// writeBinary
// Writes 'length' ints from 'values' to a raw binary file.                               
// File layout: [int count][int v0][int v1]...[int v_{n-1}]
// =============================================================================
void writeBinary(string filename, int* values, int length) {   
    ofstream outFile(filename, ios::binary);
    outFile.write(reinterpret_cast<char*>(&length), sizeof(int));
    for (int i = 0; i < length; i++) {
        outFile.write(reinterpret_cast<char*>(&values[i]), sizeof(int));
    }
    outFile.close();
}

// =============================================================================
// createBinaryFile
// Allocates a heap array, fills it with random values in [0,999],                        
// calls writeBinary, then frees the memory.
// =============================================================================
void createBinaryFile(string filename) {
    int* arr = new int[SIZE];
    for (int i = 0; i < SIZE; i++) {
        arr[i] = rand() % 1000;
    }
    writeBinary(filename, arr, SIZE);
    delete[] arr;
}

// =============================================================================
// selection_sort
// In-place O(n^2) sort. Each pass finds the minimum in the unsorted suffix
// and swaps it into position, growing the sorted prefix by one.
// =============================================================================
void selection_sort(int* values, int size) {
    for (int i = 0; i < size - 1; i++) {
        int minIndex = i;
        for (int j = i + 1; j < size; j++) {
            if (values[j] < values[minIndex]) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            int temp = values[i];
            values[i] = values[minIndex];
            values[minIndex] = temp;
        }
    }
}

// =============================================================================
// binary_search_recursive
// Recursively searches sorted sub-array values[start..end] for 'key'.                    
// Returns true if found, false if the search space is exhausted.
// =============================================================================
bool binary_search_recursive(int* values, int key, int start, int end) {
    if (start > end) {
        return false;    // base case: empty search space
    }

    int mid = start + (end - start) / 2;   // safe midpoint avoids overflow

    if (values[mid] == key) {
        return true;                        // found at midpoint
    }
    else if (values[mid] < key) {
        return binary_search_recursive(values, key, mid + 1, end);   // search right
    }
    else {
        return binary_search_recursive(values, key, start, mid - 1); // search left
    }
}

// =============================================================================
// binary_search
// Public wrapper: hides start/end bookkeeping; array must already be sorted.
// =============================================================================
bool binary_search(int* values, int key, int size) {
    return binary_search_recursive(values, key, 0, size - 1);
}

// =============================================================================
// BinaryReader
// Reads a binary file written by writeBinary() and exposes the data via
// getValues() and getSize().
// =============================================================================
class BinaryReader {
private:
    int* values;
    int  size;

    void readValues(string filename) {
        ifstream inFile(filename, ios::binary);
        inFile.read(reinterpret_cast<char*>(&size), sizeof(int));
        values = new int[size];
        for (int i = 0; i < size; i++) {
            inFile.read(reinterpret_cast<char*>(&values[i]), sizeof(int));
        }
        inFile.close();
    }

public:
    BinaryReader(string filename) {
        values = nullptr;
        size = 0;
        readValues(filename);
    }

    ~BinaryReader() {
        delete[] values;
    }

    int* getValues() { return values; }
    int  getSize() { return size; }
};

// =============================================================================
// Analyzer  (abstract base class)
// Stores a deep copy of the input array so subclasses can freely sort or
// modify it without affecting other analyzers or the caller.
// =============================================================================
class Analyzer {
protected:
    int* values;
    int  size;

    // Allocates a new array and copies all elements from arr
    int* cloneValues(int* arr, int length) {
        int* clone = new int[length];
        for (int i = 0; i < length; i++) {
            clone[i] = arr[i];
        }
        return clone;
    }

public:
    Analyzer(int* arr, int length) {
        size = length;
        values = cloneValues(arr, length);
    }

    ~Analyzer() {
        delete[] values;
    }

    virtual string analyze() = 0;
};

// =============================================================================
// StatisticsAnalyzer
// Sorts the private array, then computes min, max, mean, median, and mode.
// Because the data is sorted: min = values[0], max = values[size-1],
// median is the centre element(s), and mode is found with one linear scan.
// =============================================================================
class StatisticsAnalyzer : public Analyzer {
public:
    StatisticsAnalyzer(int* arr, int length) : Analyzer(arr, length) {}

    string analyze() override {
        // Sort our private copy so all statistics can be derived efficiently
        selection_sort(values, size);

        // Min and Max are now at the sorted endpoints
        int min = values[0];
        int max = values[size - 1];

        // Mean: sum all values and divide by count
        double sum = 0.0;
        for (int i = 0; i < size; i++) {
            sum += values[i];
        }
        double mean = sum / size;

        // Median: exact centre (odd n) or average of two centre values (even n)
        double median;
        if (size % 2 != 0) {
            median = values[size / 2];
        }
        else {
            median = (values[size / 2 - 1] + values[size / 2]) / 2.0;
        }

        // Mode: most frequent value; first occurrence wins ties.
        // The sorted array groups equal values together, making run detection O(n).
        int modeValue = values[0];
        int modeCount = 1;
        int currentCount = 1;

        for (int i = 1; i < size; i++) {
            if (values[i] == values[i - 1]) {
                currentCount++;
                if (currentCount > modeCount) {
                    modeCount = currentCount;
                    modeValue = values[i];
                }
            }
            else {
                currentCount = 1;   // new value: reset run counter
            }
        }

        string result;
        result += "The minimum value is " + to_string(min) + "\n";
        result += "The maximum value is " + to_string(max) + "\n";
        result += "The mean value is " + to_string(mean) + "\n";
        result += "The median value is " + to_string((int)median) + "\n";
        result += "The mode value is " + to_string(modeValue) +
            " which occurred " + to_string(modeCount) + " times";
        return result;
    }
};

// =============================================================================
// DuplicatesAnalyzer
// Counts how many distinct values in the dataset appear more than once.
// Uses a nested O(n^2) scan: for each element, look ahead for a repeat.
// =============================================================================
class DuplicatesAnalyzer : public Analyzer {
public:
    DuplicatesAnalyzer(int* arr, int length) : Analyzer(arr, length) {}

    string analyze() override {
        int count = 0;
        for (int i = 0; i < size; i++) {
            for (int j = i + 1; j < size; j++) {
                if (values[i] == values[j]) {
                    count++;   // values[i] has at least one duplicate
                    break;     // stop scanning; we only count each value once
                }
            }
        }
        return "There were " + to_string(count) + " duplicated values";
    }
};

// =============================================================================
// MissingAnalyzer
// Counts integers in [0, 999] that do not appear anywhere in the dataset.
// =============================================================================
class MissingAnalyzer : public Analyzer {
public:
    MissingAnalyzer(int* arr, int length) : Analyzer(arr, length) {}

    string analyze() override {
        int count = 0;
        for (int i = 0; i < 1000; i++) {
            bool found = false;
            for (int j = 0; j < size; j++) {
                if (values[j] == i) {
                    found = true;
                    break;   // short-circuit once the value is located
                }
            }
            if (!found) {
                count++;
            }
        }
        return "There were " + to_string(count) + " missing values";
    }
};

// =============================================================================
// SearchAnalyzer
// Sorts the private array in the constructor (prerequisite for binary search),
// then in analyze() tests 100 random keys and reports the hit count.
// =============================================================================
class SearchAnalyzer : public Analyzer {
public:
    // Sort immediately so binary_search is valid for any subsequent call
    SearchAnalyzer(int* arr, int length) : Analyzer(arr, length) {
        selection_sort(values, size);
    }

    string analyze() override {
        int found = 0;
        for (int i = 0; i < 100; i++) {
            int key = rand() % 1000;
            if (binary_search(values, key, size)) {
                found++;
            }
        }
        return "There were " + to_string(found) + " out of 100 random values found";
    }
};

// =============================================================================
// main
// =============================================================================
int main() {
    srand(static_cast<unsigned>(time(0)));

    // Create the binary file with 1000 random integers
    createBinaryFile("binary.dat");

    // Read the file back
    BinaryReader reader("binary.dat");

    // Each analyzer receives its own independent deep copy of the data
    StatisticsAnalyzer stats(reader.getValues(), reader.getSize());
    DuplicatesAnalyzer dups(reader.getValues(), reader.getSize());
    MissingAnalyzer    missing(reader.getValues(), reader.getSize());
    SearchAnalyzer     search(reader.getValues(), reader.getSize());

    // Run and display all four analyses
    cout << stats.analyze() << endl;
    cout << dups.analyze() << endl;
    cout << missing.analyze() << endl;
    cout << search.analyze() << endl;

    return 0;
}