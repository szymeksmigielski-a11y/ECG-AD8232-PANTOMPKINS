//RecorderPanel.cpp
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "RecorderPanel.hpp"
#include "ECGException.hpp"

namespace {
    const float PANEL_X = 340.0f;
    const float PANEL_Y = 250.0f;
    const float PANEL_W = 520.0f;
    const float PANEL_H = 400.0f;

    sf::RectangleShape makeRect(float x, float y, float w, float h) {
        sf::RectangleShape r(sf::Vector2f(w, h));
        r.setPosition(x, y);
        return r;
    }

    // automatyczna nazwa z data i godzina, np. zapis_200Hz_20261014_153012
    std::string makeDefaultName() {
        std::time_t t = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &t);
        std::ostringstream ss;
        ss << "zapis_200Hz_" << std::put_time(&local, "%Y%m%d_%H%M%S");
        return ss.str();
    }

    // znak dozwolony w nazwie pliku; polskie litery zamieniane na lacinskie, 0 = znak odrzucony
    char toFileNameChar(sf::Uint32 c) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
            return (char)c;
        if (c == ' ') return '_';
        switch (c) {
        case 0x0105: return 'a'; case 0x0104: return 'A';   // ą Ą
        case 0x0107: return 'c'; case 0x0106: return 'C';   // ć Ć
        case 0x0119: return 'e'; case 0x0118: return 'E';   // ę Ę
        case 0x0142: return 'l'; case 0x0141: return 'L';   // ł Ł
        case 0x0144: return 'n'; case 0x0143: return 'N';   // ń Ń
        case 0x00F3: return 'o'; case 0x00D3: return 'O';   // ó Ó
        case 0x015B: return 's'; case 0x015A: return 'S';   // ś Ś
        case 0x017A: return 'z'; case 0x0179: return 'Z';   // ź Ź
        case 0x017C: return 'z'; case 0x017B: return 'Z';   // ż Ż
        }
        return 0;
    }

    // nie nadpisujemy istniejacych plikow: nazwa, nazwa_2, nazwa_3...
    std::string makeUniqueFileName(const std::filesystem::path& dir, const std::string& base, const std::string& ext) {
        std::string name = base + ext;
        for (int i = 2; std::filesystem::exists(dir / name); i++)
            name = base + "_" + std::to_string(i) + ext;
        return name;
    }
}

RecorderPanel::RecorderPanel(const sf::Font& font_m, const std::filesystem::path& dataDir_m,
    std::function<void()> onSaved_m)
    : font(font_m), dataDir(dataDir_m), onSaved(std::move(onSaved_m))
{
    iconButton   = makeRect(1120.0f, 20.0f, 60.0f, 40.0f);
    overlay      = makeRect(0.0f, 0.0f, 1200.0f, 1000.0f);
    panel        = makeRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H);
    closeButton  = makeRect(PANEL_X + PANEL_W - 40.0f, PANEL_Y + 10.0f, 30.0f, 30.0f);
    nameField    = makeRect(PANEL_X + 160.0f, PANEL_Y + 120.0f, 340.0f, 30.0f);
    csvButton    = makeRect(PANEL_X + 160.0f, PANEL_Y + 170.0f, 70.0f, 30.0f);
    ekgButton    = makeRect(PANEL_X + 240.0f, PANEL_Y + 170.0f, 70.0f, 30.0f);
    recordButton = makeRect(PANEL_X + 20.0f, PANEL_Y + 225.0f, 140.0f, 40.0f);
    stopButton   = makeRect(PANEL_X + 180.0f, PANEL_Y + 225.0f, 180.0f, 40.0f);

    overlay.setFillColor(sf::Color(0, 0, 0, 160));
    panel.setFillColor(sf::Color(30, 30, 50));
    panel.setOutlineColor(sf::Color(100, 100, 180));
    panel.setOutlineThickness(2.0f);
}

void RecorderPanel::open() {
    visible = true;
    infoMessage.clear();
    recorder.start();
}

void RecorderPanel::close() {
    countingDown = false;
    nameFocused = false;
    if (recorder.isRecording())
        saveRecording();
    recorder.stop();
    visible = false;
}

