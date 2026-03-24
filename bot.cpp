#pragma GCC optimize("Ofast")

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <iomanip>
#include <ranges>
#include <optional>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <bitset>

using namespace std;

// ==========================================if
// 1. STAŁE, NARZĘDZIA I STRUKTURY BAZOWE
// ==========================================

const int DX[] = {0, 0, -1, 1}; // UP, DOWN, LEFT, RIGHT
const int DY[] = {-1, 1, 0, 0};
const string DIR_NAMES[] = {"UP", "DOWN", "LEFT", "RIGHT"};
const int WALL_VAL = -1;
const int UNVISITED = -2;
const int INF = numeric_limits<int>::max();

struct Point
{
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

bool isOpposite(int d1, int d2)
{
    if (d1 == -1)
        return false;
    return (d1 == 0 && d2 == 1) || (d1 == 1 && d2 == 0) ||
        (d1 == 2 && d2 == 3) || (d1 == 3 && d2 == 2);
}

int manhattanDist(const Point& a, const Point& b)
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

// ==========================================
// 2. MODELE DANYCH (WĘZŁY, CELE, WĄŻ)
// ==========================================

struct Node
{
    Point p;
    int lastDir;
    int g, f;
    bool operator>(const Node& o) const { return f > o.f; }
};

class Target
{
public:
    int id, x, y;
    bool exists = true;
    bool available = true;

    Target(int id, const int x, const int y) : id(id), x(x), y(y)
    {
    }
};

class GraphNode
{
public:
    int id;
    char type = '?';
    bool isActive = true;
    bool isAvailable = true;
    Point pos{};
    vector<pair<int, int>> neighbours;

    GraphNode(int id, Point p, char t) : id(id), type(t), pos(p)
    {
    }
};

// ==========================================
// GRAPH BEAM SEARCH
// ==========================================

class Map
{
public:
    string grid;
    vector<int> graphGrid;
    int width, height;
    int extWidth, extHeight;

    Map(int height, int width)
        : width(width), height(height), extWidth(width + 2), extHeight(height + 2),
          grid((width + 2) * (height + 2), '.'), graphGrid((width + 2) * (height + 2), -1)
    {
    }

    inline int getIndex(int x, int y) const
    {
        return (y + 1) * extWidth + (x + 1);
    }

    bool isInside(int x, int y) const
    {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    void setCell(int x, int y, char val)
    {
        if (x >= -1 && x <= width && y >= -1 && y <= height)
        {
            grid[getIndex(x, y)] = val;
        }
    }

    char getCell(int x, int y) const
    {
        return grid[getIndex(x, y)];
    }

    void setGraphCell(int x, int y, int val)
    {
        if (x >= -1 && x <= width && y >= -1 && y <= height)
        {
            graphGrid[getIndex(x, y)] = val;
        }
    }

    int getGraphCell(int x, int y) const
    {
        if (x >= -1 && x <= width && y >= -1 && y <= height)
        {
            return graphGrid[getIndex(x, y)];
        }
        return -1;
    }

    void printMap() const
    {
        for (int y = -1; y <= height; y++)
        {
            //clog << grid.substr(getIndex(-1, y), extWidth) << endl;
        }
        // for (int y = -1; y <= height; y++) {
        // for (int x = -1; x <= width; x++) {
        // //clog<< graphGrid[getIndex(x, y)] << "\t";
        // }
        // //clog<< endl;
        //  }
    }
};

class Snake
{
public:
    int id, length = 3;
    Point headPos{0, 0}, prevHeadPos{-1, -1};
    vector<Point> bodyPos;

    Point targetPoint;
    vector<Point> path;
    int targetID = -1;
    vector<int> targets;

    string last_move = "UP";
    vector<string> move_horizont;
    bool exists = true;

    bool fighter = false;
    int helper = -1;
    bool helping = false;

    Snake(int id) : id(id)
    {
    }

    void updateBody(const string& body)
    {
        bodyPos.clear();
        int a = 0, b = 0;
        bool parsingX = true;

        for (char c : body)
        {
            if (c == ',')
            {
                parsingX = false;
            }
            else if (c == ':')
            {
                if (a >= -1 && b >= -1)
                    bodyPos.push_back({a, b});
                a = 0;
                b = 0;
                parsingX = true;
            }
            else
            {
                if (parsingX)
                {
                    a = a * 10 + (c - '0');
                }
                else
                {
                    b = b * 10 + (c - '0');
                }
            }
        }
        if (a >= -1 && b >= -1)
            bodyPos.push_back({a, b});

        length = bodyPos.size();
        if (!bodyPos.empty())
            headPos = bodyPos[0];
        exists = true;
        if (length > 1)
        {
            int dx = bodyPos[0].x - bodyPos[1].x;
            int dy = bodyPos[0].y - bodyPos[1].y;

            if (dx > 0) last_move = "RIGHT";
            else if (dx < 0) last_move = "LEFT";
            else if (dy > 0) last_move = "DOWN";
            else if (dy < 0) last_move = "UP";
        }
    }
};

// ==========================================
// 5. TUNELEEE
// ==========================================

struct Tunnel
{
    int id;
    int length;
    bool isDeadEnd;
    vector<Point> entrances;
    vector<Point> cells;
};

class TunnelAnalyzer
{
public:
    vector<Tunnel> tunnels;
    vector<int> cellToTunnelId;

    void analyzeMap(const Map& map)
    {
        tunnels.clear();
        int n = map.extWidth * map.extHeight;
        cellToTunnelId.assign(n, -1);

        vector<int> degree(n, 0);
        vector<bool> isWalkable(n, false);

        for (int y = 0; y < map.height; ++y)
        {
            for (int x = 0; x < map.width; ++x)
            {
                if (map.getCell(x, y) != '#')
                {
                    isWalkable[map.getIndex(x, y)] = true;
                }
            }
        }

        for (int y = 0; y < map.height; ++y)
        {
            for (int x = 0; x < map.width; ++x)
            {
                int idx = map.getIndex(x, y);
                if (!isWalkable[idx]) continue;

                int deg = 0;
                for (int i = 0; i < 4; i++)
                {
                    int nx = x + DX[i];
                    int ny = y + DY[i];
                    if (map.isInside(nx, ny) && isWalkable[map.getIndex(nx, ny)])
                    {
                        deg++;
                    }
                }
                degree[idx] = deg;
            }
        }

        vector<bool> visited(n, false);
        int currentTunnelId = 0;

        for (int y = 0; y < map.height; ++y)
        {
            for (int x = 0; x < map.width; ++x)
            {
                int startIdx = map.getIndex(x, y);

                if (isWalkable[startIdx] && !visited[startIdx] && degree[startIdx] <= 2)
                {
                    Tunnel t;
                    t.id = currentTunnelId;
                    t.isDeadEnd = false;

                    queue<Point> q;
                    q.push({x, y});
                    visited[startIdx] = true;

                    int deadEndCount = 0;

                    // BFS
                    while (!q.empty())
                    {
                        Point curr = q.front();
                        q.pop();

                        t.cells.push_back(curr);
                        int currIdx = map.getIndex(curr.x, curr.y);
                        cellToTunnelId[currIdx] = t.id;

                        if (degree[currIdx] == 1)
                        {
                            deadEndCount++;
                        }
                        for (int i = 0; i < 4; i++)
                        {
                            int nx = curr.x + DX[i];
                            int ny = curr.y + DY[i];

                            if (map.isInside(nx, ny) && isWalkable[map.getIndex(nx, ny)])
                            {
                                int nIdx = map.getIndex(nx, ny);

                                if (degree[nIdx] >= 3)
                                {
                                    Point entrance = {nx, ny};
                                    if (ranges::find(t.entrances, entrance) == t.entrances.end())
                                    {
                                        t.entrances.push_back(entrance);
                                    }
                                }
                                else if (!visited[nIdx])
                                {
                                    // Sąsiad to kontynuacja tunelu
                                    visited[nIdx] = true;
                                    q.push({nx, ny});
                                }
                            }
                        }
                    }

                    t.length = t.cells.size();
                    if (deadEndCount > 0 || t.entrances.size() <= 1)
                    {
                        t.isDeadEnd = true;
                    }

                    tunnels.push_back(t);
                    currentTunnelId++;
                }
            }
        }
    }
};


struct PathResult
{
    vector<int> path;
    int totalLength;
};

// ==========================================
// SYMULACJA BEAM SEARCH
// ==========================================

struct LightState
{
    int grid[1600];
    std::bitset<1600> hasTarget;
    int width, height;
    int turn;

    int headX, headY;
    int length;
    bool alive;

    int firstMove;
    double score;

    int bodyX[70];
    int bodyY[70];
    int headIdx;
    int tailIdx;
    int enemyHeadX[10];
    int enemyHeadY[10];
    int enemyLength[10];
    std::bitset<1600> isStable;
    int numEnemies;
    int lastDir;

    bool hunterMode;

    const TunnelAnalyzer* tunnelAnalyzer = nullptr;

    void init(const Map& map, const Snake& mySnake, const vector<Snake>& enemies, const vector<GraphNode>& nodes,
              const TunnelAnalyzer* ta, bool isHunter = true)
    {
        hunterMode = isHunter;
        tunnelAnalyzer = ta;

        width = map.width;
        height = map.height;
        turn = 0;
        alive = true;
        score = 0;
        firstMove = -1;
        if (mySnake.last_move == "UP")
            lastDir = 0;
        else if (mySnake.last_move == "DOWN")
            lastDir = 1;
        else if (mySnake.last_move == "LEFT")
            lastDir = 2;
        else if (mySnake.last_move == "RIGHT")
            lastDir = 3;
        else
            lastDir = -1;
        fill(begin(grid), end(grid), 0);
        hasTarget.reset();
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (map.getCell(x, y) == '#')
                    grid[y * width + x] = 65000;
                if (map.getCell(x, y) == 'P')
                    grid[y * width + x] = 60000;
            }
        }

