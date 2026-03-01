#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <iomanip>

using namespace std;

struct SimpleGraph
{
    // ребра
    unordered_map<char, vector<char>> edges; 

    // соседи (относительно id)
    vector<char> neighbours(char id)
    {
        return edges[id];
    }
};

struct GridLocation
{
    // координаты
    int x;
    int y;

    // перегрузка == для того, чтобы можно было
    // использовать в сравнениях всяких
    bool operator==(const GridLocation& other) const
    {
        return (x == other.x) && (y == other.y);
    }
};

// добавляем в std, 
// потому что хэш-таблицы из std
namespace std
{
    // перегружаем хэш-функцию, потому что
    // для GridLocation по-дефолту не существует
    // хэш-функции (он не умеет ее считать)
    template <> struct hash<GridLocation>
    {
        size_t operator() (const GridLocation& id) const noexcept
        {
            // хэш-функция считается как 
            // смещаем y на 16 бит (чтобы избежать коллизии)
            // и побитово складываем с x
            return hash<int>()(id.x ^ (id.y << 16));
        }
    };
}

// сетка
struct SquareGrid
{
    // стороны света, по котором можно идти
    // из одной точки
    // С, З, Ю, В
    static vector<GridLocation> DIRS;

    int width, height;

    // координаты стен
    unordered_set<GridLocation> walls;

    SquareGrid(int width, int height)
    {
        this->height = height;
        this->width = width;
    }

    bool InBounds(GridLocation id) const
    {
        // проверка на границы
        return ((0 <= id.x && 0 <= id.y) && 
               (id.x < width && id.y < height));
    }

    bool Passable(GridLocation id) const
    {
        return walls.find(id) == walls.end();
    }

    vector<GridLocation> neighbours(GridLocation id) const
    {
        vector<GridLocation> results;

        for (GridLocation dir : DIRS)
        {
            // перебираем потенциальных соседей (их всего максимум 4)
            GridLocation next{ id.x + dir.x, id.y + dir.y };

            if (InBounds(next) && Passable(next))
            {
                results.push_back(next);
            }
        }

        // проверка на "некрасивый" путь
        if ((id.x + id.y) % 2 == 0)
        {
            reverse(results.begin(), results.end());
        }

        return results;
    }
};

vector<GridLocation> SquareGrid::DIRS =
{
    // Запад, Восток, Север, Юг
    GridLocation{1, 0}, GridLocation{-1, 0},
    GridLocation{0, 1}, GridLocation{0, -1}
};

struct GridWithWeights : SquareGrid
{
    // список лесов (труднопроходимых клеток)
    unordered_set<GridLocation> forests;

    GridWithWeights(int w, int h) : SquareGrid(w, h) {}

    double cost(GridLocation from, GridLocation to) const
    {
        return forests.find(to) != forests.end() ? 5 : 1;
    }
};
// T - вершина, priority_t - ее приоритет
template<typename T, typename priority_t>
struct PriorityQueue
{
    typedef pair<priority_t, T> PQElement;

    priority_queue<PQElement, vector<PQElement>,
        greater<PQElement >> elements;

    inline bool empty() const
    {
        return elements.empty();
    }

    inline void put(T item, priority_t priority)
    {
        elements.emplace(priority, item);
    }

    T get()
    {
        T best_item = elements.top().second;
        elements.pop();
        return best_item;
    }
};