void RecorderPanel::update() {
    if (countingDown && countdownClock.getElapsedTime().asSeconds() >= COUNTDOWN_SECONDS) {
        countingDown = false;
        if (!recorder.startRecording())
            infoMessage = "Nagrywanie nie rozpoczęte – sprawdź elektrody";
    }
}

void RecorderPanel::shutdown() {
    if (visible)
        close();
}

void RecorderPanel::saveRecording() {
    std::vector<ECGSample> data = recorder.stopRecording();
    if (data.empty()) {
        infoMessage = "Brak próbek – nic nie zapisano";
        return;
    }

    // odrzucamy ostatnie 3 s (wg timestampow z Arduino) - ruch reki przy klikaniu zaklóca sygnal
    long long cutoff = data.back().timestamp - TRIM_END_MS;
    while (!data.empty() && data.back().timestamp > cutoff)
        data.pop_back();
    if (data.empty()) {
        infoMessage = "Nagranie krótsze niż 3 s – nic nie zapisano";
        return;
    }

    std::string baseName = fileName.empty() ? makeDefaultName() : fileName;
    std::string savedName = makeUniqueFileName(dataDir, baseName, saveAsEkg ? ".ekg" : ".csv");
    std::string path = (dataDir / savedName).string();
    try {
        if (saveAsEkg)
            loader.saveToBinary(path, data);
        else
            loader.saveToCSV(path, data);
        std::cout << "Zapisano " << data.size() << " probek -> " << path << std::endl;
        infoMessage = "Zapisano: " + savedName + " (" + std::to_string(data.size()) + " próbek)\nOstatnie 3 s nagrania zostały obcięte";
        fileName.clear();   // kolejne nagranie znow dostaje nazwe automatyczna
        if (onSaved) onSaved();
    }
    catch (const ECGException& e) {
        std::cerr << "Blad zapisu: " << e.what() << std::endl;
        infoMessage = std::string("Błąd zapisu: ") + e.what();
    }
}

bool RecorderPanel::handleClick(sf::Vector2f mousePos) {
    if (!visible) {
        if (iconButton.getGlobalBounds().contains(mousePos)) {
            open();
            return true;
        }
        return false;
    }

    // okienko jest modalne - kazde klikniecie nalezy do niego
    bool isRecording = recorder.isRecording();
    bool busy = isRecording || countingDown;

    // klikniecie w pole nazwy aktywuje je, klikniecie gdziekolwiek indziej - dezaktywuje
    nameFocused = !busy && nameField.getGlobalBounds().contains(mousePos);
    if (nameFocused)
        cursorClock.restart();

    if (closeButton.getGlobalBounds().contains(mousePos)) {
        close();
    }
    else if (!busy && csvButton.getGlobalBounds().contains(mousePos)) {
        saveAsEkg = false;
    }
    else if (!busy && ekgButton.getGlobalBounds().contains(mousePos)) {
        saveAsEkg = true;
    }
    else if (!busy && recorder.getStatus() == DeviceStatus::READY &&
        recordButton.getGlobalBounds().contains(mousePos)) {
        countingDown = true;
        countdownClock.restart();
        infoMessage.clear();
    }
    else if (busy && stopButton.getGlobalBounds().contains(mousePos)) {
        if (countingDown) {
            countingDown = false;
            infoMessage = "Odliczanie przerwane – nic nie zapisano";
        }
        else {
            saveRecording();
        }
    }
    return true;
}

void RecorderPanel::handleText(sf::Uint32 unicode) {
    if (!visible || !nameFocused)
        return;

    if (unicode == '\b') {                     // Backspace
        if (!fileName.empty()) fileName.pop_back();
    }
    else if (unicode == '\r' || unicode == 27) {   // Enter / Escape - koniec edycji
        nameFocused = false;
    }
    else if (fileName.size() < MAX_NAME_LENGTH) {
        char c = toFileNameChar(unicode);
        if (c) fileName += c;
    }
    cursorClock.restart();   // kursor widoczny zaraz po wpisaniu znaku
}

void RecorderPanel::drawText(sf::RenderWindow& window, const std::string& utf8, unsigned size,
    sf::Vector2f pos, sf::Color color) {
    sf::Text text(sf::String::fromUtf8(utf8.begin(), utf8.end()), font, size);
    text.setPosition(pos);
    text.setFillColor(color);
    window.draw(text);
}

