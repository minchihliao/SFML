#include <SFML/Graphics/RenderStates.hpp>

#include <catch2/catch_test_macros.hpp>

#include <GraphicsUtil.hpp>
#include <type_traits>

TEST_CASE("[Graphics] sf::RenderStates")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(std::is_aggregate_v<sf::RenderStates>);
        STATIC_CHECK(std::is_copy_constructible_v<sf::RenderStates>);
        STATIC_CHECK(std::is_copy_assignable_v<sf::RenderStates>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<sf::RenderStates>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::RenderStates>);
    }

    SECTION("Construction")
    {
        const sf::RenderStates renderStates;
        CHECK(renderStates.blendMode == sf::BlendMode());
        CHECK(renderStates.transform == sf::Transform());
        CHECK(renderStates.texture == nullptr);
        CHECK(renderStates.shader == nullptr);
    }

    SECTION("Default constant")
    {
        CHECK(sf::RenderStates::Default.blendMode == sf::BlendMode());
        CHECK(sf::RenderStates::Default.transform == sf::Transform());
        CHECK(sf::RenderStates::Default.texture == nullptr);
        CHECK(sf::RenderStates::Default.shader == nullptr);
    }
}
