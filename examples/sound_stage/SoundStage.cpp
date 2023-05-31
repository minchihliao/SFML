
////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics.hpp>

#include <SFML/Audio.hpp>

#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <random>
#include <vector>

#include <cmath>

namespace
{
constexpr auto windowWidth  = 1600u;
constexpr auto windowHeight = 900u;
constexpr auto markerRadius = 40.f;
constexpr auto pi           = 3.14159265359f;

std::filesystem::path resourcesDir()
{
#ifdef SFML_SYSTEM_IOS
    return "";
#else
    return "resources";
#endif
}

void errorMessage(const std::string& message)
{
    if (ImGui::BeginPopupModal("Error"))
    {
        ImGui::Text("%s", message.c_str());
        ImGui::EndPopup();
    }
}

class Marker
{
public:
    Marker(const sf::SoundSource* soundSource,
           const sf::Vector2f*    listenerPosition,
           const sf::Font&        font,
           const sf::Color&       color,
           const sf::String&      text) :
    m_soundSource(soundSource),
    m_listenerPosition(listenerPosition),
    m_label(font, text, static_cast<unsigned int>(markerRadius))
    {
        m_coneCenter.setFillColor(sf::Color::Magenta);
        m_coneInner.setFillColor(sf::Color(255, 0, 127));
        m_coneOuter.setFillColor(sf::Color::Red);
        m_marker.setFillColor(color);
        m_label.setFillColor(sf::Color::Black);
        m_label.setStyle(sf::Text::Bold);
    }

    void draw(sf::RenderTarget& target)
    {
        const auto listenerOffset = (m_listenerPosition && m_soundSource && m_soundSource->isRelativeToListener())
                                        ? *m_listenerPosition
                                        : sf::Vector2f();
        const auto labelBounds    = m_label.getLocalBounds().getSize().cwiseMul({0.5f, 0.75f});

        target.draw(m_coneCenter);

        sf::Angle innerAngle;
        sf::Angle outerAngle;

        // Check if we are a SoundSource or the Listener
        if (m_soundSource)
        {
            innerAngle = m_soundSource->getCone().innerAngle;
            outerAngle = m_soundSource->getCone().outerAngle;
        }
        else
        {
            innerAngle = sf::Listener::getCone().innerAngle;
            outerAngle = sf::Listener::getCone().outerAngle;
        }

        if (innerAngle != sf::degrees(360))
        {
            m_coneInner.setRotation(m_coneCenter.getRotation() - innerAngle / 2);
            target.draw(m_coneInner);
            m_coneInner.setRotation(m_coneCenter.getRotation() + innerAngle / 2);
            target.draw(m_coneInner);
        }

        if (outerAngle != sf::degrees(360))
        {
            m_coneOuter.setRotation(m_coneCenter.getRotation() - outerAngle / 2);
            target.draw(m_coneOuter);
            m_coneOuter.setRotation(m_coneCenter.getRotation() + outerAngle / 2);
            target.draw(m_coneOuter);
        }

        target.draw(m_marker, sf::Transform().translate(-sf::Vector2f(markerRadius, markerRadius) + listenerOffset));
        target.draw(m_label, sf::Transform().translate(-labelBounds + listenerOffset));
    }

    void setPosition(const sf::Vector2f& position)
    {
        m_coneCenter.setPosition(position);
        m_coneInner.setPosition(position);
        m_coneOuter.setPosition(position);
        m_marker.setPosition(position);
        m_label.setPosition(position);
    }

    sf::Angle getRotation() const
    {
        return m_coneCenter.getRotation();
    }

    void setRotation(const sf::Angle& angle)
    {
        m_coneCenter.setRotation(angle);
    }

private:
    const sf::SoundSource* m_soundSource;
    const sf::Vector2f*    m_listenerPosition;
    sf::RectangleShape     m_coneCenter{{120.f, 1.f}};
    sf::RectangleShape     m_coneInner{{100.f, 1.f}};
    sf::RectangleShape     m_coneOuter{{100.f, 1.f}};
    sf::CircleShape        m_marker{markerRadius};
    sf::Text               m_label;
};

class Object
{
public:
    Object(const sf::Font&     font,
           sf::SoundSource&    soundSource,
           const sf::Vector2f& listenerPosition,
           const sf::Color&    markerColor,
           const std::string&  markerPrefix) :
    m_soundSource(soundSource),
    m_marker(&m_soundSource, &listenerPosition, font, markerColor, markerPrefix + getIndex())
    {
        m_marker.setPosition(m_position);
        m_marker.setRotation(m_rotation);
    }

    virtual ~Object() = default;

    virtual void draw(sf::RenderTarget& target) = 0;

    void drawMarker(sf::RenderTarget& target)
    {
        m_marker.draw(target);
    }

