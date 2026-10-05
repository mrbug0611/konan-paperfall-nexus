#pragma once
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include "../map/Grid.hpp"

#include "../Types.hpp"
#include "../simulation/SimWorld.hpp"

namespace nexus::engine
{
    class Renderer
    {
    public:
        static constexpr int TILE_PX = 16;

        // SFML 3: sf::Text requires a font reference at construction time
        // (no default sf::Text()), so we defer building Text objects until
        // the font has actually loaded.
        void init(sf::RenderWindow& /*window*/)
        {
            bool loaded = font_.openFromFile("../assets/monospace.ttf");
            if (!loaded)
            {
                loaded = font_.openFromFile("C:/Windows/Fonts/consola.ttf");
            }
            fontReady_ = loaded;

        }

        void render(sf::RenderWindow& win, SimWorld& sim, float camX, float camY) const
        {
            win.clear(sf::Color(10, 10, 15));

            sf::View view = win.getDefaultView();
            view.setCenter({camX, camY});
            win.setView(view);

            drawGrid(win, sim.grid());
            drawFog(win, sim.grid());
            drawEntities(win, sim);
            drawExplosionParticles(win);

            // Reset to default view for HUD
            win.setView(win.getDefaultView());
            drawHUD(win, sim);

        }

        void addExplosionAt(const GridPos& pos) {
            particles_.push_back({
                .pos = sf::Vector2f(
                    static_cast<float>(pos.x * TILE_PX) + (static_cast<float>(TILE_PX) / 2.0f),
                    static_cast<float>(pos.y * TILE_PX) + (static_cast<float>(TILE_PX) / 2.0f)
                ),
                .life = 0.8f
            });
        }

        void update(const float dt)
        {
            // std::erase_if safely handles the loop and deletion in O(N) time
            std::erase_if(particles_, [dt](Particle& p) {
                p.life -= dt;          // 1. Update the life
                return p.life <= 0.f;  // 2. Remove if dead
            });
        }






    private:

        static sf::Color terrainColor(const TerrainType t, const bool onFire) {
            if (onFire) return {255, 140, 20};
            switch (t) {
            case TerrainType::Open:        return {30, 35, 25};
            case TerrainType::Wall:        return {55, 50, 45};
            case TerrainType::Water:       return {20, 40, 80};
            case TerrainType::PaperWall:   return {200, 185, 140};
            case TerrainType::PaperBridge: return {180, 160, 110};
            case TerrainType::BurnedAsh:   return {40, 35, 30};
            default: return {20, 20, 20};
            }
        }

        static char terrainGlyph(const TerrainType t) {
            switch (t) {
            case TerrainType::Wall:        return '#';
            case TerrainType::Water:       return '~';
            case TerrainType::PaperWall:   return '|';
            case TerrainType::PaperBridge: return '=';
            case TerrainType::BurnedAsh:   return '.';
            default: return ' ';
            }
        }

        // Only called once fontReady_ is confirmed true.
        sf::Text makeText(const unsigned size) const
        {
            sf::Text t(font_, "", size);
            return t;
        }

        void drawGrid(sf::RenderWindow& win, map::Grid& grid) const
        {
            if (!fontReady_)
            {
                return ;
            }

            sf::Text glyph = makeText(TILE_PX);
            sf::RectangleShape cell(sf::Vector2f(TILE_PX - 1.F, TILE_PX - 1.F));
            for (int y = 0; y < grid.height(); y++)
            {
                for (int x = 0; x < grid.width(); x++)
                {
                    const auto& tile = grid.at(x, y);
                    const auto fog = grid.playerFog().get(x, y);

                    if (fog == map::FogLayer::State::Hidden)
                    {
                        continue;
                    }

                    const float brightness = (fog == map::FogLayer::State::Memory) ? 0.4F : 1.F;
                    sf::Color base = terrainColor(tile.terrain, tile.onFire);
                    base.r = static_cast<uint8_t>(static_cast<float>(base.r) * brightness);
                    base.g = static_cast<uint8_t>(static_cast<float>(base.g) * brightness);
                    base.b = static_cast<uint8_t>(static_cast<float>(base.b) * brightness);

                    cell.setPosition({static_cast<float>(x * TILE_PX), static_cast<float>(y * TILE_PX)});
                    cell.setFillColor(base);
                    win.draw(cell);

                    if (char g = terrainGlyph(tile.terrain); g != ' ' && fog == map::FogLayer::State::Visible)
                    {
                        glyph.setString(sf::String(std::string(1, g)));
                        glyph.setPosition({static_cast<float>(x * TILE_PX), static_cast<float>(y * TILE_PX) - 2.f});
                        glyph.setFillColor(sf::Color(
                            static_cast<uint8_t>(std::min(255, base.r + 40)),
                            static_cast<uint8_t>(std::min(255, base.g + 40)),
                            static_cast<uint8_t>(std::min(255, base.b + 40))));
                        win.draw(glyph);
                    }

                }
            }
        }

        static void drawFog(sf::RenderWindow& win, map::Grid& grid)
        {
            sf::RectangleShape fogCell(sf::Vector2f(TILE_PX, TILE_PX));
            for (int y = 0; y < grid.height(); y++)
            {
                for (int x = 0; x < grid.width(); x++)
                {
                    const auto fog = grid.playerFog().get(x, y);
                    if (fog == map::FogLayer::State::Visible)
                    {
                        continue ;
                    }

                    fogCell.setPosition({static_cast<float>(x * TILE_PX), static_cast<float>(y * TILE_PX)});
                    fogCell.setFillColor(fog == map::FogLayer::State::Hidden
                        ? sf::Color(0, 0, 0, 230) : sf::Color(0, 0, 0, 140));
                    win.draw(fogCell);
                }
            }

        }