void RecorderPanel::drawButton(sf::RenderWindow& window, sf::RectangleShape& button, const std::string& label,
    bool enabled, bool highlighted, sf::Color highlightFill, sf::Color highlightOutline) {
    if (!enabled) {
        button.setFillColor(sf::Color(45, 45, 55));
        button.setOutlineColor(sf::Color(70, 70, 90));
    }
    else if (highlighted) {
        button.setFillColor(highlightFill);
        button.setOutlineColor(highlightOutline);
    }
    else {
        button.setFillColor(sf::Color(60, 60, 90));
        button.setOutlineColor(sf::Color(100, 100, 180));
    }
    button.setOutlineThickness(2.0f);
    window.draw(button);

    sf::Text text(sf::String::fromUtf8(label.begin(), label.end()), font, 16);
    sf::FloatRect bounds = text.getLocalBounds();
    sf::Vector2f pos = button.getPosition();
    sf::Vector2f size = button.getSize();
    text.setPosition(pos.x + (size.x - bounds.width) / 2.0f - bounds.left,
        pos.y + (size.y - bounds.height) / 2.0f - bounds.top);
    text.setFillColor(enabled ? sf::Color::White : sf::Color(110, 110, 110));
    window.draw(text);
}

void RecorderPanel::drawIcon(sf::RenderWindow& window) {
    // stylizowana ikona Arduino: turkusowe tlo i symbol nieskonczonosci z "-" i "+"
    iconButton.setFillColor(sf::Color(0, 129, 132));
    iconButton.setOutlineColor(visible ? sf::Color::White : sf::Color(100, 100, 180));
    iconButton.setOutlineThickness(2.0f);
    window.draw(iconButton);

    const sf::Color white(255, 255, 255);
    const float cy = 40.0f;
    for (float cx : { 1141.0f, 1159.0f }) {
        sf::CircleShape ring(8.0f);
        ring.setOrigin(8.0f, 8.0f);
        ring.setPosition(cx, cy);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(white);
        ring.setOutlineThickness(3.0f);
        window.draw(ring);
    }
    sf::RectangleShape minus(sf::Vector2f(7.0f, 2.0f));
    minus.setPosition(1137.5f, cy - 1.0f);
    minus.setFillColor(white);
    window.draw(minus);

    sf::RectangleShape plusH(sf::Vector2f(7.0f, 2.0f));
    plusH.setPosition(1155.5f, cy - 1.0f);
    plusH.setFillColor(white);
    window.draw(plusH);
    sf::RectangleShape plusV(sf::Vector2f(2.0f, 7.0f));
    plusV.setPosition(1158.0f, cy - 3.5f);
    plusV.setFillColor(white);
    window.draw(plusV);
}