    void drawPositionRotationControls()
    {
        ImGui::DragFloat2("Position", &m_position.x);
        if (auto radians = m_rotation.asRadians(); ImGui::DragFloat("Rotation", &radians, 0.01f, -2 * pi, 2 * pi))
            m_rotation = sf::radians(radians);
    }

    void drawConeControls()
    {
        auto cone = m_soundSource.getCone();

        if (auto innerAngle = cone.innerAngle.asRadians(); ImGui::DragFloat("Cone Inner", &innerAngle, 0.01f, 0.f, 2 * pi))
            cone.innerAngle = std::clamp(sf::radians(innerAngle), sf::degrees(0), cone.outerAngle);

        if (auto outerAngle = cone.outerAngle.asRadians(); ImGui::DragFloat("Cone Outer", &outerAngle, 0.01f, 0.f, 2 * pi))
            cone.outerAngle = std::clamp(sf::radians(outerAngle), cone.innerAngle, sf::degrees(360));

        if (auto outerGain = cone.outerGain; ImGui::DragFloat("Outer Gain", &outerGain, 0.001f, 0.f, 1.f))
            cone.outerGain = outerGain;

        m_soundSource.setCone(cone);
    }

    void drawSoundSourceControls()
    {
        if (auto relative = m_soundSource.isRelativeToListener(); ImGui::Checkbox("Relative to Listener", &relative))
            m_soundSource.setRelativeToListener(relative);

        if (auto pitch = m_soundSource.getPitch(); ImGui::DragFloat("Pitch", &pitch, 0.01f, 0.f, 10.f))
            m_soundSource.setPitch(pitch);

        if (auto volume = m_soundSource.getVolume(); ImGui::DragFloat("Volume", &volume, 1.f, 0.f, 100.f))
            m_soundSource.setVolume(volume);

        if (auto attenuation = m_soundSource.getAttenuation();
            ImGui::DragFloat("Attenuation", &attenuation, 0.01f, 0.f, 10.f))
            m_soundSource.setAttenuation(attenuation);

        if (auto minDistance = m_soundSource.getMinDistance(); ImGui::DragFloat("Min. Distance", &minDistance))
            m_soundSource.setMinDistance(minDistance);
    }

    void drawPlayControls()
    {
        if (ImGui::Button("Play"))
            m_soundSource.play();
        ImGui::SameLine();
        if (ImGui::Button("Pause"))
            m_soundSource.pause();
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
            m_soundSource.stop();
    }

    void update()
    {
        m_marker.setPosition(m_position);
        m_marker.setRotation(m_rotation);
        m_soundSource.setPosition({m_position.x, 0, m_position.y});
        m_soundSource.setDirection({std::cos(m_rotation.asRadians()), 0, std::sin(m_rotation.asRadians())});
    }

    const std::string& getIndex() const
    {
        return m_index;
    }

private:
    std::string      m_index{[]()
                        {
                            static std::size_t nextIndex = 0;
                            return std::to_string(nextIndex++);
                        }()};
    sf::SoundSource& m_soundSource;
    Marker           m_marker;
    sf::Vector2f     m_position{[]()
                            {
                                static std::random_device                    rd;
                                static std::mt19937                          rng(rd());
                                static std::uniform_real_distribution<float> xDistribution(-100.f, 100.f);
                                static std::uniform_real_distribution<float> yDistribution(-100.f, 100.f);
                                return sf::Vector2f(xDistribution(rng), yDistribution(rng));
                            }()};
    sf::Angle        m_rotation{sf::degrees(270)};
};

class Sound : public Object
{
public:
    Sound(const sf::Font& font, const sf::Vector2f& listenerPosition) :
    Object(font, m_sound, listenerPosition, sf::Color::Yellow, "S")
    {
        m_sound.setAttenuation(0.01f);
    }

    void draw(sf::RenderTarget& target) override
    {
        if (m_sound.getBuffer().getDuration() != sf::Time::Zero)
            drawMarker(target);

        ImGui::SetNextWindowSize({0.f, 0.f});
        ImGui::Begin(("Sound " + getIndex()).c_str());

        ImGui::InputText("File Path", &m_path);
        ImGui::SameLine();
        if (ImGui::Button("Load"))
        {
            m_sound.stop();

            if (!m_buffer.loadFromFile(m_path))
                errorMessage("Failed to load " + m_path);
        }

        if (m_sound.getBuffer().getDuration() != sf::Time::Zero)
        {
            const auto duration = m_sound.getBuffer().getDuration().asSeconds();
            if (auto offset = m_sound.getPlayingOffset().asSeconds();
                ImGui::SliderFloat("Playing Offset", &offset, 0.f, duration))
                m_sound.setPlayingOffset(sf::seconds(offset));

            drawPositionRotationControls();
            drawConeControls();
            drawSoundSourceControls();

            if (auto loop = m_sound.getLoop(); ImGui::Checkbox("Loop", &loop))
                m_sound.setLoop(loop);

            drawPlayControls();
        }

        ImGui::End();
    }

private:
    std::string     m_path{(resourcesDir() / "ding.flac").string()};
    sf::SoundBuffer m_buffer;
    sf::Sound       m_sound{m_buffer};
};

class Music : public Object
{
public:
    Music(const sf::Font& font, const sf::Vector2f& listenerPosition) :
    Object(font, m_music, listenerPosition, sf::Color::Cyan, "M")
    {
        m_music.setAttenuation(0.01f);
    }

