#pragma once
#include <cstdint>
#include <random>


#include "Grid.hpp"


namespace nexus::map {
   struct MapGenParams {
       int width{60};
       int height{40};
       uint32_t seed{42};
       float wallDensity{0.12F};
       float waterDensity{0.06F};
       int numRooms{8};
       int corridorWidth{2};
       bool generateElevation{true};


   };


   class MapGenerator {
   public:
       static Grid generate(const MapGenParams& p) {
           Grid grid(p.width, p.height);
           std::mt19937 rng(p.seed); // pseudo random rng alg


           // fill with open terrain
           grid.fill(TerrainType::Open);


           // border walls
           for (int x = 0; x<p.width; x++) {
               grid.setTerrain({x, 0}, TerrainType::Wall);
               grid.setTerrain({x, p.height - 1}, TerrainType::Wall);
           }


           for (int y=0; y<p.height; ++y) {
               grid.setTerrain({0,y}, TerrainType::Wall);
               grid.setTerrain({p.width-1,y}, TerrainType::Wall);
           }


           // rooms connected by corridors
           std::vector<std::pair<GridPos, GridPos>> rooms; // top left, bottom right
           std::uniform_int_distribution<int> roomW(4, 10); // random int within closed interval
           std::uniform_int_distribution<int> roomH(3, 7);
           std::uniform_int_distribution<int> posX(2, p.width-12);
           std::uniform_int_distribution<int> posY(2, p.height - 9);


           for (int i = 0; i < p.numRooms; i++) {
               int rw = roomW(rng);
               int rh = roomH(rng);
               int rx = posX(rng);
               int ry = posY(rng);
               rx = std::min(rx, p.width-rw-2);
               ry = std::min(ry, p.height-rh-2);
               rooms.push_back({{rx,ry},{rx+rw,ry+rh}});


               for (int y=ry; y<=ry+rh; y++) {
                   for (int x=rx; x<=rx+rw; x++) {
                       grid.setTerrain({x, y}, TerrainType::Open);
                   }
               }




           }


           // connect rooms with corridors
           for (size_t i = 1; i<rooms.size(); i++) {
               GridPos a = roomCenter(rooms[i-1]);
               GridPos b = roomCenter(rooms[i]);
               carveCorridor(grid, a, b, p.corridorWidth);
           }


           //random walls
           std::uniform_real_distribution<float> rnd(0.F, 1.F);


           for (int y=2; y<p.height-2; y++) {
               for (int x = 2; x<p.width-2; x++) {
                   if (grid.at(x, y).terrain != TerrainType::Open) {
                       continue;
                   }


                   if (rnd(rng) < p.wallDensity) {
                       grid.setTerrain({x,y}, TerrainType::Wall);
                   }
               }
           }

           // Water Patches
           const int numPools = static_cast<int>(static_cast<float>(p.width) *
               static_cast<float>(p.height) * p.waterDensity / 9.0F);
           std::uniform_int_distribution<int> px(3, p.width-4);
           std::uniform_int_distribution<int> py(3, p.height-4);
           std::uniform_int_distribution<int> poolR(1, 3);

           for (int i = 0; i < numPools; i++)
           {
               const int cx = px(rng);
               const int cy = py(rng);
               const int r = poolR(rng);

               for (int dy=-r; dy<=r; dy++)
               {
                   for (int dx=-r; dx<=r; dx++)
                   {
                       if (dx*dx + dy*dy < r*r && grid.at(cx+dx, cy+dy).terrain == TerrainType::Open)
                       {
                            grid.setTerrain({.x = cx,.y = cy+dy}, TerrainType::Water);
                       }
                   }
               }
           }

           //Elevation Hints (Ramps)
           if (p.generateElevation)
           {
               for (int i = 0; i < 4; i++)
               {
                   GridPos rp{px(rng),py(rng)};

                   if (grid.at(rp).isPassable())
                   {
                       grid.at(rp).elevation = 1;
                       grid.at(rp).traversalCost = 1.5F;

                   }
               }
           }

           return grid;



       }


   private:
       static GridPos roomCenter(const std::pair<GridPos, GridPos>& r) {
           return {.x = (r.first.x + r.second.x / 2), .y = (r.first.y + r.second.y / 2)};
       }


       static void carveCorridor(Grid& g, const GridPos a, const GridPos b, const int w) {
           // horizontal than vertical L-shaped corridor
           const int sx = (a.x < b.x) ? 1 : -1;
           const int sy = (a.y < b.y) ? 1 : -1;


           for (int x=a.x; x!=b.x; x+=sx) {
               for (int dw=-w/2; dw<=w/2; dw += 1) {
                   if (g.inBounds(x, a.y+dw)) {
                       g.setTerrain({.x = x, .y = a.y+dw}, TerrainType::Open);
                   }
               }
           }


           for (int y=a.y; y!=b.y; y+=sy) {
               for (int dw=-w/2; dw<=w/2; ++dw) {
                   if (g.inBounds(b.x+dw, y)) {
                       g.setTerrain({.x = b.x+dw, .y = y}, TerrainType::Open);
                   }
               }
           }
       }
   };
};

