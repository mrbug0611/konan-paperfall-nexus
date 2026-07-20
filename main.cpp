#include <SFML/Graphics.hpp>
#include <iostream>

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.

auto main() -> int {
    // TIP Press <shortcut actionId="RenameElement"/> when your caret is at the <b>lang</b> variable name to see how CLion can help you rename it.

    sf::RenderWindow window(sf::VideoMode({1280U, 720u}), "Paperfall Nexus");
    window.setFramerateLimit(60);

    while (window.isOpen()) {
        while (const std::optional<sf::Event> ev = window.pollEvent()) {
            if (ev -> is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* key = ev -> getIf<sf::Event::KeyPressed>()) {
                if (key -> code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }

        window.clear(sf::Color(10, 10, 15));
        window.display();

    }


    return 0;
    // TIP See CLion help at <a href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>. Also, you can try interactive lessons for CLion by selecting 'Help | Learn IDE Features' from the main menu.
}