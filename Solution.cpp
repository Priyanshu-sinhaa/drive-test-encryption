#include <bits/stdc++.h>
using namespace std;
#include <vector>
#include <cmath> // for pow()
#include <gmpxx.h>
using BigFloat = mpf_class;  // Arbitrary-precision floating point
using BigInt = mpz_class;    // Arbitrary-precision integer

struct Entry {
    string base;
    string value;
};



// defining a structure to parse the json.
struct ParsedData {
    int n, k;  // Assuming n and k are not extremely large
    map<int, Entry> entries;
};

string extractStringValue(const string& json, const string& key) {
    size_t key_pos = json.find("\"" + key + "\":");
    if (key_pos == string::npos) return "";

    // Handle both quoted and unquoted values
    size_t value_start = json.find_first_of("\":", key_pos + key.length() + 2) + 1;
    value_start = json.find_first_not_of(" \t\n\r", value_start);
    if (value_start == string::npos) return "";

    if (json[value_start] == '"') {
        // Quoted string value
        size_t start = value_start + 1;
        size_t end = json.find("\"", start);
        if (end == string::npos) return "";
        return json.substr(start, end - start);
    } else {
        // Unquoted numeric value
        size_t end = json.find_first_of(", \t\n\r}", value_start);
        if (end == string::npos) end = json.length();
        return json.substr(value_start, end - value_start);
    }
}

bool parseInput(const string& filename, ParsedData& data) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error: Could not open file '" << filename << "'" << endl;
        return false;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string json = buffer.str();

    // Extract keys block
    size_t keys_start = json.find("\"keys\":");
    if (keys_start == string::npos) {
        cerr << "Error: 'keys' block not found!" << endl;
        return false;
    }

    // Extract n and k
    string keys_block = json.substr(keys_start, json.find("}", keys_start) - keys_start + 1);
    string n_str = extractStringValue(keys_block, "n");
    string k_str = extractStringValue(keys_block, "k");

    if (n_str.empty() || k_str.empty()) {
        cerr << "Error: Missing 'n' or 'k' values in keys block!" << endl;
        return false;
    }

    try {
        data.n = stoi(n_str);
        data.k = stoi(k_str);
    } catch (...) {
        cerr << "Error: Invalid numeric format for 'n' or 'k'!" << endl;
        return false;
    }

    // Parse entries
    size_t pos = 0;
    while ((pos = json.find("\"", pos + 1)) != string::npos) {
        if (!isdigit(json[pos + 1])) continue;

        size_t key_end = json.find("\"", pos + 1);
        string key_str = json.substr(pos + 1, key_end - pos - 1);
        
        // Find the entry object
        size_t entry_start = json.find("{", key_end);
        if (entry_start == string::npos) continue;
        
        size_t entry_end = json.find("}", entry_start);
        if (entry_end == string::npos) continue;
        
        string entry_str = json.substr(entry_start, entry_end - entry_start + 1);

        Entry entry;
        entry.base = extractStringValue(entry_str, "base");
        entry.value = extractStringValue(entry_str, "value");

        if (!entry.base.empty() && !entry.value.empty()) {
            try {
                int key = stoi(key_str);
                data.entries[key] = entry;
            } catch (...) {
                cerr << "Warning: Invalid entry key '" << key_str << "'" << endl;
            }
        }

        pos = entry_end;
    }

    return true;
}


// Function to convert value from given base to decimal
int convertToDecimal(const string& valueStr, const string& baseStr) {
    try {
        int base = stoi(baseStr);
        int decimal = 0;
        int power = 1; // base^0
        
        // Process digits from right to left
        for (int i = valueStr.length() - 1; i >= 0; --i) {
            char c = valueStr[i];
            int digit = c - '0';
            
            if (digit >= base) {
                throw invalid_argument("Digit exceeds base");
            }
            
            decimal += digit * power;
            power *= base;
        }
        return decimal;
    } catch (...) {
        throw invalid_argument("Invalid base or value format");
    }
}