        for (const auto& t : nodes)
        {
            if (t.isActive && t.pos.x >= 0 && t.pos.x < width && t.pos.y >= 0 && t.pos.y < height)
            {
                if (t.type == 'S')
                {
                    hasTarget[t.pos.y * width + t.pos.x] = true;
                }
                else
                {
                    isStable[t.pos.y * width + t.pos.x] = true;
                }
            }
        }

        headX = mySnake.headPos.x;
        headY = mySnake.headPos.y;
        length = mySnake.bodyPos.size();

        tailIdx = 0;
        headIdx = length - 1;
        for (int i = 0; i < length; ++i)
        {
            bodyX[length - 1 - i] = mySnake.bodyPos[i].x;
            bodyY[length - 1 - i] = mySnake.bodyPos[i].y;
            // //clog << mySnake.id << " body body " << mySnake.bodyPos[i].x << " " << mySnake.bodyPos[i].y << endl;
        }

        int offset = length;
        for (const auto& p : mySnake.bodyPos)
        {
            if (p.x >= 0 && p.x < width && p.y >= 0 && p.y < height)
            {
                grid[p.y * width + p.x] = offset--;
            }
        }
        numEnemies = 0;
        for (const auto& e : enemies)
        {
            if (!e.exists || e.bodyPos.empty())
                continue;

            enemyHeadX[numEnemies] = e.headPos.x;
            enemyHeadY[numEnemies] = e.headPos.y;
            enemyLength[numEnemies] = e.length; // <--- NOWE
            numEnemies++;

            offset = e.length;
            for (const auto& p : e.bodyPos)
            {
                if (p.x >= 0 && p.x < width && p.y >= 0 && p.y < height)
                {
                    grid[p.y * width + p.x] = offset--;
                }
            }
        }
    }

    void simulate(int moveDir)
    {
        if (!alive)
            return;
        if (isOpposite(lastDir, moveDir))
        {
            alive = false;
            return;
        }
        lastDir = moveDir;

        turn++;

        int nextHeadX = headX + DX[moveDir];
        int nextHeadY = headY + DY[moveDir];

        if (nextHeadX < 0 || nextHeadX >= width || nextHeadY < 0 || nextHeadY >= height)
        {
            alive = false;

            return;
        }

        int idx = nextHeadY * width + nextHeadX;

        bool isSmallerEnemyHead = false;
        if (hunterMode)
        {
            for (int i = 0; i < numEnemies; ++i)
            {
                if (((nextHeadX == (enemyHeadX[i] + 1) && nextHeadY == enemyHeadY[i] && grid[idx + 1] < turn) ||
                        (nextHeadX == (enemyHeadX[i] + 1) - 1 && nextHeadY == enemyHeadY[i] && grid[idx - 1] < turn) ||
                        (nextHeadX == (enemyHeadX[i]) && nextHeadY == enemyHeadY[i] + 1 && grid[idx + width] < turn) ||
                        (nextHeadX == (enemyHeadX[i]) && nextHeadY == enemyHeadY[i] - 1 && grid[idx - width] < turn))
                    && length > enemyLength[i])
                {
                    isSmallerEnemyHead = true;
                    //clog << "YOLLOOOO " << enemyHeadX[i] << " " << enemyHeadY[i] << endl;
                    break;
                }
            }
        }
        ////clog << "sim1 " << endl;
        if (grid[idx] > turn && !isSmallerEnemyHead)
        {
            alive = false;
            // //clog << "SECOND " << endl;
            return;
        }
        ////clog << "sim2 " << tailIdx << " " << headIdx <<  endl;

        // --- 4. RUCH JEST LEGALNY - ZDEJMUJEMY WĘŻA Z PLANSZY ---
        // Teraz możemy bezpiecznie go usunąć, żeby przygotować się na ew. grawitację
        for (int i = tailIdx; i <= headIdx; ++i)
        {
            ////clog << "sim21 " << i << " " << bodyY[i] << " " << bodyX[i] <<  endl;
            int bodyIdx = bodyY[i] * width + bodyX[i];
            ////clog << "sim22 " << bodyIdx <<  endl;
            if (grid[bodyIdx] < 60000)
            {
                grid[bodyIdx] = 0;
            }
        }
        ////clog << "sim3 " << endl;

        // Aktualizuja pozycji
        headX = nextHeadX;
        headY = nextHeadY;

        if (hasTarget[idx])
        {
            length++;
            score += 1000;
            hasTarget[idx] = false;
        }
        else if (isStable[idx])
        {
            score += 500;
            // hasTarget[idx] = false;
        }
        else
        {
            tailIdx++;
        }
        ////clog << "sim4 " << endl;

        headIdx++;
        bodyX[headIdx] = headX;
        bodyY[headIdx] = headY;

        bool isSupported = false;
        while (alive)
        {
            isSupported = false;

            for (int i = tailIdx; i <= headIdx; ++i)
            {
                int bx = bodyX[i];
                int by = bodyY[i];

                if (by + 1 >= height)
                {
                    alive = false;

                    break;
                }

                int belowIdx = (by + 1) * width + bx;
                // Skoro zdjęliśmy węża z gridu (krok 4), nie zatrzyma się sam na sobie w powietrzu!
                if (grid[belowIdx] >= 65000 || grid[belowIdx] > turn)
                {
                    isSupported = true;
                    // score += 1;

                    break;
                }
            }

            if (!alive || isSupported)
            {
                break;
            }

            headY++;
            for (int i = tailIdx; i <= headIdx; ++i)
            {
                bodyY[i]++;
                if (i == headIdx)
                {
                    int fallingHeadIdx = bodyY[i] * width + bodyX[i];
                    if (hasTarget[fallingHeadIdx])
                    {
                        length++;
                        score += 1000;
                        hasTarget[fallingHeadIdx] = false;
                        if (tailIdx > 0)
                            tailIdx--;
                    }
                    else if (isStable[fallingHeadIdx])
                    {
                        score += 100;
                    }
                }
            }
            // score -= 5;
        }
        if (alive)
        {
            int currentLifespan = turn + length;
            for (int i = headIdx; i >= tailIdx; --i)
            {
                int bodyIdx = bodyY[i] * width + bodyX[i];
                grid[bodyIdx] = currentLifespan--;
            }
        }
    }

    void evaluate(Point targetPoint)
    {
        bool huntedSmaller = false;-
        if (hunterMode)
        {
            for (int i = 0; i < numEnemies; ++i)
            {
                if (headX == enemyHeadX[i] && headY == enemyHeadY[i] && length > enemyLength[i])
                {
                    huntedSmaller = true;
                    score += 20000;
                    break;
                }
            }
        }
        if (!alive)
        {
            if (!huntedSmaller)
            {
                int safeTurn = (turn == 0) ? 1 : turn;
                score -= int(50000 / safeTurn);
            }
            return;
        }
        int dist = abs(headX - targetPoint.x) + abs(headY - targetPoint.y);
        score -= dist;
        for (int i = 0; i < numEnemies; ++i)
        {
            int distToEnemy = abs(headX - enemyHeadX[i]) + abs(headY - enemyHeadY[i]);
            int safeDist = (distToEnemy == 0) ? 1 : distToEnemy;

            if (hunterMode && length > enemyLength[i])
            {
                if (distToEnemy <= 3)
                {
                    score += 1500 / safeDist;
                }
            }
            else
            {
                score -= 500 / safeDist;
            }
        }
        int space = 0;
        int q[1600];
        int bfsTurn[1600];

        bool vis[1600] = {false};

        int qHead = 0, qTail = 0;

        int startIdx = headY * width + headX;
        q[qTail] = startIdx;
        bfsTurn[qTail] = turn;
        qTail++;
        vis[startIdx] = true;

        while (qHead < qTail && space < length + 10)
        {
            int currIdx = q[qHead];
            int currentBfsTurn = bfsTurn[qHead];
            qHead++;

            int cx = currIdx % width;
            int cy = currIdx / width;
            space++;

            for (int i = 0; i < 4; i++)
            {
                int nx = cx + DX[i];
                int ny = cy + DY[i];

                if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                {
                    int nIdx = ny * width + nx;

                    if (!vis[nIdx])
                    {
                        if (currentBfsTurn + 1 >= grid[nIdx])
                        {
                            vis[nIdx] = true;
                            q[qTail] = nIdx;
                            bfsTurn[qTail] = currentBfsTurn + 1;
                            qTail++;
                        }
                    }
                }
            }
        }

        if (alive && tunnelAnalyzer != nullptr)
        {
            // UWAGA: LightState używa indeksów 0..width-1, a TunnelAnalyzer używa extWidth (z ramką).
            // Musimy przeliczyć współrzędne headX, headY na indeks zgodny z map.getIndex()
            int extWidth = width + 2;
            int mapIdx = (headY + 1) * extWidth + (headX + 1);

            int tId = tunnelAnalyzer->cellToTunnelId[mapIdx];
            // //clog << headX << " " << headY << " " << tId << endl;
            if (tId != -1)
            {
                const Tunnel& tunnel = tunnelAnalyzer->tunnels[tId];

                // //clog << headX << " " << headY << " KARA " << endl;
                score -= 500;
            }
        }
        score += space * 50;
    }
};

