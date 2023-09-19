#include <SFML/Window/Mouse.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("[Window] sf::Mouse")
{
    SECTION("isButtonPressed()")
    {
        CHECK(!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left));
        CHECK(!sf::Mouse::isButtonPressed(sf::Mouse::Button::Right));
        CHECK(!sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle));
        CHECK(!sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra1));
        CHECK(!sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra2));
    }
}
