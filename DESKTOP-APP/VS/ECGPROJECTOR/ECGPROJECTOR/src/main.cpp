//main.cpp
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include "ECGSample.hpp"
#include "DataLoader.hpp"
#include "PanTompkins.hpp"
#include "Stats.hpp"
#include "ECGException.hpp" 
#include <filesystem>

enum class AppState {
    FILE_BROWSER,
    SIGNAL_VIEW
};

// sciezki wzgledne - liczone od katalogu roboczego (folder z plikiem .exe)
const std::string           FONT_PATH = "assets/font.ttf";
const std::filesystem::path DATA_DIR  = "data";

int main() {

    sf::RenderWindow window(sf::VideoMode(1200, 1000), "EKG Browser");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile(FONT_PATH)) {
        std::cerr << "Blad: Nie znaleziono pliku czcionki font.ttf!" << std::endl;
        return -1;
    }

    DataLoader loader;
    std::vector<std::string> csvFiles;
    try {
        csvFiles = loader.scanFolder(DATA_DIR.string());
    }
    catch (const ECGException& e) {
        std::cerr << "Blad skanowania folderu: " << e.what() << std::endl;
        return -1;
    }

    // --- KAFELKI ---
    const float TILE_SIZE = 180.0f;
    const float TILE_PADDING = 20.0f;
    const float TILE_START_X = 30.0f;
    const float TILE_START_Y = 80.0f;
    const int   TILES_PER_ROW = 5;

    std::vector<sf::RectangleShape> tiles;
    std::vector<sf::Text>           tileLabels;
    std::vector<sf::RectangleShape> convertButtons;
    std::vector<sf::Text>           convertLabels;
    std::vector<int>                convertButtonFileIndex;

    auto rebuildTiles = [&]() {
        tiles.clear();
        tileLabels.clear();
        convertButtons.clear();
        convertLabels.clear();
        convertButtonFileIndex.clear();

        for (size_t i = 0; i < csvFiles.size(); i++) {
            int col = i % TILES_PER_ROW;
            int row = i / TILES_PER_ROW;

            float x = TILE_START_X + col * (TILE_SIZE + TILE_PADDING);
            float y = TILE_START_Y + row * (TILE_SIZE + TILE_PADDING);

            sf::RectangleShape tile(sf::Vector2f(TILE_SIZE, TILE_SIZE));
            tile.setPosition(x, y);
            tile.setFillColor(sf::Color(40, 40, 60));
            tile.setOutlineColor(sf::Color(100, 100, 180));
            tile.setOutlineThickness(2.0f);
            tiles.push_back(tile);

            sf::Text label;
            label.setFont(font);
            label.setCharacterSize(13);
            label.setFillColor(sf::Color::White);
            std::string name = csvFiles[i];
            if (name.size() > 20) name = name.substr(0, 18) + "..";
            label.setString(name);
            label.setPosition(x + 10.0f, y + TILE_SIZE / 2.0f - 10.0f);
            tileLabels.push_back(label);

            // przycisk konwersji - tylko dla plikow .csv
            std::filesystem::path filePath(csvFiles[i]);
            if (filePath.extension() == ".csv") {
                sf::RectangleShape convBtn(sf::Vector2f(80.0f, 22.0f));
                convBtn.setPosition(x + TILE_SIZE - 90.0f, y + TILE_SIZE - 30.0f);
                convBtn.setFillColor(sf::Color(70, 90, 70));
                convBtn.setOutlineColor(sf::Color(120, 200, 120));
                convBtn.setOutlineThickness(1.0f);
                convertButtons.push_back(convBtn);

                sf::Text convLbl("Konwertuj", font, 11);
                convLbl.setPosition(x + TILE_SIZE - 85.0f, y + TILE_SIZE - 27.0f);
                convLbl.setFillColor(sf::Color::White);
                convertLabels.push_back(convLbl);

                convertButtonFileIndex.push_back((int)i);
            }
        }
        };

    rebuildTiles();

    sf::Text title("Wybierz plik EKG:", font, 22);
    title.setPosition(30.0f, 30.0f);
    title.setFillColor(sf::Color(180, 180, 255));

    // --- DANE SYGNAŁU ---
    AppState currentState = AppState::FILE_BROWSER;
    std::vector<ECGSample> rawData;
    sf::VertexArray plot_raw;
    sf::VertexArray plot_processed;
    sf::VertexArray qrsLines;
    sf::View signalView(sf::FloatRect(0, 0, 1200, 1000));
    float scrollSpeed = 10.0f;
    ECGStats currentStats;

    // --- PRZYCISK WRÓĆ ---
    sf::RectangleShape backButton(sf::Vector2f(120.0f, 35.0f));
    backButton.setPosition(10.0f, 10.0f);
    backButton.setFillColor(sf::Color(60, 60, 90));
    backButton.setOutlineColor(sf::Color(100, 100, 180));
    backButton.setOutlineThickness(2.0f);

    sf::Text backLabel("< Wroc", font, 16);
    backLabel.setPosition(20.0f, 18.0f);
    backLabel.setFillColor(sf::Color::White);

    // -------------------------------------------------------
    // GŁÓWNA PĘTLA
    // -------------------------------------------------------
    while (window.isOpen()) {

        sf::Event event;
        while (window.pollEvent(event)) {

            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {

                sf::Vector2f mousePos(
                    (float)event.mouseButton.x,
                    (float)event.mouseButton.y
                );

                if (currentState == AppState::FILE_BROWSER) {

                    bool handledConvert = false;
                    for (size_t i = 0; i < convertButtons.size(); i++) {
                        if (convertButtons[i].getGlobalBounds().contains(mousePos)) {

                            int fileIdx = convertButtonFileIndex[i];
                            std::string fullPath = (DATA_DIR / csvFiles[fileIdx]).string();

                            try {
                                loader.convertCsvToBinary(fullPath);
                                csvFiles = loader.scanFolder(DATA_DIR.string());
                                rebuildTiles();
                            }
                            catch (const ECGException& e) {
                                std::cerr << "Blad konwersji: " << e.what() << std::endl;
                            }

                            handledConvert = true;
                            break;
                        }
                    }

                    if (!handledConvert) {
                        for (size_t i = 0; i < tiles.size(); i++) {
                            if (tiles[i].getGlobalBounds().contains(mousePos)) {

                                std::string path = (DATA_DIR / csvFiles[i]).string();
                                std::string ext = std::filesystem::path(path).extension().string();

                                try {
                                    if (ext == ".ekg")
                                        rawData = loader.loadFromBinary(path);
                                    else
                                        rawData = loader.loadFromCSV(path);
                                }
                                catch (const ECGException& e) {
                                    std::cerr << "Blad wczytywania: " << e.what() << std::endl;
                                    break;
                                }

                                PanTompkins_Pipeline panTompkins;
                                auto result = panTompkins.run(rawData);

                                // wykres surowego sygnału (góra, zielony)
                                plot_raw = sf::VertexArray(sf::LineStrip, rawData.size());
                                for (size_t j = 0; j < rawData.size(); j++) {
                                    float x = float(j);
                                    float y = 250.0f - float(rawData[j].value * 0.05f);
                                    plot_raw[j].position = sf::Vector2f(x, y);
                                    plot_raw[j].color = sf::Color::Green;
                                }

                                // wykres po pipeline (dół, czerwony)
                                plot_processed = sf::VertexArray(sf::LineStrip, result.integrated.size());
                                for (size_t j = 0; j < result.integrated.size(); j++) {
                                    float x = float(j);
                                    float y = 700.0f - float(result.integrated[j].value * 0.00001f);
                                    plot_processed[j].position = sf::Vector2f(x, y);
                                    plot_processed[j].color = sf::Color::Red;
                                }

                                // linie QRS (pomarańczowe)
                                qrsLines = sf::VertexArray(sf::Lines);
                                for (size_t j = 0; j < result.qrsMarkers.size(); j++) {
                                    if (result.qrsMarkers[j].value == 999999.0) {
                                        sf::Vertex top(sf::Vector2f(float(j), 0.0f), sf::Color(255, 140, 0));
                                        sf::Vertex bot(sf::Vector2f(float(j), 900.0f), sf::Color(255, 140, 0));
                                        qrsLines.append(top);
                                        qrsLines.append(bot);
                                    }
                                }

                                currentStats = computeStats(result.qrsMarkers);

                                signalView = sf::View(sf::FloatRect(0, 0, 1200, 1000));
                                currentState = AppState::SIGNAL_VIEW;
                                break;
                            }
                        }
                    }
                }
                else if (currentState == AppState::SIGNAL_VIEW) {
                    if (backButton.getGlobalBounds().contains(mousePos)) {
                        currentState = AppState::FILE_BROWSER;
                        rawData.clear();
                    }
                }
            }
        }

        // --- PRZEWIJANIE ---
        if (currentState == AppState::SIGNAL_VIEW) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                signalView.move(scrollSpeed, 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                signalView.move(-scrollSpeed, 0);

            sf::Vector2f center = signalView.getCenter();
            if (center.x < 600) center.x = 600;
            if (center.x > (float)rawData.size() - 600)
                center.x = (float)rawData.size() - 600;
            signalView.setCenter(center);
        }

        // -------------------------------------------------------
        // RYSOWANIE
        // -------------------------------------------------------
        window.clear(sf::Color(20, 20, 30));

        if (currentState == AppState::FILE_BROWSER) {
            window.setView(window.getDefaultView());
            window.draw(title);
            for (auto& tile : tiles)          window.draw(tile);
            for (auto& label : tileLabels)    window.draw(label);
            for (auto& btn : convertButtons)  window.draw(btn);
            for (auto& lbl : convertLabels)   window.draw(lbl);
        }
        else if (currentState == AppState::SIGNAL_VIEW) {

            window.setView(signalView);
            window.draw(qrsLines);
            window.draw(plot_raw);
            window.draw(plot_processed);

            window.setView(window.getDefaultView());
            window.draw(backButton);
            window.draw(backLabel);

            float startX = signalView.getCenter().x - 600.0f;
            for (int i = 0; i < 1200; i += 100) {
                float sampleIdx = startX + i;
                if (sampleIdx < 0 || sampleIdx >= (float)rawData.size()) continue;
                float timeSeconds = sampleIdx * 0.005f;
                std::stringstream ss;
                ss << std::fixed << std::setprecision(1) << timeSeconds << "s";
                sf::Text label(ss.str(), font, 13);
                label.setPosition(float(i), 960.0f);
                label.setFillColor(sf::Color(180, 180, 180));
                window.draw(label);
            }

            sf::RectangleShape statsPanel(sf::Vector2f(220.0f, 130.0f));
            statsPanel.setPosition(970.0f, 10.0f);
            statsPanel.setFillColor(sf::Color(30, 30, 50, 220));
            statsPanel.setOutlineColor(sf::Color(100, 100, 180));
            statsPanel.setOutlineThickness(2.0f);
            window.draw(statsPanel);

            auto makeStatText = [&](const std::string& str, float y) {
                sf::Text t(str, font, 15);
                t.setPosition(980.0f, y);
                t.setFillColor(sf::Color(200, 200, 255));
                window.draw(t);
                };

            std::stringstream ssAvg, ssMin, ssMax, ssBeats;
            ssAvg << "Tetno srednie: " << (int)currentStats.getAvgHR() << " BPM";
            ssMin << "Tetno min:     " << (int)currentStats.getMinHR() << " BPM";
            ssMax << "Tetno max:     " << (int)currentStats.getMaxHR() << " BPM";
            ssBeats << "Uderzenia:     " << currentStats.getTotalBeats();

            makeStatText(ssAvg.str(), 20.0f);
            makeStatText(ssMin.str(), 50.0f);
            makeStatText(ssMax.str(), 80.0f);
            makeStatText(ssBeats.str(), 110.0f);
        }

        window.display();
    }

    return 0;
}