class BeamSearch
{
public:
    static int getBestSurvivalMove(const Map& map, const Snake& snake, const vector<Snake>& enemies,
                                   const vector<GraphNode>& nodes, Point targetPoint, TunnelAnalyzer& tunelAnalyzer)
    {
        int BEAM_WIDTH = 10;
        int MAX_DEPTH = 4;
        // //clog << "pre init " << endl;
        vector<LightState> beam;
        LightState root;
        root.init(map, snake, enemies, nodes, &tunelAnalyzer);
        beam.push_back(root);
        // //clog << " po init " << endl;
        for (int depth = 0; depth < MAX_DEPTH; ++depth)
        {
            // //clog << " popo init " << endl;
            vector<LightState> nextBeam;
            nextBeam.reserve(beam.size() * 4);

            for (auto& state : beam)
            {
                if (!state.alive)
                {
                    // //clog<< "dead: " << state.headX << " " << state.headY << " " << state.score << endl;
                    nextBeam.push_back(state);
                    continue;
                }

                for (int dir = 0; dir < 4; ++dir)
                {
                    LightState nextState = state;
                    // //clog<< "nextState1: " << state.headX << " " << state.headY << " " << state.score << endl;

                    if (depth == 0)
                    {
                        nextState.firstMove = dir;
                    }
                    // //clog<< "nextState2: " << state.headX << " " << state.headY << " " << state.score << endl;

                    nextState.simulate(dir);
                    // //clog<< "nextState3: " << state.headX << " " << state.headY << " " << state.score << endl;

                    nextState.evaluate(targetPoint);
                    // //clog<< "nextState4: " << state.headX << " " << state.headY << " " << state.score << endl;

                    nextBeam.push_back(nextState);
                    // //clog<< "nextState: " << state.headX << " " << state.headY << " " << state.score << endl;
                }
            }

            sort(nextBeam.begin(), nextBeam.end(), [](const LightState& a, const LightState& b)
            {
                return a.score > b.score;
            });
            if (nextBeam.size() > BEAM_WIDTH)
            {
                nextBeam.resize(BEAM_WIDTH);
            }
            beam = nextBeam;
        }
        if (!beam.empty() && beam[0].alive)
        {
            return beam[0].firstMove;
        }
        else if (!beam.empty())

        {
            return beam[0].firstMove;
        }
        return -1;
    }
};

// ==========================================
// 3. MENEDŻERY MAPY I STANÓW
// ==========================================

class TargetManager
{
public:
    vector<Target> targets;
    int targetIdCounter = 0;
    bool missingTargetProcedure = false;

    TargetManager()
    {
        //clog << "[INFO] Target Manager constructed\n";
    }

    void addTarget(int x, int y)
    {
        targets.emplace_back(targetIdCounter++, x, y);
    }

    void setTargetExistence(int tx, int ty)
    {
        auto it = ranges::find_if(targets, [tx, ty](const auto& t)
        {
            return t.x == tx && t.y == ty;
        });
        if (it != targets.end())
            it->exists = true;
    }
};

class SnakeManager
{
public:
    vector<Snake> my_snakes;
    vector<Snake> enemy_snakes;

    void addSnake(int id, bool isMine)
    {
        if (isMine)
            my_snakes.emplace_back(id);
        else
            enemy_snakes.emplace_back(id);
    }

    void sortSnakes()
    {
        ranges::sort(my_snakes, {}, &Snake::id);
        ranges::sort(enemy_snakes, {}, &Snake::id);
    }

    Snake* getSnake(int id)
    {
        auto it_my = ranges::find_if(my_snakes, [id](const Snake& s)
        {
            return s.id == id;
        });
        if (it_my != my_snakes.end())
            return &(*it_my);

        auto it_enemy = ranges::find_if(enemy_snakes, [id](const Snake& s)
        {
            return s.id == id;
        });
        if (it_enemy != enemy_snakes.end())
            return &(*it_enemy);

        return nullptr;
    }
};

// ==========================================
// 4. PATHFINDING
// ==========================================

class Pathfinder
{
public:
    struct AStarResult
    {
        vector<string> path;
        Point endPoint;
    };

    struct Parent
    {
        Point p;
        int dir;
    };

    static vector<int> buildExpirationMap(const Map& map, const Snake& mySnake, const vector<Snake>& enemies)
    {
        vector<int> expMap(map.extWidth * map.extHeight, 0);

        // 1. Oznaczanie stałych przeszkód
        for (int y = 0; y < map.height; ++y)
        {
            for (int x = 0; x < map.width; ++x)
            {
                if (map.getCell(x, y) == '#')
                {
                    expMap[map.getIndex(x, y)] = 200;
                }
            }
        }
        // 2. Oznaczanie ciał węży (istniejąca logika)
        auto mapSnake = [&](const Snake& s)
        {
            if (!s.exists || s.bodyPos.empty()) return;
            int timeToFree = s.bodyPos.size();
            for (const auto& p : s.bodyPos)
            {
                if (map.isInside(p.x, p.y))
                {
                    // //clog << p.x << "," << p.y << " " << max(expMap[map.getIndex(p.x, p.y)], timeToFree) << " " << timeToFree << " || ";
                    expMap[map.getIndex(p.x, p.y)] = max(expMap[map.getIndex(p.x, p.y)], timeToFree);
                }
                timeToFree--;
            }
        };
        // 3. PREDYKCJA RUCHU WROGA
        auto mapEnemyPrediction = [&](const Snake& e)
        {
            if (!e.exists || e.bodyPos.empty()) return;

            // Zamiana stringa na int (0=UP, 1=DOWN, 2=LEFT, 3=RIGHT)
            int lastDirInt = -1;
            if (e.last_move == "UP") lastDirInt = 0;
            else if (e.last_move == "DOWN") lastDirInt = 1;
            else if (e.last_move == "LEFT") lastDirInt = 2;
            else if (e.last_move == "RIGHT") lastDirInt = 3;

            // Blokujemy wszystkie możliwe przyszłe pola wokół głowy wroga
            for (int i = 0; i < 4; i++)
            {
                if (isOpposite(lastDirInt, i)) continue; // Używamy Twojego isOpposite!

                int nx = e.headPos.x + DX[i];
                int ny = e.headPos.y + DY[i];

                if (map.isInside(nx, ny) && map.getCell(nx, ny) != '#')
                {
                    int dangerTime = e.length + 1;
                    expMap[map.getIndex(nx, ny)] = max(expMap[map.getIndex(nx, ny)], dangerTime);
                }
            }
        };
        mapSnake(mySnake);
        for (const auto& enemy : enemies)
        {
            mapSnake(enemy);
        }
        return expMap;
    }

    struct TimeNode
    {
        Point p;
        int lastDir;
        int g, f;
        int supportDist;
        int turn;
        bool operator>(const TimeNode& o) const { return f > o.f; }
    };

