#pragma once
#include <functional>
#include "../Types.hpp"
#include "Renderer.hpp"

namespace nexus::engine
{
    class InputHandler
    {

    public:
        // callable object that has no parameters and can return void
        using SaveCallBack = std::function<void()>;

        void setOnSave(SaveCallBack cb)
        {
            onSave_ = std::move(cb);

        }

        // SFML 3: events arrive as std::optional<sf::Event> from pollEvent(),
        // and sf::Event is a variant-like type queried with is<T>()/getIf<T>()
        // instead of the old `.type` + union-of-structs approach.

        void processEvent(const sf::Event& ev, sf::RenderWindow& win,
                  SimWorld& sim, Renderer& rend,
                  float& camX, float& camY)
        {
            if (ev.is<sf::Event::Closed>())
            {
                win.close();
                return;
            }

            if (const auto* key = ev.getIf<sf::Event::KeyPressed>())
            {
                handleKey(key -> code, sim, win, camX, camY);
            }


            if (const auto* mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left)
                    handleLeftClick(mb->position.x, mb->position.y, win, sim, camX, camY);
                if (mb->button == sf::Mouse::Button::Right)
                    handleRightClick(mb->position.x, mb->position.y, win, sim, camX, camY);
            }

            if (const auto* wheel = ev.getIf<sf::Event::MouseWheelScrolled>())
            {
                zoom_ -= wheel-> delta * 0.1F;
                zoom_ = std::clamp(zoom_, 0.5F, 3.F);
            }
        }

        static void update(const float dt, float& camX, float& camY)
        {
            constexpr float spd = 200.F;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  camX -= spd * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) camX += spd * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))    camY -= spd * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  camY += spd * dt;
        }

        [[nodiscard]] float zoom() const noexcept
        {
            return zoom_;
        }

    private:

          static GridPos screenToGrid(const int sx, const int sy, const float camX, const float camY,
  const sf::RenderWindow& win)
          {
              const float hw = static_cast<float>(win.getSize().x) / 2.F;
              const float hh = static_cast<float>(win.getSize().y) / 2.F;
              const float wx = camX - hw + static_cast<float>(sx);
              const float wy = camY - hh + static_cast<float>(sy);

              return {.x = static_cast<int>(wx / Renderer::TILE_PX),
        .y = static_cast<int>(wy / Renderer::TILE_PX)};
          }

        void handleKey(const sf::Keyboard::Key key, SimWorld& sim,
               sf::RenderWindow& win, float& /*camX*/, float& /*camY*/) const
        {
              if (!sim.ecs().isAlive(sim.commanderID()))
              {
                  return;
              }

              auto& cmd = sim.ecs().getComponent<components::Commander>(sim.commanderID());

              switch (key) {
              case sf::Keyboard::Key::Num1: cmd.selectedType = PaperType::Crane;        break;
              case sf::Keyboard::Key::Num2: cmd.selectedType = PaperType::Barrier;      break;
              case sf::Keyboard::Key::Num3: cmd.selectedType = PaperType::CloneSlip;    break;
              case sf::Keyboard::Key::Num4: cmd.selectedType = PaperType::ExplosiveTag; break;
              case sf::Keyboard::Key::Num5: cmd.selectedType = PaperType::Bridge;       break;
              case sf::Keyboard::Key::F5:
                  if (onSave_)
                  {
                      onSave_();
                      break;
                  }
              case sf::Keyboard::Key::Escape:
                  win.close();
                  break;
              default: break;
              }
          }

        static void handleLeftClick(const int sx, const int sy, const sf::RenderWindow& win,
                      SimWorld& sim, const float camX, const float camY) {
              if (!sim.ecs().isAlive(sim.commanderID())) return;
              const auto& cmd = sim.ecs().getComponent<components::Commander>(sim.commanderID());
              const GridPos pos = screenToGrid(sx, sy, camX, camY, win);
                // ignore hud clicks
              if (sy >= static_cast<int>(win.getSize().y - 60))
              {
                  return ;
              }

              sim.deployPaper(pos, cmd.selectedType);
          }

        static void handleRightClick(const int sx, const int sy, const sf::RenderWindow& win,
                       SimWorld& sim, const float camX, const float camY) {
              const GridPos pos = screenToGrid(sx, sy, camX, camY, win);
              if (sy >= static_cast<int>(win.getSize().y) - 60) return;
              sim.moveCommanderTo(pos);
          }




        SaveCallBack onSave_;
        float zoom_{1.F};






    };
}
