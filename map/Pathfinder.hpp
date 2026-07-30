#pragma once
#include <algorithm>
#include <cmath>
#include <queue>

#include "../Types.hpp"
#include "../map/Grid.hpp"

namespace nexus::pathfinding {
    struct Node {
        GridPos pos;
        float g{0.F};
        float f{0.F};
        auto operator>(const Node& o) const noexcept -> bool {
            return f > o.f;
        }
    };

    //copies function code directly
    inline float heuristic(const GridPos& a, const GridPos& b) noexcept {
        //octile distance
        const float dx = std::abs(static_cast<float>(a.x - b.x));
        const float dy = std::abs(static_cast<float>(a.y - b.y));
        const double sqrt2 = std::sqrt(2.0);
        return (dx+dy) + (static_cast<float>(sqrt2) - 2.F) * std::min(dx,dy);
    }

    // ─── A* on Grid ───────────────────────────────────────────────────────────
    inline std::vector<GridPos> findPath(
        const map::Grid& grid, const GridPos& start, const GridPos& goal, const bool diagonal = false, const int maxNodes = 2000)
    {
        if (!grid.inBounds(start) || !grid.inBounds(goal)) {
            return {};
        }

        if (start == goal) {
            return {start};
        }
        if (!grid.at(goal).isPassable()) {
            return {};
        }

        std::priority_queue<Node, std::vector<Node>, std::greater<>> open; // stores in descending order
        std::unordered_map<GridPos, GridPos, GridPosHash> cameFrom;
        std::unordered_map<GridPos, float, GridPosHash> gScore;

        gScore[start] = 0.F;
        open.push({start, 0.f, heuristic(start, goal)});

        int visited = 0;
        while (!open.empty() && visited < maxNodes) {
            Node cur = open.top();
            open.pop();
            visited++;

            if (cur.pos == goal) {
                //reconstruct
                std::vector<GridPos> path;
                GridPos p = goal;

                while (p != start) {
                    path.push_back(p);
                    p = cameFrom[p];
                }

                path.push_back(start);
                std::ranges::reverse(path);
                return path; 
            }

            for (const GridPos& nb : grid.neighbors(cur.pos, diagonal)) {
                // Wrap cur.pos in FromPos{} and nb in ToPos{}
                const float tentG = gScore[cur.pos] + grid.cost(map::Grid::FromPos{cur.pos}, map::Grid::ToPos{nb});


                if (auto it = gScore.find(nb); it == gScore.end() || tentG < it->second) {
                    gScore[nb] = tentG;
                    cameFrom[nb] = cur.pos;
                    open.push({.pos = nb, .g = tentG, .f = tentG + heuristic(nb, goal)});

                }
            }


        }


        return {};
    }

    // ─── Dijkstra flood (for vision range, influence maps) ────────────────────
    inline std::unordered_map<GridPos, float, GridPosHash> dijkstraFlood(
            const map::Grid& grid,
            const GridPos& origin,
            const float maxCost,
            const bool diagonal = false
        ) {

        std::unordered_map<GridPos, float, GridPosHash> dist;
        // couples 2 values of different data types into a single unit
        using PQPair = std::pair<float, GridPos>;

        struct PQCmp {
            bool operator()(const PQPair& a, const PQPair& b) const {return a.first > b.first;}

        };

        std::priority_queue<PQPair, std::vector<PQPair>, PQCmp> pq;
        dist[origin] = 0.F;
        pq.emplace(0.F, origin);

        while (!pq.empty()) {
            auto [d, pos] = pq.top();
            pq.pop();

            if (d > dist[pos]) {
                continue;
            }

            if (d > maxCost) {
                break;
            }

            for (const GridPos& nb : grid.neighbors(pos, diagonal)) {
                if (float nd = d +
                    grid.cost(static_cast<map::Grid::FromPos>(pos), map::Grid::ToPos(nb)); nd <= maxCost) {
                    if (auto it = dist.find(nb); it == dist.end() || nd < it->second) {
                        dist[nb] = nd;
                        pq.emplace(nd, nb);
                    }
                }
            }
        }


        return dist;
    }

    // ─── Line of sight check (Bresenham) ─────────────────────────────────────
    inline bool hasLineOfSight(
                const map::Grid& grid,
                const GridPos& from,
                const GridPos& to
    ) {
        int x0=from.x;
        int y0=from.y;
        int x1=to.x;
        int y1=to.y;
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            if (x0 == x1 && y0 == y1) {
                return true;
            }

            if (grid.inBounds(x0, y0) && grid.at(x0, y0).blocksVision) {
                if (x0 == from.x && y0 == from.y) {

                }

                else {
                    return false;
                }
            }

            int e2 = 2 * err;

            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }

            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }

    }

}; // namespace nexus::pathfinding