        void drawEntities(sf::RenderWindow& win, SimWorld& sim) const
        {
            if (!fontReady_)
            {
                return ;
            }

            auto& world = sim.ecs();
            auto& grid = sim.grid();

            sf::Text glyph = makeText(TILE_PX);

            struct DrawCall {
             int layer;
             sf::Vector2f pos;
             char sym;
             sf::Color color;
             float opacity;
            };

            std::vector<DrawCall> drawList;

            for (const EntityID e : world.view<components::Renderable, components::Position>())
            {
                const auto& rend = world.getComponent<components::Renderable>(e);
                const auto& pos  = world.getComponent<components::Position>(e);

                if (!rend.visible)
                {
                    continue ;
                }

                if (const auto fog = grid.playerFog().get(pos.grid.x, pos.grid.y);
                    fog == map::FogLayer::State::Visible)
                {
                    continue ;
                }

                drawList.push_back({.layer = rend.layer, .pos = pos.world, .sym = rend.symbol, .color = rend.color,
                    .opacity = rend.opacity});
            }


            std::ranges::sort(drawList,
                              [](const DrawCall& a, const DrawCall& b)
                              {
                                  return a.layer < b.layer;
                              });

            sf::CircleShape circle(TILE_PX / 2.F - 2.F);

            for (const auto& dc : drawList)
            {
                const auto alpha = static_cast<uint8_t>(static_cast<float>(dc.opacity) * 255);
                circle.setPosition({dc.pos.x + 2.f, dc.pos.y + 2.f});
                sf::Color bg = dc.color;
                bg.a = alpha / 3;
                circle.setFillColor(bg);
                win.draw(circle);

                glyph.setString(sf::String(std::string(1, dc.sym)));
                glyph.setPosition({dc.pos.x + 2.f, dc.pos.y - 1.f});
                sf::Color c = dc.color;
                c.a = alpha;
                glyph.setFillColor(c);
                win.draw(glyph);

            }


        }

        void drawExplosionParticles(sf::RenderWindow& win) const
        {
            sf::CircleShape c;

            for (const auto& [pos, life] : particles_)
            {
                const float t = life / 0.8F;
                const float r = (1.F - t) * TILE_PX * 3.F;
                c.setRadius(r);
                c.setPosition({pos.x - r, pos.y - r});
                const auto alpha = static_cast<uint8_t>(t * 200.f);
                c.setFillColor(sf::Color(255, 160, 30, alpha));
                win.draw(c);
            }
        }

        void drawHUD(sf::RenderWindow& win, SimWorld& sim) const
        {
            auto& world = sim.ecs();

            if (!world.isAlive(sim.commanderID()))
            {
                return ;
            }

            auto& cmd = world.getComponent<components::Commander>(sim.commanderID());
            auto& [current, max]  = world.getComponent<components::Health>(sim.commanderID());

            const int sw = static_cast<int>(win.getSize().x);
            const int sh = static_cast<int>(win.getSize().y);

            sf::RectangleShape bar(sf::Vector2f(static_cast<float>(sw), 60.f));
            bar.setPosition({0.f, static_cast<float>(sh - 60)});
            bar.setFillColor(sf::Color(10, 10, 20, 200));
            win.draw(bar);

            if (!fontReady_)
            {
                return ;
            }

            sf::Text txt = makeText(14);

            txt.setString("HP: " + std::to_string(current) + "/" + std::to_string(max));
            txt.setPosition({10.f, static_cast<float>(sh - 55)});
            txt.setFillColor(sf::Color(100, 255, 100));
            win.draw(txt);

            std::string chargeStr = "Paper: ";

            for (int i = 0; i < cmd.maxPaperCharges; i++)
            {
                chargeStr += (i < cmd.maxPaperCharges) ? "[*]" : "[ ]";
            }
            txt.setString(chargeStr);
            txt.setPosition({10.f, static_cast<float>(sh - 35)});
            txt.setFillColor(sf::Color(220, 200, 150));
            win.draw(txt);

            const char* typeNames[] = {"Crane", "Barrier", "Clone", "Explosive", "Bridge", "Ramp"};
            txt.setString(std::string("Selected: ") + typeNames[static_cast<int>(cmd.selectedType)]
                + "  [1-5] Change  [LMB] Deploy  [RMB] Move  [F5] Save");
            txt.setPosition({10.f, static_cast<float>(sh - 15)});
            txt.setFillColor(sf::Color(160, 160, 200));
            win.draw(txt);

            txt.setString("Enemies: " + std::to_string(sim.enemiesLeft()));
            txt.setPosition({static_cast<float>(sw - 150), static_cast<float>(sh - 55)});
            txt.setFillColor(sf::Color(255, 100, 100));
            win.draw(txt);

            txt.setString("Frame: " + std::to_string(sim.frameNum()));
            txt.setPosition({static_cast<float>(sw - 150), static_cast<float>(sh - 35)});
            txt.setFillColor(sf::Color(80, 80, 100));
            win.draw(txt);

        }


        sf::Font font_;
        bool fontReady_{false};

        struct Particle
        {
            sf::Vector2f pos;
            float life{};
        };

        std::vector<Particle> particles_;


    };
}