// Creates Vandermonde matrix for polynomial fitting
vector<vector<BigFloat>> createVandermonde(const vector<BigInt>& x, int degree) {
    int n = x.size();
    vector<vector<BigFloat>> matrix(n, vector<BigFloat>(degree + 1));
    
    for (int i = 0; i < n; ++i) {
        BigFloat x_pow(1);  // x^0 = 1
        for (int j = 0; j <= degree; ++j) {
            matrix[i][j] = x_pow;
            x_pow *= x[i];
        }
    }
    return matrix;
}

vector<BigFloat> solveSystem(vector<vector<BigFloat>>& A, const vector<BigInt>& b) {
    int n = A.size();
    
    // Augment the matrix
    for (int i = 0; i < n; ++i) {
        A[i].push_back(b[i]);
    }

    // Gaussian elimination with partial pivoting
    for (int col = 0; col < n; ++col) {
        // Partial pivoting
        int max_row = col;
        for (int row = col + 1; row < n; ++row) {
            if (abs(A[row][col]) > abs(A[max_row][col])) {
                max_row = row;
            }
        }
        swap(A[col], A[max_row]);

        // Elimination
        for (int row = col + 1; row < n; ++row) {
            BigFloat factor = A[row][col] / A[col][col];
            for (int c = col; c <= n; ++c) {
                A[row][c] -= factor * A[col][c];
            }
        }
    }

    // Back substitution
    vector<BigFloat> solution(n);
    for (int row = n - 1; row >= 0; --row) {
        solution[row] = A[row][n];
        for (int col = row + 1; col < n; ++col) {
            solution[row] -= A[row][col] * solution[col];
        }
        solution[row] /= A[row][row];
    }

    return solution;
}

vector<double> findPolynomialCoefficients(const map<int, int>& xyMap, int degree) {
    // Extract x and y values
    vector<double> x, y;
    for (const auto& [xi, yi] : xyMap) {
        x.push_back(xi);
        y.push_back(yi);
    }

    // Create Vandermonde matrix
    auto V = createVandermonde(x, degree);
    
    // Solve the system V*a = y
    return solveSystem(V, y);
}

int main() {

    // Checkpoint 1: Parsing the json 

    cout<<"Enter the name of your json \n";
    string input;
    cin>>input;
    ParsedData data;
    if (!parseInput(input, data)) {
        cerr << "Failed to parse input!" << endl;
        return 1;
    }

    cout << "Parsed successfully:\n";
    cout << "n: " << data.n << ", k: " << data.k << "\n";
    cout << "Entries:\n";
    for (const auto& [key, entry] : data.entries) {
        cout << "  " << key << ": {base: " << entry.base 
             << ", value: " << entry.value << "}\n";
    }


    // Checkpoint2: Decoding y values Creating map of (x, y) pairs
    map<int, int> xyMap;
    
    for (const auto& [x, entry] : data.entries) {
        try {
            int y = convertToDecimal(entry.value, entry.base);
            xyMap[x] = y;
        } catch (const invalid_argument& e) {
            cerr << "Error converting entry " << x << ": " << e.what() << endl;
        }
    }
    
    // Print the (x, y) map
    cout << "\n(x, y) pairs:\n";
    for (const auto& [x, y] : xyMap) {
        cout << "  " << x << ": " << y << "\n";
    }



        // Checkpoint 3: Find polynomial coefficients
    try {
        int polynomial_degree = data.k - 1;
        if (xyMap.size() <= polynomial_degree) {
            throw runtime_error("Not enough points for polynomial fitting");
        }

        auto coefficients = findPolynomialCoefficients(xyMap, polynomial_degree);
        
        cout << "\nPolynomial coefficients (highest degree first):\n";
        for (size_t i = 0; i < coefficients.size(); ++i) {
            cout << "  x^" << coefficients.size()-1-i << ": " << coefficients[i] << "\n";
        }
        
        // The constant term is the last coefficient
        cout << "\nConstant term: " << coefficients.back() << endl;
    } catch (const exception& e) {
        cerr << "Error in polynomial fitting: " << e.what() << endl;
    }
    return 0;
}