template<class Graph>
void draw_grid(const Graph& graph,
    std::unordered_map<GridLocation, double>* distances = nullptr,
    std::unordered_map<GridLocation, GridLocation>* point_to = nullptr,
    std::vector<GridLocation>* path = nullptr,
    GridLocation* start = nullptr,
    GridLocation* goal = nullptr)
{
    const int field_width = 3;
    cout << string(field_width * graph.width, '_') << '\n';
    for (int y = 0; y != graph.height; ++y) {
        for (int x = 0; x != graph.width; ++x) {
            GridLocation id{ x, y };
            if (graph.walls.find(id) != graph.walls.end()) {
                cout << "\033[43;47m" << string(field_width, '#');
                cout << "\033[0m";
            }
            else if (start && id == *start) {
                cout << "\033[43;34m A \033[0m";
            }
            else if (goal && id == *goal) {
                cout << "\033[43;34m X \033[0m";
            }
            else if (path != nullptr && find(path->begin(), path->end(), id) != path->end()) {
                cout << " @ ";
            }
            else if (point_to != nullptr && point_to->count(id)) {
                GridLocation next = (*point_to)[id];
                if (next.x == x + 1) { cout << "\033[44;104m > \033[0m"; }
                else if (next.x == x - 1) { cout << "\033[44;104m < \033[0m"; }
                else if (next.y == y + 1) { cout << "\033[44;104m v \033[0m"; }
                else if (next.y == y - 1) { cout << "\033[44;104m ^ \033[0m"; }
                else {cout << " * "; }
            }
            else if (distances != nullptr && distances->count(id)) {
                cout << ' ' << left << setw(field_width - 1) << (*distances)[id];
            }
            else {
                cout << " . ";
            }
        }
        cout << '\n';
    }
    std::cout << std::string(field_width * graph.width, '~') << '\n';
}

// отрисовка стен
void add_rect(SquareGrid& grid, int x1, int y1, int x2, int y2) {
    for (int x = x1; x < x2; ++x) {
        for (int y = y1; y < y2; ++y) {
            grid.walls.insert(GridLocation{ x, y });
        }
    }
}

SquareGrid make_diagram1() {
    SquareGrid grid(30, 15);
    add_rect(grid, 5, 5, 7, 15);
    add_rect(grid, 7, 5, 9, 6);
    add_rect(grid, 9, 5, 11, 15);
    add_rect(grid, 15, 6, 17, 7);
    add_rect(grid, 17, 5, 20, 10);
    
    return grid;
}

template<typename Graph, typename Location>
unordered_map<Location, Location> bfs(Graph graph, Location start, Location goal)
{
    queue<Location> frontier;
    frontier.push(start);

    unordered_map<Location, Location> came_from;
    came_from[start] = start;

    while (!frontier.empty())
    {
        // достаем элемент из очереди
        // кладем его в current
        Location current = frontier.front();
        frontier.pop();

        if (current == goal) { break; }

        // проходимся по соседям
        for (Location next : graph.neighbours(current))
        {
            // находится элемент в сете
            if (came_from.find(next) == came_from.end())
            {
                frontier.push(next);
                came_from[next] = current;
            }
        } 
    }
    return came_from;

}

template <typename Graph, typename Location>
void dijkstra(Graph graph, Location start, Location goal,
              unordered_map<Location, Location>& came_from,
              unordered_map<Location, double>& cost_so_far)
{
    PriorityQueue<Location, double> frontier;
    frontier.put(start, 0);

    came_from[start] = start;
    cost_so_far[start] = 0;

    while (!frontier.empty())
    {
        Location current = frontier.get();

        if (current == goal) { break; }

        for (Location next : graph.neighbours(current))
        {
            double new_cost = cost_so_far[current] + graph.cost(current, next);

            // если новая стоимость меньшей той, что записана в ячейке
            // или в ячейке вообще ничего нет
            // то запишем туда новую стоимость
            if (new_cost < cost_so_far[next] ||
                cost_so_far.find(next) == cost_so_far.end())
            {
                cost_so_far[next] = new_cost;
                came_from[next] = current;
                frontier.put(next, new_cost);
            }
        }
    }
}


int main()
{
    setlocale(LC_ALL, "Russian");

    /*SimpleGraph exampleGraph =
    {{
        {'A', {'B'}},
        {'B', {'C'}},
        {'C', {'B', 'D', 'F'}},
        {'D', {'C', 'E'}},
        {'E', {'F'}},
        {'F', {}}
    }};*/

    SquareGrid grid = make_diagram1();
    GridLocation start{15, 5};
    GridLocation goal{5, 2};
    auto parents = bfs(grid, start, goal);

    draw_grid(grid, nullptr, &parents, nullptr, &start, &goal);
}