    void draw(sf::RenderTarget& target) override
    {
        if (m_music.getDuration() != sf::Time::Zero)
            drawMarker(target);

        ImGui::SetNextWindowSize({0.f, 0.f});
        ImGui::Begin(("Music " + getIndex()).c_str());

        ImGui::InputText("File Path", &m_path);
        ImGui::SameLine();
        if (ImGui::Button("Load"))
        {
            m_music.stop();

            if (!m_music.openFromFile(m_path))
                errorMessage("Failed to load " + m_path);
        }

        if (m_music.getDuration() != sf::Time::Zero)
        {
            if (auto offset = m_music.getPlayingOffset().asSeconds();
                ImGui::SliderFloat("Playing Offset", &offset, 0.f, m_music.getDuration().asSeconds()))
                m_music.setPlayingOffset(sf::seconds(offset));

            drawPositionRotationControls();
            drawConeControls();
            drawSoundSourceControls();

            if (auto loop = m_music.getLoop(); ImGui::Checkbox("Loop", &loop))
                m_music.setLoop(loop);

            drawPlayControls();
        }

        ImGui::End();
    }

private:
    std::string m_path{(resourcesDir() / "doodle_pop.ogg").string()};
    sf::Music   m_music;
};

class Tone : public sf::SoundStream, public Object
{
public:
    Tone(const sf::Font& font, const sf::Vector2f& listenerPosition) :
    Object(font, *this, listenerPosition, sf::Color::Green, "T")
    {
        sf::SoundStream::initialize(1, sampleRate, {sf::SoundChannel::Mono});
        setAttenuation(0.01f);
    }

    void draw(sf::RenderTarget& target) override
    {
        drawMarker(target);

        ImGui::SetNextWindowSize({0.f, 0.f});
        ImGui::Begin(("Tone " + getIndex()).c_str());

        if (ImGui::RadioButton("Sine", m_type == Type::Sine))
            m_type = Type::Sine;
        ImGui::SameLine();
        if (ImGui::RadioButton("Square", m_type == Type::Square))
            m_type = Type::Square;
        ImGui::SameLine();
        if (ImGui::RadioButton("Triangle", m_type == Type::Triangle))
            m_type = Type::Triangle;
        ImGui::SameLine();
        if (ImGui::RadioButton("Sawtooth", m_type == Type::Sawtooth))
            m_type = Type::Sawtooth;

        ImGui::DragFloat("Amplitude", &m_amplitude, 0.01f, 0.f, 1.f);
        ImGui::DragFloat("Frequency", &m_frequency, 1.f, 0.f, 1000.f);

        ImGui::PlotLines("Wave",
                         [](void* data, int sampleIndex)
                         {
                             const auto& samples = *static_cast<std::vector<std::int16_t>*>(data);
                             return static_cast<float>(samples[static_cast<std::size_t>(sampleIndex)]);
                         },
                         &m_sampleBuffer,
                         static_cast<int>(m_sampleBuffer.size()),
                         0,
                         nullptr,
                         std::numeric_limits<std::int16_t>::min() * m_amplitude,
                         std::numeric_limits<std::int16_t>::max() * m_amplitude,
                         {0.f, 100.f});

        drawPositionRotationControls();
        drawConeControls();
        drawSoundSourceControls();
        drawPlayControls();

        ImGui::End();
    }

private:
    bool onGetData(sf::SoundStream::Chunk& chunk) override
    {
        const auto period = 1.f / m_frequency;

        for (auto i = 0u; i < chunkSize; ++i)
        {
            auto value = 0.f;

            switch (m_type)
            {
                case Type::Sine:
                {
                    value = m_amplitude * std::sin(2 * pi * m_frequency * m_time);
                    break;
                }
                case Type::Square:
                {
                    value = m_amplitude *
                            (2 * (2 * std::floor(m_frequency * m_time) - std::floor(2 * m_frequency * m_time)) + 1);
                    break;
                }
                case Type::Triangle:
                {
                    value = 4 * m_amplitude / period *
                                std::abs(std::fmod(((std::fmod((m_time - period / 4), period)) + period), period) -
                                         period / 2) -
                            m_amplitude;
                    break;
                }
                case Type::Sawtooth:
                {
                    value = m_amplitude * 2 * (m_time / period - std::floor(0.5f + m_time / period));
                    break;
                }
            }

            m_sampleBuffer[i] = static_cast<std::int16_t>(std::lround(value * std::numeric_limits<std::int16_t>::max()));
            m_time += timePerSample;
        }

        chunk.sampleCount = chunkSize;
        chunk.samples     = m_sampleBuffer.data();

        return true;
    }

