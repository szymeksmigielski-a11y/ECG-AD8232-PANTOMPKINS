//RecorderPanel.hpp
#ifndef RECORDERPANEL_HPP
#define RECORDERPANEL_HPP

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <functional>
#include <string>
#include "DataLoader.hpp"
#include "ECGRecorder.hpp"

// Ikona Arduino w prawym gornym rogu + okienko nagrywania z ESP32
class RecorderPanel {
private:
    const sf::Font& font;
    std::filesystem::path dataDir;
    std::function<void()> onSaved;   // wywolywane po zapisaniu nowego pliku

    DataLoader loader;
    ECGRecorder recorder;
    bool visible = false;
    bool saveAsEkg = false;
    std::string infoMessage;

    // nazwa pliku wpisana przez uzytkownika (pusta = nazwa automatyczna)
    static constexpr size_t MAX_NAME_LENGTH = 32;
    std::string fileName;
    bool nameFocused = false;
    sf::Clock cursorClock;

    // koncowka nagrania odrzucana przy zapisie (ruch reki przy klikaniu "Stop recording")
    static constexpr long long TRIM_END_MS = 3000;

    // odliczanie przed startem nagrywania
    static constexpr float COUNTDOWN_SECONDS = 7.0f;
    bool countingDown = false;
    sf::Clock countdownClock;

    sf::RectangleShape iconButton;
    sf::RectangleShape overlay;
    sf::RectangleShape panel;
    sf::RectangleShape closeButton;
    sf::RectangleShape nameField;
    sf::RectangleShape csvButton;
    sf::RectangleShape ekgButton;
    sf::RectangleShape recordButton;
    sf::RectangleShape stopButton;

    void open();
    void close();
    void update();          // konczy odliczanie i startuje nagrywanie
    void saveRecording();

    void drawIcon(sf::RenderWindow& window);
    void drawText(sf::RenderWindow& window, const std::string& utf8, unsigned size,
        sf::Vector2f pos, sf::Color color);
    void drawButton(sf::RenderWindow& window, sf::RectangleShape& button, const std::string& label,
        bool enabled, bool highlighted, sf::Color highlightFill, sf::Color highlightOutline);

public:
    RecorderPanel(const sf::Font& font, const std::filesystem::path& dataDir, std::function<void()> onSaved);

    bool isOpen() const { return visible; }

    // true = klikniecie obsluzyla ikona albo otwarte okienko
    bool handleClick(sf::Vector2f mousePos);

    // znak z sf::Event::TextEntered - wpisywanie nazwy pliku
    void handleText(sf::Uint32 unicode);

    // przy zamykaniu aplikacji: zatrzymuje i zapisuje trwajace nagranie
    void shutdown();

    void draw(sf::RenderWindow& window);
};

#endif
