#include <iostream>
#include <utility>
#include <vector>
#include <stack>
#include <unordered_map>
#include <cassert>

typedef uint64_t u64;

using std::vector;
using std::cout;
using std::cin;
using std::string;
using std::stack;
typedef std::unordered_map<u64, u64> hashtable;


struct backtrace {
    int tileSize, cursorRow;
};

class Grid{
public:
    vector<int> rowSizes;
    int cursor = 0;

    Grid(vector<int> rowSizes) : rowSizes(std::move(rowSizes)){ }

    void setCursorAtRow(int pos){
        cursor = pos;
    }

    bool isUnbranchable() const {
        // grid is unbranchable if area to tile is a single row, or if it is an L shape
        return rowSizes[rowSizes.size()-2] <= 1;
    }

    int getMaxTileSize() const {
        return std::min(rowSizes[cursor], (int)rowSizes.size()-cursor);
    }

    bool canRemoveTileOfsize(int n) const {
        if (cursor>0){
            if (rowSizes[cursor-1] > rowSizes[cursor]-n) return false;
        }
        return rowSizes[cursor+n-1] == rowSizes[cursor];
    }

    void removeTileOfSize(int n){
        for (int i = cursor; i < cursor+n; i++){
            rowSizes[i] -= n;
        }
    }

    bool isEmpty() const {
        return rowSizes.empty() || rowSizes.back() == 0;
    }

    void clean(){ // remove empty rows
        int i = 0;
        for (; i < (int)rowSizes.size() && rowSizes[i] == 0; i++);
        rowSizes.erase(rowSizes.begin(), rowSizes.begin() + i);
    }

    u64 hash() const {
        u64 seed = rowSizes.size();
        for(auto x : rowSizes) {
            x = ((x >> 16) ^ x) * 0x45d9f3b;
            x = ((x >> 16) ^ x) * 0x45d9f3b;
            x = (x >> 16) ^ x;
            seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    u64 hashWBt(stack<backtrace> bt){
        u64 seed = rowSizes.size() | bt.size();
        for(auto x : rowSizes) {
            x = ((x >> 16) ^ x) * 0x45d9f3b;
            x = ((x >> 16) ^ x) * 0x45d9f3b;
            x = (x >> 16) ^ x;
            seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        while(!bt.empty()){
            backtrace b = bt.top();
            bt.pop();
            seed ^= b.tileSize + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= b.cursorRow + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    bool tileRemovalIsValid(backtrace bt, int squareSize) const {
        // removal is valid if it does not affect the tile removal in backtrace
        return (squareSize + rowSizes[bt.cursorRow] <= rowSizes[bt.tileSize-1+bt.cursorRow]) &&
               ((cursor>0 && rowSizes[cursor]-squareSize>=rowSizes[cursor-1]) || cursor == 0);
    }
};

hashtable cache, cacheBt;

u64 solve(Grid& grid, stack<backtrace>& backTraceStack);

void readInput(vector<int>& rowSizes){
    int n, m; // n - rows, m - cols
    cin >> n >> m;
    rowSizes.resize(n);
    for (int i = 0; i < n; i++){
        cin >> rowSizes[i];
    }
}

int main(){
    vector<int> rowSizes;
    readInput(rowSizes);

    Grid grid(rowSizes);

    stack<backtrace> backTraceStack;
    if (grid.isEmpty()){
        cout << "0" << std::endl;
    } else {
        u64 result = solve(grid, backTraceStack);
        std::cout << result << std::endl;
    }
    return 0;
}

u64 inline solveAndStore(Grid& grid, stack<backtrace>& backTraceStack){
    u64 result = solve(grid, backTraceStack);
    u64 gridHash = grid.hash();
    if (cache.find(gridHash)==cache.end()){
        cache[gridHash] = result;
    }
    return result;
}

u64 inline solveAndStoreWBt(Grid& grid, stack<backtrace>& backTraceStack){
    u64 result = solve(grid, backTraceStack);
    u64 gridHash = grid.hashWBt(backTraceStack);
    if (cacheBt.find(gridHash)==cacheBt.end()){
        cacheBt[gridHash] = result;
    }
    return result;
}

u64 solve(Grid& grid, stack<backtrace>& backTraceStack) {
    grid.clean();

    if (grid.isEmpty() || grid.isUnbranchable())
        return 1;

    if (backTraceStack.empty()) {
        u64 gridHash = grid.hash();
        if (cache.find(gridHash) != cache.end()){
            return cache[gridHash];
        }
    }else{
        u64 gridHash = grid.hashWBt(backTraceStack);
        if (cacheBt.find(gridHash) != cacheBt.end()){
            return cacheBt[gridHash];
        }
    }

    u64 permutations = 0;
    int maxSquareSize = grid.getMaxTileSize();

    for (int i = 1; i <= maxSquareSize; i++){
        if (!backTraceStack.empty() && !grid.tileRemovalIsValid(backTraceStack.top(), i))
            continue;

        if (!grid.canRemoveTileOfsize(i)){
            Grid newGrid = grid;
            stack<backtrace> newStack;
            if (!backTraceStack.empty()) newStack = stack<backtrace>(backTraceStack);

            newStack.push({i, grid.cursor});
            for (int x = grid.cursor+1; x<(int)grid.rowSizes.size(); x++){
                if (grid.rowSizes[x] > grid.rowSizes[grid.cursor]){
                    newGrid.setCursorAtRow(x);
                    permutations += solveAndStoreWBt(newGrid, newStack);
                    break;
                }
            }
            continue;
        }

        Grid newGrid = grid;
        newGrid.removeTileOfSize(i);

        if (backTraceStack.empty()){
            stack<backtrace> newStack;
            permutations += solveAndStore(newGrid, newStack);
            continue;
        }

        // backtrace isn't empty
        stack<backtrace> newStack = stack<backtrace>(backTraceStack);
        int cursorRow = newGrid.cursor;

        while (!newStack.empty()){
            backtrace ithBacktrace = newStack.top();
            newGrid.setCursorAtRow(ithBacktrace.cursorRow);

            if (newGrid.canRemoveTileOfsize(ithBacktrace.tileSize)){
                newStack.pop();
                newGrid.removeTileOfSize(ithBacktrace.tileSize);
                cursorRow = ithBacktrace.cursorRow;
                if (newStack.empty()){
                    permutations+=solveAndStore(newGrid, newStack);
                }
            } else {
                for (int x = cursorRow; x<(int)newGrid.rowSizes.size(); x++){
                    if (newGrid.rowSizes[x]>newGrid.rowSizes[newGrid.cursor]){
                        newGrid.setCursorAtRow(x);
                        break;
                    }
                }
                permutations+=solveAndStoreWBt(newGrid, newStack);
                break;
            }
        }

    }

    return permutations;
}