void RecorderPanel::draw(sf::RenderWindow& window) {
    drawIcon(window);
    if (!visible) return;
    update();

    window.draw(overlay);
    window.draw(panel);
    drawText(window, "Nagrywanie z Arduino", 20, { PANEL_X + 20.0f, PANEL_Y + 15.0f }, sf::Color(180, 180, 255));
    drawButton(window, closeButton, "X", true, false, sf::Color(), sf::Color());

    // --- STATUS POLACZENIA ---
    DeviceStatus status = recorder.getStatus();
    std::string statusText;
    sf::Color statusColor;
    switch (status) {
    case DeviceStatus::NO_DEVICE:
        statusText = "Podłącz Arduino";
        statusColor = sf::Color(220, 80, 80);
        break;
    case DeviceStatus::CONNECTED:
        statusText = "Podłączono Arduino";
        statusColor = sf::Color(230, 200, 80);
        break;
    case DeviceStatus::NO_ELECTRODES:
        statusText = "Podłącz elektrody";
        statusColor = sf::Color(240, 150, 60);
        break;
    case DeviceStatus::READY:
        statusText = "Elektrody podłączone";
        statusColor = sf::Color(100, 210, 120);
        break;
    }

    sf::CircleShape dot(7.0f);
    dot.setPosition(PANEL_X + 20.0f, PANEL_Y + 67.0f);
    dot.setFillColor(statusColor);
    window.draw(dot);
    drawText(window, statusText, 18, { PANEL_X + 42.0f, PANEL_Y + 62.0f }, statusColor);
    if (status != DeviceStatus::NO_DEVICE)
        drawText(window, "Port: " + recorder.getPortName(), 14, { PANEL_X + 42.0f, PANEL_Y + 88.0f },
            sf::Color(180, 180, 180));

    bool isRecording = recorder.isRecording();
    bool busy = isRecording || countingDown;
    std::string extension = saveAsEkg ? ".ekg" : ".csv";

    // --- NAZWA PLIKU ---
    drawText(window, "Nazwa pliku:", 16, { PANEL_X + 20.0f, PANEL_Y + 125.0f }, sf::Color(200, 200, 255));
    nameField.setFillColor(busy ? sf::Color(45, 45, 55) : sf::Color(20, 20, 35));
    nameField.setOutlineColor(nameFocused ? sf::Color(180, 180, 255) : sf::Color(100, 100, 180));
    nameField.setOutlineThickness(2.0f);
    window.draw(nameField);

    sf::Vector2f fieldText(nameField.getPosition().x + 8.0f, nameField.getPosition().y + 5.0f);
    if (fileName.empty() && !nameFocused) {
        drawText(window, "automatyczna (data i godzina)", 15, fieldText, sf::Color(110, 110, 130));
    }
    else {
        sf::Text typed(fileName, font, 15);
        typed.setPosition(fieldText);
        typed.setFillColor(busy ? sf::Color(150, 150, 150) : sf::Color::White);
        window.draw(typed);

        float endX = fieldText.x + typed.getLocalBounds().width + 2.0f;
        bool cursorVisible = nameFocused && std::fmod(cursorClock.getElapsedTime().asSeconds(), 1.0f) < 0.5f;
        if (cursorVisible)
            drawText(window, "|", 15, { endX, fieldText.y }, sf::Color::White);
        drawText(window, extension, 15, { endX + 6.0f, fieldText.y }, sf::Color(110, 110, 130));
    }

    // --- FORMAT ZAPISU ---
    const sf::Color formatFill(70, 90, 70);
    const sf::Color formatOutline(120, 200, 120);
    drawText(window, "Format zapisu:", 16, { PANEL_X + 20.0f, PANEL_Y + 175.0f }, sf::Color(200, 200, 255));
    drawButton(window, csvButton, "CSV", !busy || !saveAsEkg, !saveAsEkg, formatFill, formatOutline);
    drawButton(window, ekgButton, "EKG", !busy || saveAsEkg, saveAsEkg, formatFill, formatOutline);

    // --- RECORD / STOP ---
    bool canRecord = !busy && status == DeviceStatus::READY;
    drawButton(window, recordButton, "Record", canRecord, true, sf::Color(120, 40, 40), sf::Color(220, 80, 80));
    drawButton(window, stopButton, "Stop recording", busy, false, sf::Color(), sf::Color());

    // --- INFORMACJE ---
    if (countingDown) {
        float left = COUNTDOWN_SECONDS - countdownClock.getElapsedTime().asSeconds();
        int secondsLeft = std::max(1, (int)std::ceil(left));
        drawText(window, std::to_string(secondsLeft), 48, { PANEL_X + 420.0f, PANEL_Y + 210.0f },
            sf::Color(240, 200, 90));
        drawText(window, "Ustaw się w wygodnej pozycji", 18, { PANEL_X + 20.0f, PANEL_Y + 288.0f },
            sf::Color(240, 200, 90));
    }
    else if (isRecording) {
        std::ostringstream ss;
        ss << "REC  " << std::fixed << std::setprecision(1) << recorder.getRecordingSeconds()
            << " s  |  " << recorder.getSampleCount() << " próbek";
        drawText(window, ss.str(), 16, { PANEL_X + 20.0f, PANEL_Y + 290.0f }, sf::Color(240, 90, 90));
    }
    if (!infoMessage.empty())
        drawText(window, infoMessage, 14, { PANEL_X + 20.0f, PANEL_Y + 335.0f }, sf::Color(200, 200, 200));
}