    static AStarResult findTimeAwarePath(const Map& map, const Snake& snake, const vector<Snake>& otherSnakes,
                                         Point end, const vector<int>& expirationMap, int startTurn = 0,
                                         const TunnelAnalyzer* ta = nullptr)
    {
        Point start = snake.headPos;
        string initialDirStr = snake.last_move;

        static vector<int> gScore;
        static vector<Parent> parent;
        static vector<int> visitedId;
        static vector<int> bestSupport;
        static int searchToken = 0;

        int totalStates = map.extWidth * map.extHeight * 5;
        if (gScore.size() < totalStates)
        {
            gScore.resize(totalStates);
            parent.resize(totalStates);
            visitedId.resize(totalStates, 0);
            bestSupport.resize(totalStates, INF);
        }
        searchToken++;

        auto getG = [&](int idx)
        {
            return visitedId[idx] == searchToken ? gScore[idx] : INF;
        };

        priority_queue<TimeNode, vector<TimeNode>, greater<TimeNode>> openSet;
        auto getStateIndex = [&](int x, int y, int dir)
        {
            return map.getIndex(x, y) * 5 + dir;
        };

        Point bestPoint = start;
        int bestDir = -1;
        int bestH = INF;
        int bestG = 0;

        int startDir = -1;
        if (initialDirStr == "UP")
            startDir = 0;
        else if (initialDirStr == "DOWN")
            startDir = 1;
        else if (initialDirStr == "LEFT")
            startDir = 2;
        else if (initialDirStr == "RIGHT")
            startDir = 3;

        int initialSupportDist = 0;

        for (int i = 0; i < snake.length; ++i)

        {
            Point bp = snake.bodyPos[i];
            bool isSupp = false;
            if (bp.y + 1 < map.height && map.getCell(bp.x, bp.y + 1) == '#')
                isSupp = true;
            if (map.getGraphCell(bp.x, bp.y) > -1)
                isSupp = true;

            if (isSupp)
            {
                initialSupportDist = i;
                break;
            }
        }

        int startIdxDir = (startDir == -1) ? 4 : startDir;
        int startStateIdx = getStateIndex(start.x, start.y, startIdxDir);

        visitedId[startStateIdx] = searchToken;
        gScore[startStateIdx] = startTurn;
        bestSupport[startStateIdx] = initialSupportDist;
        parent[startStateIdx] = {{-1, -1}, -1};

        int initialH = abs(start.x - end.x) + abs(start.y - end.y);
        openSet.push({start, startDir, startTurn, initialH, initialSupportDist});

        bool reachedTarget = false;
        int finalLastDir = -1;
        Point finalPoint = end;

        while (!openSet.empty())
        {
            TimeNode current = openSet.top();
            openSet.pop();

            int currIdxDir = (current.lastDir == -1) ? 4 : current.lastDir;
            int currStateIdx = getStateIndex(current.p.x, current.p.y, currIdxDir);

            if (current.g > getG(currStateIdx))
            {
                continue;
            }

            int currentH = abs(current.p.x - end.x) + abs(current.p.y - end.y);
            if (expirationMap[map.getIndex(end.x, end.y)] > 1 && !snake.helping)
            {
                //clog << "error " << expirationMap[map.getIndex(end.x, end.y)] << " " << end.x << " " << end.y << endl;
                return {};
            }
            if ((currentH < bestH || (currentH == bestH && current.g < bestG)))
            {
                bestH = currentH;
                bestG = current.g;
                bestPoint = current.p;
                bestDir = current.lastDir;
                // cerr << "BESTE: " << bestPoint.x << ' ' << bestPoint.y << " || ";
            }

            if (current.p == end)
            {
                finalLastDir = current.lastDir;
                finalPoint = current.p;
                // cerr << "finalLastDir: " << finalPoint.x << ' ' << finalPoint.y << endl;
                reachedTarget = true;
                break;
            }

            // int currentTurnSim = current.g;
            const int PREFERRED_DIRS[4] = {2, 3, 1, 0};
            for (int d = 0; d < 4; d++)
            {
                int i = PREFERRED_DIRS[d];
                if (isOpposite(current.lastDir, i))
                    continue;

                int nx = current.p.x + DX[i];
                int ny = current.p.y + DY[i];

                if (map.isInside(nx, ny))
                {
                    int nextIdx = map.getIndex(nx, ny);


                    // BLOKADA  TUNELI
                    // if (ta != nullptr) {
                    //     int tId = ta->cellToTunnelId[nextIdx];
                    //     if (tId != -1) {
                    //         const Tunnel& tunnel = ta->tunnels[tId];
                    //         // for (auto &a : tunnel.cells)
                    //             // //clog << a.x << " " << a.y << endl;
                    //         // Jeśli tunel jest ślepy i wąż docelowo się w nim nie zmieści...
                    //         if (true) {
                    //             int currIdx = map.getIndex(current.p.x, current.p.y);
                    //             int currTId = ta->cellToTunnelId[currIdx];
                    //             // //clog <<current.p.x << " " <<  current.p.y << "KARALL " <<  current.g << endl;
                    //             // Jeśli wąż próbuje wejść z zewnątrz (nie jest jeszcze w tym tunelu)
                    //             // if (currTId != tId && tunnel.isDeadEnd) {
                    //             //     continue; // ZABRONIONE! Odrzucamy ten kierunek.
                    //             // }
                    //             // else
                    //             // {
                    //             //     current.g += 1000;
                    //             //     // //clog << "KARALL " <<  current.g << endl;
                    //             // }
                    //         }
                    //     }
                    // }
                    // =======================================================

                    // //clog<< "Turn: " <<currentTurnSim + 1 << " " << expirationMap[nextIdx] << " " << nx << " " << ny<< " || ";
                    int currentTurnSim = current.turn;

                    for (int d = 0; d < 4; d++)
                    {
                        int i = PREFERRED_DIRS[d];
                        // //clog << "oposie? " << current.lastDir << " " << i << " " <<  (isOpposite(current.lastDir, i)) << " || ";
                        if (isOpposite(current.lastDir, i))
                            continue;

                        int nx = current.p.x + DX[i];
                        int ny = current.p.y + DY[i];

                        if (map.isInside(nx, ny))
                        {
                            int nextIdx = map.getIndex(nx, ny);

                            if ((currentTurnSim + 1 >= expirationMap[nextIdx] || snake.fighter) && expirationMap[
                                nextIdx] < 200)
                            {
                                int tentativeTurn = currentTurnSim + 1;

                                int requiredSpace = snake.length;
                                int availableSpace = calculateFutureSpaceWithBody(
                                    map, {nx, ny}, current.p, tentativeTurn, requiredSpace, expirationMap, parent,
                                    visitedId, searchToken); //BFS

                                if (availableSpace < requiredSpace && Point{nx, ny} != end)
                                {
                                    continue; //
                                }
                                // =======================================================

                                // poparcie
                                bool hasSupport = false;
                                if (ny + 1 < map.height && map.getCell(nx, ny + 1) == '#')
                                    hasSupport = true;
                                if (map.getGraphCell(nx, ny) > -1)
                                    hasSupport = true;

                                int nextSupportDist = hasSupport ? 0 : current.supportDist + 1;

                                //  kara za wiszenie w powietrzu
                                int specialScore = 0;
                                if (nextSupportDist >= snake.length && map.getCell(nx, ny + 1) != '#')
                                {
                                    int j = 0;
                                    while (map.getCell(nx, ny + j) != '#' && map.getCell(nx, ny + j) != 'P' && j < 10)
                                    {
                                        j++;
                                    }
                                    if (j >= 3)
                                    {
                                        specialScore = 1;
                                    }
                                }

                                int dangerPenalty = 0;
                                for (const auto& other : otherSnakes)
                                {
                                    if (!other.exists || other.bodyPos.empty()) continue;

                                    int distToHead = abs(nx - other.headPos.x) + abs(ny - other.headPos.y);

                                    if (distToHead <= 3 && distToHead > 0)
                                    {
                                        if (other.length >= snake.length)
                                        {
                                            dangerPenalty += 60 / distToHead;
                                        }
                                        else
                                        {
                                            if (!snake.fighter)
                                            {
                                                dangerPenalty += 15 / distToHead;
                                            }
                                        }
                                    }
                                }

                                int tentativeG = current.g + 10 + specialScore + dangerPenalty;
                                int nextStateIdx = getStateIndex(nx, ny, i);

                                bool betterG = tentativeG < getG(nextStateIdx);
                                bool betterSupport = nextSupportDist < (visitedId[nextStateIdx] == searchToken
                                                                            ? bestSupport[nextStateIdx]
                                                                            : INF);

                                if (betterG || betterSupport)
                                {
                                    visitedId[nextStateIdx] = searchToken;
                                    if (betterG)
                                    {
                                        gScore[nextStateIdx] = tentativeG;
                                        parent[nextStateIdx] = {current.p, current.lastDir};
                                    }
                                    if (betterSupport)
                                        bestSupport[nextStateIdx] = nextSupportDist;

                                    int h = (abs(nx - end.x) + abs(ny - end.y)) * 10;

                                    openSet.push({
                                        {nx, ny}, i, tentativeG, tentativeG + h, nextSupportDist, tentativeTurn
                                    });
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!reachedTarget)
        {
            if (start == end || bestPoint == start)
                return {{}, start};
            finalPoint = bestPoint;
            finalLastDir = bestDir;
            // cerr << endl << "BESTE2: " << bestPoint.x << ' ' << bestPoint.y << endl;
        }

        vector<string> path;
        Point currP = finalPoint;
        int currD = finalLastDir;

        while (currD != -1)
        {
            int stIdx = getStateIndex(currP.x, currP.y, currD);
            Parent p = visitedId[stIdx] == searchToken ? parent[stIdx] : Parent{{-1, -1}, -1};
            if (p.p.x == -1) {
                break;
            }

            path.push_back(DIR_NAMES[currD]);
            currP = p.p;
            currD = p.dir;
        }
        reverse(path.begin(), path.end());

        return {path, finalPoint};
    }

    static int calculateFutureSpaceWithBody(
        const Map& map, Point startPos, Point currentP, int startTurn, int limit,
        const vector<int>& expirationMap,
        const vector<Parent>& parentArray, const vector<int>& visitedId, int searchToken)
    {
        static int visToken = 0;
        static vector<int> localVisited;
        if (localVisited.size() < map.extWidth * map.extHeight)
        {
            localVisited.resize(map.extWidth * map.extHeight, 0);
        }
        visToken++;

        Point trace = currentP;
        int stepsBack = 0;

        while (trace.x != -1 && stepsBack < limit)
        {
            int traceIdx = map.getIndex(trace.x, trace.y);
            localVisited[traceIdx] = visToken;

            int stateIdx = -1;

            break;
        }

        //  BFS
        queue<pair<Point, int>> q;
        q.push({startPos, startTurn});
        localVisited[map.getIndex(startPos.x, startPos.y)] = visToken;

        int space = 0;

        while (!q.empty() && space < limit)
        {
            auto [currP, currTurn] = q.front();
            q.pop();
            space++;

            for (int i = 0; i < 4; i++)
            {
                int nx = currP.x + DX[i];
                int ny = currP.y + DY[i];

                if (map.isInside(nx, ny))
                {
                    int idx = map.getIndex(nx, ny);
                    if (localVisited[idx] != visToken) // Jeśli to nie jest nasze nowe ciało ani odwiedzone pole
                    {
                        if (currTurn + 1 >= expirationMap[idx])
                        {
                            localVisited[idx] = visToken;
                            q.push({{nx, ny}, currTurn + 1});
                        }
                    }
                }
            }
        }
        return space;
    }

    static int calculateFutureSpace(const Map& map, Point startPos, int startTurn, int limit,
                                    const vector<int>& expirationMap)
    {
        static int visToken = 0;
        static vector<int> visitedId;
        if (visitedId.size() < map.extWidth * map.extHeight)
        {
            visitedId.resize(map.extWidth * map.extHeight, 0);
        }
        visToken++;

        queue<pair<Point, int>> q;
        q.push({startPos, startTurn});
        visitedId[map.getIndex(startPos.x, startPos.y)] = visToken;

        int space = 0;

        while (!q.empty() && space < limit)
        {
            auto [currP, currTurn] = q.front();
            q.pop();
            space++;

            for (int i = 0; i < 4; i++)
            {
                int nx = currP.x + DX[i];
                int ny = currP.y + DY[i];

                if (map.isInside(nx, ny))
                {
                    int idx = map.getIndex(nx, ny);
                    if (visitedId[idx] != visToken)
                    {
                        if (currTurn + 1 >= expirationMap[idx])
                        {
                            visitedId[idx] = visToken;
                            q.push({{nx, ny}, currTurn + 1});
                        }
                    }
                }
            }
        }
        return space;
    }

    static vector<string> findSafePathToAssignedTarget(const Map& map, Snake& snake, const vector<Snake>& enemies,
                                                       bool ignore, const TunnelAnalyzer* ta = nullptr)
    {
        if (snake.path.empty())
            return {};

        Point target = snake.path.front();
        vector<int> expMap = buildExpirationMap(map, snake, enemies);
        //clog << "snake.path.size() " << snake.path.size() << endl;
        while (snake.path.size() > 1 && target.x == snake.headPos.x && target.y >= snake.headPos.y && map.getCell(
            target.x, target.y) != 'S')

        {
            //clog << "ERASE: " << target.x << "," << target.y << endl;
            snake.path.erase(snake.path.begin());
            target = snake.path.front();
        }
        // if (snake.id == 1)
        // for (int y = 0; y <= map.height; y++)
        // {
        //     for (int x = 0; x <= map.width; x++)
        //
        //     {
        //         //clog << expMap[map.getIndex(x, y)] << " \t";
        //     }
        //     //clog << endl;
        // }
        auto result = findTimeAwarePath(map, snake, enemies, target, expMap, 0, ta);
        // auto result = findTimeAwarePath(map, snake, target, expMap, 0, ta);

        int turnsToReach = result.path.size();
        int predictedLength = snake.length + 1;

        int futureSpace = calculateFutureSpace(map, result.endPoint, turnsToReach, predictedLength + 2, expMap);

        if (futureSpace < predictedLength && !ignore)
        {
            //clog << "ODRZUCAM PRZYPISANY CEL " << target.x << "," << target.y < " - zablokuję się na końcu drogi! Miejsce: (" << futureSpace << "/" << predictedLength << ")\n";
            return {};
        }
        // //clog << "kończę" << endl;
        return result.path;
    }

    static void buildGraphEdges(const Map& map, GraphNode& node)
    {
        vector<int> distMap(map.extWidth * map.extHeight, UNVISITED);
        vector<int> q(map.extWidth * map.extHeight);
        int head = 0, tail = 0;

        int startIdx = map.getIndex(node.pos.x, node.pos.y);
        distMap[startIdx] = 0;
        q[tail++] = startIdx;

        while (head < tail)
        {
            int currIdx = q[head++];
            int currY = (currIdx / map.extWidth) - 1;
            int currX = (currIdx % map.extWidth) - 1;

            for (int i = 0; i < 4; i++)
            {
                int nx = currX + DX[i];
                int ny = currY + DY[i];

                if (map.isInside(nx, ny))
                {
                    int nIdx = map.getIndex(nx, ny);
                    if (distMap[nIdx] == UNVISITED)
                    {
                        char cellVal = map.getCell(nx, ny);
                        if (cellVal == '#')
                            continue;

                        if ((cellVal == 'N' || cellVal == 'S') && map.getGraphCell(nx, ny) != node.id)
                        {
                            int dist = distMap[currIdx] + 1;
                            // if (ny >= node.pos.y)
                            // {
                            // dist--;
                            // }
                            if (dist <= map.height)
                            {
                                node.neighbours.emplace_back(map.getGraphCell(nx, ny), dist);
                            }
                        }
                        distMap[nIdx] = distMap[currIdx] + 1;
                        q[tail++] = nIdx;
                    }
                }
            }
        }
    }

    static unordered_map<int, PathResult> findAllGraphPaths(int startNodeId, char targetType,
                                                            const vector<GraphNode>& graphNodes, int maxLength)
    {
        int n = graphNodes.size();
        vector<int> dist(n, INF);
        vector<int> parent(n, -1);
        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;

        dist[startNodeId] = 0;
        pq.push({0, startNodeId});

        unordered_map<int, PathResult> results;

        while (!pq.empty())
        {
            auto [currentDist, u] = pq.top();
            pq.pop();

            if (currentDist > dist[u])
                continue;
            if (u < 0 || u >= n)
                continue;

            if (graphNodes[u].type == targetType && graphNodes[u].isAvailable)
            {
                vector<int> path;
                for (int curr = u; curr != -1; curr = parent[curr])
                    path.push_back(curr);
                reverse(path.begin(), path.end());
                results[u] = {path, dist[u]};
            }

            for (const auto& neighbor : graphNodes[u].neighbours)
            {
                int v = neighbor.first, weight = neighbor.second;

                if (weight > maxLength || weight <= 1)
                    continue;
                if (v < 0 || v >= n || !graphNodes[v].isActive)
                    continue;

                if (dist[u] + weight < dist[v])
                {
                    dist[v] = dist[u] + weight;
                    parent[v] = u;
                    pq.push({dist[v], v});
                }
            }
        }
        return results;
    }

    static int getVoronoiSpace(const Map& map, Point start, const vector<Snake>& enemies, int limit)
    {
        if (!map.isInside(start.x, start.y))
            return 0;

        char initialCell = map.getCell(start.x, start.y);
        if (initialCell == '#' || initialCell == 'P')
            return 0;

        int totalCells = map.extWidth * map.extHeight;
        static vector<int> dist;
        static vector<int> owner;
        static vector<int> visToken;
        static int vToken = 0;

        if (dist.size() < totalCells)
        {
            dist.resize(totalCells);
            owner.resize(totalCells);
            visToken.resize(totalCells, 0);
        }
        vToken++;

        queue<Point> q;

        int enemyId = 1;
        for (const auto& enemy : enemies)
        {
            if (!enemy.exists || enemy.bodyPos.empty())
                continue;
            Point ep = enemy.headPos;
            if (map.isInside(ep.x, ep.y))
            {
                int eIdx = map.getIndex(ep.x, ep.y);
                visToken[eIdx] = vToken;
                dist[eIdx] = 0;
                owner[eIdx] = enemyId;
                q.push(ep);
            }
            enemyId++;
        }

        int myIdx = map.getIndex(start.x, start.y);
        if (visToken[myIdx] == vToken && owner[myIdx] != 0)
            return 0;

        visToken[myIdx] = vToken;
        dist[myIdx] = 0;
        owner[myIdx] = 0;
        q.push(start);

        int mySpace = 0;

        while (!q.empty() && mySpace < limit)
        {
            Point curr = q.front();
            q.pop();

            int currIdx = map.getIndex(curr.x, curr.y);
            if (owner[currIdx] == 0)
            {
                mySpace++;
            }

            for (int i = 0; i < 4; i++)
            {
                int nx = curr.x + DX[i];
                int ny = curr.y + DY[i];

                if (map.isInside(nx, ny))
                {
                    char cell = map.getCell(nx, ny);
                    if (cell == '.' || cell == 'S' || cell == 'N')
                    {
                        int nIdx = map.getIndex(nx, ny);
                        if (visToken[nIdx] != vToken)
                        {
                            visToken[nIdx] = vToken;
                            dist[nIdx] = dist[currIdx] + 1;
                            owner[nIdx] = owner[currIdx];
                            q.push({nx, ny});
                        }
                    }
                }
            }
        }
        return mySpace;
    }
};


// ==========================================
// 5. LOGIKA
// ==========================================


struct GlobalOption
{
    int snakeId;
    int targetNodeId;
    int cost;
    vector<Point> pathPoints;
    int firstNodeId;
};

class Game
{
public:
    Map map;
    SnakeManager snakeManager;
    TargetManager targetManager;
    vector<GraphNode> graphNodes;
    TunnelAnalyzer tunnelAnalyzer;
    int sourceCount = 0;

    Game(int height, int width) : map(height, width)
    {
        //clog << "Game start" << endl;
    }

    void setSourcesCount(int count) { sourceCount = count; }

    void addTarget(int x, int y)
    {
        if (map.getCell(x, y) != 'S')
        {
            targetManager.addTarget(x, y);
            map.setCell(x, y, 'S');

            int newId = graphNodes.size();
            graphNodes.emplace_back(newId, Point{x, y}, 'S');
            map.setGraphCell(x, y, newId);

            if (y > 0 && map.getCell(x, y - 1) != '#' && map.getCell(x, y - 1) != 'S')
            {
                map.setCell(x, y - 1, 'N');
                int newUpId = graphNodes.size();
                graphNodes.emplace_back(newUpId, Point{x, y - 1}, 'N');
                map.setGraphCell(x, y - 1, newUpId);
            }
        }
        targetManager.setTargetExistence(x, y);
    }

    void updateSnake(int id, const string& body)
    {
        Snake* snake = snakeManager.getSnake(id);
        if (!snake)
            return;

        snake->updateBody(body);
        for (const auto& p : snake->bodyPos)
        {
            map.setCell(p.x, p.y, 'P');
            int graphId = map.getGraphCell(p.x, p.y);
            if (graphId > -1 && graphId < graphNodes.size())
            {
                graphNodes[graphId].isAvailable = false;
            }
        }
    }

    void findGraphNodes()
    {
        for (int i = 1; i < map.height; i++)
        {
            for (int j = 0; j < map.width; j++)
            {
                if (map.getCell(j, i) == '#' && map.getCell(j, i - 1) != '#')
                {
                    int newId = graphNodes.size();
                    graphNodes.emplace_back(newId, Point{j, i - 1}, 'N');
                    map.setCell(j, i - 1, 'N');
                    map.setGraphCell(j, i - 1, newId);
                }
            }
        }
        //clog << " checpointst size: " << graphNodes.size() << endl;
    }

    void makeGraph()
    {
        for (auto& node : graphNodes)
        {
            Pathfinder::buildGraphEdges(map, node);
            sort(node.neighbours.begin(), node.neighbours.end(), [](const auto& a, const auto& b)
            {
                return a.second < b.second;
            });
        }
    }

    void assignTargets()
    {
        //clog << "====SET TARGET===== " << endl;
        unlockPreviousTargets();

        unordered_set<int> assignedSnakes;
        unordered_set<int> assignedTargets;

        unordered_map<int, pair<int, int>> snakeFallbacks;
        vector<GlobalOption> allOptions = evaluateAllSnakeOptions(snakeFallbacks, assignedSnakes);

        // if (targetManager.targets.size() < 20 && targetManager.targets.size() > snakeManager.my_snakes.size() * 2)
        //
        // {
        //     distributeTargetsHungarian(allOptions, assignedSnakes, assignedTargets);
        // }
        // else
        {
            distributeTerritorialTargets(allOptions, assignedSnakes, assignedTargets);
        }
        // assignEnemyTailFallback(s)
        assignEmergencyFallbacks(assignedSnakes);
    }

    void distributeTargetsHungarian(const vector<GlobalOption>& allOptions, unordered_set<int>& assignedSnakes,
                                    unordered_set<int>& assignedTargets)
    {
        vector<int> snakeIds;
        vector<int> targetIds;

        for (const auto& opt : allOptions)
        {
            if (!assignedSnakes.count(opt.snakeId) && ranges::find(snakeIds, opt.snakeId) == snakeIds.end())
            {
                snakeIds.push_back(opt.snakeId);
            }
            if (!assignedTargets.count(opt.targetNodeId) && ranges::find(targetIds, opt.targetNodeId) == targetIds.
                end())
            {
                targetIds.push_back(opt.targetNodeId);
            }
        }

        if (snakeIds.empty() || targetIds.empty())
            return;

        int N = snakeIds.size();
        int M = targetIds.size();

        const int INF_COST = 1000000;
        vector<vector<int>> costMatrix(N, vector<int>(M, INF_COST));
        vector<vector<const GlobalOption*>> optionMatrix(N, vector<const GlobalOption*>(M, nullptr));

        for (const auto& opt : allOptions)
        {
            if (assignedSnakes.count(opt.snakeId) || assignedTargets.count(opt.targetNodeId))
                continue;

            auto itSnake = ranges::find(snakeIds, opt.snakeId);
            auto itTarget = ranges::find(targetIds, opt.targetNodeId);

            if (itSnake != snakeIds.end() && itTarget != targetIds.end())
            {
                int sIdx = distance(snakeIds.begin(), itSnake);
                int tIdx = distance(targetIds.begin(), itTarget);

                if (opt.cost < costMatrix[sIdx][tIdx])
                {
                    costMatrix[sIdx][tIdx] = opt.cost;
                    optionMatrix[sIdx][tIdx] = &opt;
                }
            }
        }

        vector<int> assignments = solveHungarian(costMatrix);

        for (int i = 0; i < N; ++i)
        {
            int assignedColumn = assignments[i];

            if (assignedColumn != -1 && costMatrix[i][assignedColumn] < INF_COST)
            {
                const GlobalOption* bestOpt = optionMatrix[i][assignedColumn];
                if (bestOpt)
                {
                    assignedSnakes.insert(bestOpt->snakeId);
                    assignedTargets.insert(bestOpt->targetNodeId);

                    Snake* s = snakeManager.getSnake(bestOpt->snakeId);
                    s->path = bestOpt->pathPoints;
                    s->targetPoint = bestOpt->pathPoints.front();
                    s->targetID = bestOpt->targetNodeId;
                    graphNodes[bestOpt->targetNodeId].isAvailable = false;

                    for (auto a : bestOpt->pathPoints)

                    {
                        //clog << a.x << " " << a.y << " || ";
                    }
                    //clog << endl;
                }
            }
        }
    }

    vector<int> solveHungarian(const vector<vector<int>>& costMatrix)
    {
        if (costMatrix.empty() || costMatrix[0].empty())
            return {};

        int n = costMatrix.size();
        int m = costMatrix[0].size();
        int k = max(n, m);

        vector<vector<int>> a(k + 1, vector<int>(k + 1, 1000000));
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < m; ++j)
            {
                a[i + 1][j + 1] = costMatrix[i][j];
            }
        }

        vector<int> u(k + 1), v(k + 1), p(k + 1), way(k + 1);

        for (int i = 1; i <= k; ++i)
        {
            p[0] = i;
            int j0 = 0;
            vector<int> minv(k + 1, 2e9);
            vector<char> used(k + 1, false);

            do
            {
                used[j0] = true;
                int i0 = p[j0], delta = 2e9, j1 = 0;

                for (int j = 1; j <= k; ++j)
                {
                    if (!used[j])
                    {
                        int cur = a[i0][j] - u[i0] - v[j];
                        if (cur < minv[j])
                        {
                            minv[j] = cur;
                            way[j] = j0;
                        }
                        if (minv[j] < delta)
                        {
                            delta = minv[j];
                            j1 = j;
                        }
                    }
                }

                for (int j = 0; j <= k; ++j)
                {
                    if (used[j])
                    {
                        u[p[j]] += delta;
                        v[j] -= delta;
                    }
                    else
                    {
                        minv[j] -= delta;
                    }
                }
                j0 = j1;
            }
            while (p[j0] != 0);

            do
            {
                int j1 = way[j0];
                p[j0] = p[j1];
                j0 = j1;
            }
            while (j0 != 0);
        }

        vector<int> ans(n, -1);
        for (int j = 1; j <= k; ++j)
        {
            if (p[j] <= n && j <= m)
            {
                ans[p[j] - 1] = j - 1;
            }
        }
        return ans;
    }

    void calculateMovements()
    {
        //clog << "=====FIND MOVEMENT===" << endl;
        for (auto& snake : snakeManager.my_snakes)
        {
            if (snake.move_horizont.empty())
            {
                planMovementForSnake(snake);
            }
        }
    }

    void executeMoves()
    {
        //clog << "====PRINT MOVE =====" << endl;
        for (auto& snake : snakeManager.my_snakes)
        {
            if (!snake.move_horizont.empty())
            {
                //clog << snake.id << " " << snake.move_horizont[0] << ";" << endl;
                cout << snake.id << " " << snake.move_horizont[0] << ";";
                snake.last_move = snake.move_horizont[0];
                snake.move_horizont.erase(snake.move_horizont.begin());

                if (snake.headPos.x == snake.prevHeadPos.x && snake.headPos.y == snake.prevHeadPos.y)
                {
                    snake.move_horizont.clear();
                    //clog << "LAG " << snake.id << " " << endl;
                }
                snake.prevHeadPos = snake.headPos;
                //clog << "POS " << snake.id << " " << endl;
            }
            else
            {
                cout << "WAIT;";
            }
        }
        cout << endl;
    }

    void clearUnexistedTargets()
    {
        erase_if(targetManager.targets, [&](const auto& target)
        {
            if (!target.exists)
            {
                clearSingleTarget(target);
                return true;
            }
            return false;
        });

        erase_if(snakeManager.my_snakes, [&](const auto& snake)
        {
            if (!snake.exists)
            {
                if (snake.targetID > -1 && snake.targetID < graphNodes.size())
                {
                    graphNodes[snake.targetID].isAvailable = true;
                }
                //clog << "delete: " << snake.id << " freeTarget " << snake.targetID << "\n";
                return true;
            }
            return false;
        });
    }

    void clearRound()
    {
        auto clearSnakeBody = [&](auto& snakes)
        {
            for (auto& s : snakes)
            {
                for (const auto& b : s.bodyPos)
                {
                    map.setCell(b.x, b.y, '.');
                    int graphId = map.getGraphCell(b.x, b.y);
                    if (graphId > -1 && graphId < graphNodes.size())
                    {
                        graphNodes[graphId].isActive = true;
                        graphNodes[graphId].isAvailable = true;
                    }
                }
                // s.bodyPos.clear();
                s.exists = false;
                s.fighter = false;
                s.move_horizont.clear();
                s.path.clear();
            }
        };
        for (auto node : graphNodes)
        {
            node.isAvailable = true;
        }
        clearSnakeBody(snakeManager.my_snakes);
        clearSnakeBody(snakeManager.enemy_snakes);

        for (auto& t : targetManager.targets)
            t.exists = false;
    }

private:
    void unlockPreviousTargets()
    {
        for (auto& snake : snakeManager.my_snakes)
        {
            if (snake.targetID != -1 && snake.targetID < graphNodes.size())
            {
                graphNodes[snake.targetID].isAvailable = true;
            }
        }
    }

    int getGraphNodeUnderSnake(const Snake& snake)
    {
        //clog << " bodyPos ";
        for (auto& node : snake.bodyPos)
        {
            if (map.getCell(node.x, node.y) == '#')
                break;
            int foundNodeId = map.getGraphCell(node.x, node.y);
            if (foundNodeId > -1)
            {
                return foundNodeId;
            }
        }
        for (auto& point : snake.bodyPos)
        {
            int bodyNodeId = map.getGraphCell(point.x, point.y);
            if (bodyNodeId > -1)
            {
                return bodyNodeId;
            }
        }
        return -1;
    }

    vector<GlobalOption> evaluateAllSnakeOptions(unordered_map<int, pair<int, int>>& snakeFallbacks,
                                                 const unordered_set<int>& assignedSnakes)
    {
        vector<GlobalOption> allOptions;
        //clog << "--- START evaluateAllSnakeOptions ---" << endl;
        vector<pair<Point, int>> enemyHead;
        for (const auto& enemy : snakeManager.enemy_snakes)
        {
            enemyHead.emplace_back(enemy.headPos, enemy.length);
        }
        for (auto& snake : snakeManager.my_snakes)
        {
            //clog << "[EVAL] Sprawdzam weza ID: " << snake.id << endl;
            vector<pair<Point, int>> myHead;
            for (const auto& my : snakeManager.enemy_snakes)
            {
                if (my.id != snake.id)
                {
                    myHead.emplace_back(my.headPos, my.length);
                }
            }
            int nodeId = getGraphNodeUnderSnake(snake);
            if (nodeId <= -1)
            {
                //clog << " -> Pomijam (brak aktywnego wezla pod cialem weza)" << endl;
                continue;
            }
            graphNodes[nodeId].isAvailable = false;
            vector<pair<int, int>> validNodes;
            for (auto& n : graphNodes[nodeId].neighbours)
            {
                if (n.second <= snake.length)
                {
                    if (graphNodes[n.first].isActive && graphNodes[n.first].isAvailable)
                    {
                        validNodes.push_back(n);
                    }
                }
                else
                {
                    break;
                }
            }
            //clog << graphNodes[nodeId].pos.x << " " << graphNodes[nodeId].pos.y << " -> Znaleziono " << validNodes.size() << " poprawnych sasiadow (w zasiegu dlugosci: " << snake.length << ")" << endl;

            int bestFallbackDist = INF;
            int fallbackNodeId = -1;

            for (auto& n : validNodes)
            {
                int headDist = abs(graphNodes[n.first].pos.x - snake.headPos.x) +
                    abs(graphNodes[n.first].pos.y - snake.headPos.y);

                if (headDist < bestFallbackDist && map.getCell(graphNodes[n.first].pos.x, graphNodes[n.first].pos.y) !=
                    'P')
                {
                    bestFallbackDist = headDist;
                    fallbackNodeId = n.first;
                }

                auto targetsFound = Pathfinder::findAllGraphPaths(n.first, 'S', graphNodes, snake.length);
                for (auto& [targetId, pathResult] : targetsFound)
                {
                    if (pathResult.path.empty())
                        continue;

                    int baseCost = pathResult.totalLength + headDist;
                    int strategicBonus = calculateStrategicBonus(targetId, snake.length, graphNodes);

                    Point targetPos = graphNodes[targetId].pos;
                    int special = 0;

                    int distanceEnemy = 0;
                    int distanceMy = 0;
                    for (auto head : enemyHead)
                    {
                        distanceEnemy = abs(head.first.x - targetPos.x) + abs(head.first.y - targetPos.y);
                        distanceMy = abs(snake.headPos.x - targetPos.x) + abs(snake.headPos.y - targetPos.y);
                        // special -= 25 / (distanceEnemy + distanceMy);
                        if (distanceEnemy <= head.second && distanceMy > distanceEnemy)
                        {
                            special -= 5;
                        }
                        else
                        {
                            special -= 5;
                        }
                    }
                    int totalCost = baseCost * 2 - strategicBonus + special;
                    vector<Point> pathPoints;
                    for (int pId : pathResult.path)
                    {
                        pathPoints.push_back(graphNodes[pId].pos);
                    }
                    // graphNodes[pathResult.path.front()].isAvailable = false;
                    allOptions.push_back({snake.id, targetId, totalCost, pathPoints, pathResult.path.front()});
                }
            }

            if (fallbackNodeId != -1)
            {
                snakeFallbacks[snake.id] = {fallbackNodeId, bestFallbackDist};
                //clog << " -> Ustawiono fallback na wezel ID: " << graphNodes[fallbackNodeId].pos.x << " " << graphNodes[fallbackNodeId].pos.y << " (dystans: " << bestFallbackDist << ")" << endl;
            }
            else
            {
                //clog << " -> UWAGA: Brak fallbacku z tego wezla!" << endl;
            }
        }

        //clog << "--- KONIEC evaluateAllSnakeOptions (Wygenerowano lacznie " << allOptions.size() << "  opcji) ---" << endl;
        return allOptions;
    }

    int calculateStrategicBonus(int targetNodeId, int snakeLength, const vector<GraphNode>& graphNodes)
    {
        int bonus = 0;

        for (const auto& edge : graphNodes[targetNodeId].neighbours)
        {
            int neighborId = edge.first;
            int jumpDist = edge.second;

            if (jumpDist <= snakeLength && graphNodes[neighborId].isActive)
            {
                if (graphNodes[neighborId].type == 'S' && graphNodes[neighborId].isAvailable)
                {
                    bonus += 2 / (jumpDist + 1);
                }
            }
        }
        return bonus;
    }


    void distributeTerritorialTargets(const vector<GlobalOption>& allOptions, unordered_set<int>& assignedSnakes,
                                      unordered_set<int>& assignedTargets)
    {
        vector<Point> assignedTargetPoints;

        while (assignedSnakes.size() < snakeManager.my_snakes.size())
        {
            int bestIdx = -1;
            int bestEffectiveCost = INF;

            for (int i = 0; i < allOptions.size(); i++)
            {
                const auto& opt = allOptions[i];
                if (assignedSnakes.count(opt.snakeId) || assignedTargets.count(opt.targetNodeId))

                {
                    // //clog<< "continue " << opt.snakeId << endl;
                    continue;
                }
                int spatialPenalty = calculateSpatialPenalty(graphNodes[opt.targetNodeId].pos, assignedTargetPoints);
                int effectiveCost = opt.cost + spatialPenalty;
                // //clog<< "not continue " << opt.snakeId << endl;
                if (effectiveCost < bestEffectiveCost)
                {
                    bestEffectiveCost = effectiveCost;
                    // //clog<< "signed " << opt.snakeId << " " << graphNodes[allOptions[i].targetNodeId].pos.x << " " << graphNodes[allOptions[i].targetNodeId].pos.y << endl;
                    bestIdx = i;
                }
            }
            // //clog<< "bestIdx " << bestIdx << " " << graphNodes[bestIdx].pos.x << " " << graphNodes[bestIdx].pos.y << endl;
            if (bestIdx == -1)
                break;

            const auto& opt = allOptions[bestIdx];
            assignedSnakes.insert(opt.snakeId);
            assignedTargets.insert(opt.targetNodeId);
            assignedTargetPoints.push_back(graphNodes[opt.targetNodeId].pos);

            Snake* s = snakeManager.getSnake(opt.snakeId);
            s->path = opt.pathPoints;
            s->targetPoint = opt.pathPoints.front();
            s->targetID = opt.targetNodeId;
            graphNodes[opt.targetNodeId].isAvailable = false;

            //clog << "TERYTORY ASSIGN: Snake " << s->id << " -> Target " << graphNodes[opt.targetNodeId].pos.x << " " <<  graphNodes[opt.targetNodeId].pos.y << " (Baza: " << opt.cost << ", Z kara za tlok: " << bestEffectiveCost << ")" << endl;
        }
    }

    int calculateSpatialPenalty(const Point& targetPos, const vector<Point>& assignedTargetPoints)
    {
        int spatialPenalty = 0;
        int repulsionRadius = 12;

        for (const Point& assignedP : assignedTargetPoints)
        {
            int dist = abs(targetPos.x - assignedP.x) + abs(targetPos.y - assignedP.y);
            if (dist < repulsionRadius)
            {
                spatialPenalty += (repulsionRadius - dist) * map.width / 10;
            }
        }
        return spatialPenalty;
    }

    void assignEmergencyFallbacks(const unordered_set<int>& assignedSnakes)
    {
        for (auto& snake : snakeManager.my_snakes)
        {
            if (!assignedSnakes.count(snake.id))
            {
                // if (snake.helper != -1)
                // for (auto &helper : snakeManager.my_snakes)
                // {
                //     if (helper.id != snake.id && !assignedSnakes.count(helper.id))
                //     {
                //         //clog << "HELPER, POMAGA " << helper.id << " " << snake.id << endl;
                //         // snake.isHead = true;
                //         helper.path.push_back(snake.bodyPos.back());
                //         snake.helper = helper.id;
                //         helper.helping = true;
                //         // executeBS(snake,snake.bodyPos.back());
                //     }
                // }
                // if (snake.helper > -1 && snake.path.empty())
                // {
                // snake.length += snakeManager.my_snakes[snake.helper].length;
                executeBS(snake);
                // }
            }
        }
    }

    void executeBS(Snake& snake, const Point& targetPos = {-100, -100})
    {
        //clog << "robię " << snake.id << endl;
        // assignEnemyTailFallback(snake);
        // if (snake.path.empty())
        // {
        int bestDistance = 10000;
        int targetDistance = 0;
        Point bestTarget = targetPos;
        if (bestTarget.x == -100)
        {
            for (auto& target : targetManager.targets)
            {
                targetDistance = manhattanDist({target.x, target.y}, snake.headPos);
                if (targetDistance < bestDistance)
                {
                    bestDistance = targetDistance;
                    bestTarget = {target.x, target.y};
                }
            }
        }
        //clog << "--- BEAM SEARCH ODPALONY DLA WEZA " << snake.id << " ---" << bestTarget.x << " " << bestTarget.y << endl;
        int bestDir = BeamSearch::getBestSurvivalMove(map, snake, snakeManager.enemy_snakes, graphNodes, bestTarget,
                                                      tunnelAnalyzer);
        //clog << "BEAM SEARCH RESCUE: Wąż " << snake.id << " ratuje się ruchem " << DIR_NAMES[bestDir] << " (Przewidziano bezpieczną przyszłość!)" << endl;
        snake.move_horizont = {DIR_NAMES[bestDir]};
        Point nextP = {snake.headPos.x + DX[bestDir], snake.headPos.y + DY[bestDir]};
        if (map.getCell(nextP.x, nextP.y) != '#')
        {
            map.setCell(nextP.x, nextP.y, 'P');
            snake.bodyPos.insert(snake.bodyPos.begin(), nextP);
            //clog << " Reserve: " << nextP.x << " " << nextP.y << endl;
        }
        // }
    }

    bool assignMyTailFallback(Snake& snake)
    {
        Point bestTail = {-1, -1};
        int bestTailDist = INF;

        for (auto& enemy : snakeManager.my_snakes)
        {
            if (!enemy.exists || enemy.bodyPos.empty() || enemy.id == snake.id)
                continue;

            Point enemyTail = enemy.bodyPos.back();
            int dist = abs(snake.headPos.x - enemyTail.x) + abs(snake.headPos.y - enemyTail.y);

            if (dist < bestTailDist && enemy.id > snake.id)
            {
                bestTailDist = dist;
                bestTail = enemyTail;
                //clog << "TACTICAL FALLBACK: Snake " << snake.id << " wali w ogon " << enemy.id << " xy " << bestTail.x << "," << bestTail.y << endl;
            }
        }

        if (bestTailDist != INF)
        {
            snake.targetPoint = bestTail;
            snake.path = {bestTail};
            //clog << "TACTICAL FALLBACK: Snake " << snake.id << " wali w ogon " << bestTail.x << "," << bestTail.y <<  endl;

            return true;
        }
        return false;
    }

    bool assignEnemyTailFallback(Snake& snake)
    {
        Point bestTarget = {-1, -1};
        int bestTargetDist = INF;

        for (const auto& enemy : snakeManager.enemy_snakes)
        {
            if (!enemy.exists || enemy.bodyPos.empty() || enemy.id == snake.id) continue;

            if (enemy.length < snake.length)
            {
                int eDir = -1;
                if (enemy.last_move == "UP") eDir = 0;
                else if (enemy.last_move == "DOWN") eDir = 1;
                else if (enemy.last_move == "LEFT") eDir = 2;
                else if (enemy.last_move == "RIGHT") eDir = 3;

                Point predHead = enemy.headPos;
                if (eDir != -1)
                {
                    int nx = enemy.headPos.x + DX[eDir];
                    int ny = enemy.headPos.y + DY[eDir];

                    if (map.isInside(nx, ny) && map.getCell(nx, ny) != '#')
                    {
                        predHead = {nx, ny};
                    }
                }

                int dist = abs(snake.headPos.x - predHead.x) + abs(snake.headPos.y - predHead.y);

                if (dist < bestTargetDist)
                {
                    bestTargetDist = dist;
                    bestTarget = predHead;
                    //clog << "ATAAAK " << snake.id << " " << bestTarget.x << " " << bestTarget.y << endl;
                }
            }
        }

        if (bestTargetDist < 5)
        {
            snake.targetPoint = bestTarget;
            snake.path = {bestTarget};
            snake.fighter = true; // Włączamy tryb wojownika dla A*
            //clog << "TACTICAL HUNT: Snake " << snake.id << " atak wroga na predykcji: " << bestTarget.x << ","   << bestTarget.y << endl;
            return true;
        }
        return false;
    }

    void assignNodeFallback(Snake& snake, const unordered_map<int, pair<int, int>>& snakeFallbacks)
    {
        if (snakeFallbacks.count(snake.id))
        {
            int fallbackId = snakeFallbacks.at(snake.id).first;
            snake.targetPoint = graphNodes[fallbackId].pos;
            snake.targetID = fallbackId;
            snake.path = {snake.targetPoint};

            if (snake.targetID < graphNodes.size())
            {
                graphNodes[snake.targetID].isAvailable = false;
            }
            //clog << "FALLBACK ASSIGN: Snake " << snake.id << " ratuje sie wezlem " << graphNodes[fallbackId].pos.x << " " << graphNodes[fallbackId].pos.y << endl;
        }
    }

    void planMovementForSnake(Snake& snake)
    {
        vector<int> expirationMap = Pathfinder::buildExpirationMap(map, snake, snakeManager.enemy_snakes);
        int sapce = Pathfinder::calculateFutureSpace(map, snake.path.front(), snake.length + 1, 0, expirationMap);
        //clog << "SAPCE " << snake.id << " " << sapce << endl;
        if (!snake.path.empty())
        {
            //clog << snake.id << " SNAKE aim: " << snake.path.front().x << "," << snake.path.front().y << endl;
        }
        vector<Snake> otherSnakes = snakeManager.enemy_snakes;
        for (auto& s : snakeManager.my_snakes)
        {
            if (s.id != snake.id)
            {
                otherSnakes.push_back(s);
            }
        }
        bool ignore = false;
        if (targetManager.targets.size() <= 1)

        {
            ignore = true;
        }
        snake.move_horizont =
            Pathfinder::findSafePathToAssignedTarget(map, snake, otherSnakes, ignore, &tunnelAnalyzer);

        if (snake.move_horizont.empty())
        {
            //clog << "SNAKE " << snake.id << " - przypisany cel jest pułapką lu2b brak drogi!" << endl;
            executeBS(snake);
            // executePanicFallbackMove(snake);
        }
        if (!snake.move_horizont.empty())

        {
            string dir = snake.move_horizont.front();
            int startDir = 0;
            if (dir == "UP")
                startDir = 0;
            else if (dir == "DOWN")
                startDir = 1;
            else if (dir == "LEFT")
                startDir = 2;
            else if (dir == "RIGHT")
                startDir = 3;
            int newX = snake.headPos.x + DX[startDir];
            int newY = snake.headPos.y + DY[startDir];
            if (map.getCell(newX, newY) != '#')
            {
                // //clog << " add: " << newX << " " << newY << map.getCell(newX, newY) << endl;
                map.setCell(newX, newY, 'P');
                snake.bodyPos.insert(snake.bodyPos.begin(), {newX, newY});
                // //clog << " add: " << newX << " " << newY << map.getCell(newX, newY) << endl;
            }
            //clog << " Reserve: " << newX << " " << newY << endl;
        }
    }

    void executePanicFallbackMove(Snake& snake)
    {
        //clog << "FATAL (BEAM): Wąż " << snake.id << " calkowicie uwięziony, brak scenariusza na przetrwanie!" << endl;
        for (int i = 0; i < 4; i++)
        {
            if (DIR_NAMES[i] == snake.last_move)
                continue;
            Point nextP = {snake.headPos.x + DX[i], snake.headPos.y + DY[i]};
            if (map.isInside(nextP.x, nextP.y) && map.getCell(nextP.x, nextP.y) != '#')
            {
                snake.move_horizont = {DIR_NAMES[i]};
                break;
            }
        }
    }

    void clearSingleTarget(const Target& target)
    {
        int id = map.getGraphCell(target.x, target.y);
        if (id != -1 && id < graphNodes.size())
        {
            graphNodes[id].isActive = false;
        }

        if (target.y > 0 && map.getCell(target.x, target.y - 1) != '#' && map.getCell(target.x, target.y - 1) != 'S')
        {
            int up_id = map.getGraphCell(target.x, target.y - 1);
            if (map.getCell(target.x, target.y - 1) != 'P')
            {
                map.setCell(target.x, target.y - 1, '.');
            }
            map.setGraphCell(target.x, target.y - 1, -1);

            if (up_id != -1 && up_id < graphNodes.size())
            {
                graphNodes[up_id].isActive = false;
            }
            //clog << "RESET UP " << target.x << " " << target.y << endl;
        }

        if (target.y + 1 < map.height && map.getCell(target.x, target.y + 1) == '#')
        {
            int szukane_x = target.x;
            int szukane_y = target.y;

            auto it = ranges::find_if(graphNodes, [szukane_x, szukane_y](const GraphNode& node)
            {
                return node.pos.x == szukane_x && node.pos.y == szukane_y && node.isActive;
            });

            if (it != graphNodes.end())
            {
                map.setCell(target.x, target.y, 'N');
                map.setGraphCell(target.x, target.y, it->id);
            }
            else
            {
                map.setCell(target.x, target.y, '.');
                map.setGraphCell(target.x, target.y, -1);
            }
        }
        else
        {
            map.setCell(target.x, target.y, '.');
            map.setGraphCell(target.x, target.y, -1);
        }
    }
};

// ==========================================
// 6. GŁÓWNA PĘTLA
// ==========================================

int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int my_id, width, height;
    if (!(cin >> my_id >> width >> height))
        return 0;

    Game game(height, width);
    for (int i = 0; i < height; i++)
    {
        string row;
        cin >> row;
        for (int j = 0; j < width; j++)
        {
            game.map.setCell(j, i, row[j]);
        }
    }
    game.findGraphNodes();

    int snakebots_per_player;
    cin >> snakebots_per_player;

    for (int i = 0; i < snakebots_per_player; i++)
    {
        int id;
        cin >> id;
        game.snakeManager.addSnake(id, true);
    }

    for (int i = 0; i < snakebots_per_player; i++)
    {
        int id;
        cin >> id;
        game.snakeManager.addSnake(id, false);
    }
    game.snakeManager.sortSnakes();

    bool firstRound = true;

    while (true)
    {
        int power_source_count;
        if (!(cin >> power_source_count))
            break;

        game.clearRound();

        //clog << "-1d" << endl;
        for (int i = 0; i < power_source_count; i++)
        {
            int x, y;
            cin >> x >> y;
            game.addTarget(x, y);
        }
        game.setSourcesCount(power_source_count);

        if (firstRound)
        {
            game.makeGraph();
            game.tunnelAnalyzer.analyzeMap(game.map);
            // for (auto &tunel : game.tunnelAnalyzer.tunnels)
            // {
            //     //clog << "tunel: " << tunel.id << endl;
            //     for (auto &cell : tunel.cells)
            //     {
            //         //clog << cell.x << " " << cell.y << endl;
            //     }
            // }
            firstRound = false;
        }

        int snakebot_count;
        cin >> snakebot_count;

        for (int i = 0; i < snakebot_count; i++)
        {
            int id;
            string body;
            cin >> id >> body;
            game.updateSnake(id, body);
        }
        // game.map.printMap();
        game.clearUnexistedTargets();
        //clog << "-0.3d" << endl;

        //clog << "0d" << endl;
        game.assignTargets();

        //clog << "1d" << endl;
        game.calculateMovements();

        //clog << "2d" << endl;
        game.executeMoves();

        //clog << "3d" << endl;
    }

    return 0;
}