    void onSeek(sf::Time) override
    {
        // It doesn't make sense to seek in a tone generator
    }

    enum class Type
    {
        Sine,
        Square,
        Triangle,
        Sawtooth
    };

    static constexpr unsigned int sampleRate = 44100;
    static constexpr std::size_t  chunkSize  = sampleRate / 100;
    static constexpr float        timePerSample{1.f / static_cast<float>(sampleRate)};

    std::vector<std::int16_t> m_sampleBuffer = std::vector<std::int16_t>(chunkSize, 0);
    Type                      m_type{Type::Triangle};
    float                     m_amplitude{0.05f};
    float                     m_frequency{220};
    float                     m_time{};
};
} // namespace


////////////////////////////////////////////////////////////
/// Entry point of application
///
/// \return Application exit code
///
////////////////////////////////////////////////////////////
int main()
{
    sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), "SFML Sound Stage");
    window.setFramerateLimit(60);
    window.setView(sf::View(sf::Vector2f(0, 0), static_cast<sf::Vector2f>(window.getSize())));

    if (!ImGui::SFML::Init(window))
        return EXIT_FAILURE;

    sf::Font font;
    if (!font.loadFromFile(resourcesDir() / "tuffy.ttf"))
        return EXIT_FAILURE;

    sf::Vector2f listenerPosition;

    Marker marker(nullptr, nullptr, font, sf::Color::White, "L");
    marker.setRotation(sf::degrees(270));

    std::vector<std::unique_ptr<Object>> objects;

    sf::Clock deltaClock;
    while (window.isOpen())
    {
        for (sf::Event event; window.pollEvent(event);)
        {
            ImGui::SFML::ProcessEvent(window, event);

            switch (event.type)
            {
                case sf::Event::Resized:
                    window.setView(sf::View(sf::Vector2f(0, 0), static_cast<sf::Vector2f>(window.getSize())));
                    break;
                case sf::Event::Closed:
                    window.close();
                    break;
                default:
                    break;
            }
        }

        for (auto& object : objects)
            object->update();

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::SetNextWindowSize({0.f, 0.f});
        ImGui::Begin("Control");

        if (ImGui::DragFloat2("Listener Position", &listenerPosition.x))
        {
            marker.setPosition(listenerPosition);
            sf::Listener::setPosition({listenerPosition.x, 0, listenerPosition.y});
        }

        if (auto rotation = marker.getRotation().asRadians();
            ImGui::DragFloat("Listener Rotation", &rotation, 0.01f, -2 * pi, 2 * pi))
        {
            marker.setRotation(sf::radians(rotation));
            sf::Listener::setDirection({std::cos(rotation), 0, std::sin(rotation)});
        }

        auto cone = sf::Listener::getCone();

        if (auto innerAngle = cone.innerAngle.asRadians();
            ImGui::DragFloat("Listener Cone Inner", &innerAngle, 0.01f, 0.f, 2 * pi))
            cone.innerAngle = std::clamp(sf::radians(innerAngle), sf::degrees(0), cone.outerAngle);

        if (auto outerAngle = cone.outerAngle.asRadians();
            ImGui::DragFloat("Listener Cone Outer", &outerAngle, 0.01f, 0.f, 2 * pi))
            cone.outerAngle = std::clamp(sf::radians(outerAngle), cone.innerAngle, sf::degrees(360));

        if (auto outerGain = cone.outerGain; ImGui::DragFloat("Outer Gain", &outerGain, 0.001f, 0.f, 1.f))
            cone.outerGain = outerGain;

        sf::Listener::setCone(cone);

        if (ImGui::Button("Add Sound"))
            objects.emplace_back(std::make_unique<Sound>(font, listenerPosition));
        ImGui::SameLine();
        if (ImGui::Button("Add Music"))
            objects.emplace_back(std::make_unique<Music>(font, listenerPosition));
        ImGui::SameLine();
        if (ImGui::Button("Add Tone"))
            objects.emplace_back(std::make_unique<Tone>(font, listenerPosition));

        ImGui::End();

        window.clear();

        for (auto& object : objects)
            object->draw(window);
        marker.draw(window);
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
}
