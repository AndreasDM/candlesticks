#include <SFML/Graphics.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

constexpr float MARGIN_X_PERCENT = 0.05f;
constexpr float MARGIN_Y_PERCENT = 0.05f;
constexpr float MIN_VIEW_WIDTH = 5.f;
constexpr auto MODAL_FILL = sf::Color(255, 255, 200);
constexpr float CANDLE_WIDTH_FACTOR = 0.8f;
constexpr float SPACING_FACTOR = 0.2f;
constexpr int NUM_HORIZONTAL_GRID_LINES = 5;
constexpr auto LIGHT_GRAY = sf::Color(240, 240, 240);

struct Candlestick {
  float open, close, high, low, volume;
  std::string date;
};

struct ChartLine {
  float startCandle, startPrice, endCandle, endPrice;
};

struct ChartRect {
  float startCandle, startPrice, endCandle, endPrice;
};

struct ChartText {
  float candle, price;
  std::string text;
};

std::vector<Candlestick> parseData(const std::string &filename)
{
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filename << std::endl;
    return {};
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  std::vector<Candlestick> candles;
  std::stringstream ss(content);
  std::string line;
  std::getline(ss, line); // Skip header
  while (std::getline(ss, line)) {
    std::stringstream ls(line);
    std::string date, close, volume, open, high, low;
    std::getline(ls, date, ',');
    std::getline(ls, close, ',');
    std::getline(ls, volume, ',');
    std::getline(ls, open, ',');
    std::getline(ls, high, ',');
    std::getline(ls, low, ',');
    Candlestick candle;
    candle.date = date;
    candle.close = std::stof(close.substr(1));
    candle.volume = std::stof(volume);
    candle.open = std::stof(open.substr(1));
    candle.high = std::stof(high.substr(1));
    candle.low = std::stof(low.substr(1));
    candles.push_back(candle);
  }
  return candles;
}

void drawCandlesticks(sf::RenderWindow &window,
                      const std::vector<Candlestick> &candles, float viewStart,
                      float viewEnd, float marginX, float chartTop,
                      float chartHeight, float candleWidth, float spacing,
                      float viewMinPrice, float viewMaxPrice)
{
  float viewPriceRange = viewMaxPrice - viewMinPrice;
  for (size_t i = static_cast<size_t>(viewStart);
       i <= static_cast<size_t>(viewEnd); ++i) {
    const auto &candle = candles[i];
    float x = marginX + ((i - viewStart) * (candleWidth + spacing));
    float highY = chartTop +
                  ((viewMaxPrice - candle.high) / viewPriceRange * chartHeight);
    float lowY =
        chartTop + ((viewMaxPrice - candle.low) / viewPriceRange * chartHeight);
    float openY = chartTop +
                  ((viewMaxPrice - candle.open) / viewPriceRange * chartHeight);
    float closeY = chartTop + ((viewMaxPrice - candle.close) / viewPriceRange *
                               chartHeight);

    sf::RectangleShape wick({1, lowY - highY});
    wick.setPosition({x + candleWidth / 2, highY});
    wick.setFillColor(sf::Color::Black);
    window.draw(wick);

    float bodyHeight = std::abs(openY - closeY);
    float bodyY = std::min(openY, closeY);
    sf::RectangleShape body({candleWidth, bodyHeight});
    body.setPosition({x + 0.5f, bodyY});
    body.setFillColor(candle.close >= candle.open ? sf::Color::Green
                                                  : sf::Color::Red);
    window.draw(body);
  }
}

void drawDateLabels(sf::RenderWindow &window,
                    const std::vector<Candlestick> &candles, float viewStart,
                    float viewEnd, float marginX, float chartBottom,
                    float chartWidth, float candleWidth, float spacing,
                    const sf::Font &font)
{
  float viewWidth = viewEnd - viewStart + 1;
  size_t numLabels = static_cast<size_t>(viewWidth);
  size_t step =
      std::max<size_t>(1, numLabels / std::max<size_t>(1, chartWidth / 50));
  for (size_t i = static_cast<size_t>(viewStart);
       i <= static_cast<size_t>(viewEnd); i += step) {
    const auto &candle = candles[i];
    float x = marginX + ((i - viewStart) * (candleWidth + spacing));
    sf::Text dateText(font, candle.date, 12);
    dateText.setFillColor(sf::Color::Black);
    dateText.setRotation(sf::degrees(45));
    sf::FloatRect bounds = dateText.getLocalBounds();
    dateText.setOrigin({bounds.size.x / 2, 0});
    dateText.setPosition({x + candleWidth / 2, chartBottom + 50});
    window.draw(dateText);
  }
}

void drawGridLines(sf::RenderWindow &window, float viewStart, float viewEnd,
                   float marginX, float chartTop, float chartWidth,
                   float chartHeight, float candleWidth, float spacing,
                   float viewMinPrice, float viewMaxPrice)
{
  float viewPriceRange = viewMaxPrice - viewMinPrice;
  float priceStep = viewPriceRange / (NUM_HORIZONTAL_GRID_LINES - 1);
  // Horizontal grid lines
  for (int i = 0; i < NUM_HORIZONTAL_GRID_LINES; ++i) {
    float price = viewMinPrice + i * priceStep;
    float y =
        chartTop + ((viewMaxPrice - price) / viewPriceRange * chartHeight);
    sf::Vertex line[] = {{{marginX, y}, LIGHT_GRAY},
                         {{marginX + chartWidth, y}, LIGHT_GRAY}};
    window.draw(line, 2, sf::PrimitiveType::Lines);
  }
  // Vertical grid lines
  for (size_t i = static_cast<size_t>(viewStart);
       i <= static_cast<size_t>(viewEnd); ++i) {
    float x =
        marginX + ((i - viewStart) * (candleWidth + spacing)) + candleWidth / 2;
    sf::Vertex line[] = {{{x, chartTop}, LIGHT_GRAY},
                         {{x, chartTop + chartHeight}, LIGHT_GRAY}};
    window.draw(line, 2, sf::PrimitiveType::Lines);
  }
}

void drawLines(sf::RenderWindow &window, const std::vector<ChartLine> &lines,
               float viewStart, float viewEnd, float marginX, float chartTop,
               float chartWidth, float chartHeight, float viewMinPrice,
               float viewMaxPrice)
{
  float viewWidth = viewEnd - viewStart + 1;
  float viewPriceRange = viewMaxPrice - viewMinPrice;
  for (const auto &line : lines) {
    float startX =
        marginX + ((line.startCandle - viewStart) / viewWidth) * chartWidth;
    float startY = chartTop + ((viewMaxPrice - line.startPrice) /
                               viewPriceRange * chartHeight);
    float endX =
        marginX + ((line.endCandle - viewStart) / viewWidth) * chartWidth;
    float endY = chartTop + ((viewMaxPrice - line.endPrice) / viewPriceRange *
                             chartHeight);
    sf::Vertex lineVertices[] = {{{startX, startY}, sf::Color::Blue},
                                 {{endX, endY}, sf::Color::Blue}};
    window.draw(lineVertices, 2, sf::PrimitiveType::Lines);
  }
}

void drawRectangles(sf::RenderWindow &window,
                    const std::vector<ChartRect> &rects, float viewStart,
                    float viewEnd, float marginX, float chartTop,
                    float chartWidth, float chartHeight, float viewMinPrice,
                    float viewMaxPrice)
{
  float viewWidth = viewEnd - viewStart + 1;
  float viewPriceRange = viewMaxPrice - viewMinPrice;
  for (const auto &rect : rects) {
    float startX =
        marginX + ((rect.startCandle - viewStart) / viewWidth) * chartWidth;
    float startY = chartTop + ((viewMaxPrice - rect.startPrice) /
                               viewPriceRange * chartHeight);
    float endX =
        marginX + ((rect.endCandle - viewStart) / viewWidth) * chartWidth;
    float endY = chartTop + ((viewMaxPrice - rect.endPrice) / viewPriceRange *
                             chartHeight);
    sf::RectangleShape rectangle;
    rectangle.setPosition({std::min(startX, endX), std::min(startY, endY)});
    rectangle.setSize({std::abs(endX - startX), std::abs(endY - startY)});
    rectangle.setFillColor(sf::Color(0, 0, 0, 51));
    rectangle.setOutlineColor(sf::Color::Black);
    rectangle.setOutlineThickness(1);
    window.draw(rectangle);
  }
}

void drawTexts(sf::RenderWindow &window, const std::vector<ChartText> &texts,
               float viewStart, float viewEnd, float marginX, float chartTop,
               float chartWidth, float chartHeight, float viewMinPrice,
               float viewMaxPrice, const sf::Font &font)
{
  float viewWidth = viewEnd - viewStart + 1;
  float viewPriceRange = viewMaxPrice - viewMinPrice;
  for (const auto &text : texts) {
    float x = marginX + ((text.candle - viewStart) / viewWidth) * chartWidth;
    float y =
        chartTop + ((viewMaxPrice - text.price) / viewPriceRange * chartHeight);
    sf::Text textObj(font, text.text, 16);
    textObj.setFillColor(sf::Color::Black);
    textObj.setPosition({x, y});
    window.draw(textObj);
  }
}

int main()
{
  auto candles = parseData("./NVDA.csv");
  if (candles.empty()) {
    std::cerr << "No data loaded. Exiting." << std::endl;
    return 1;
  }
  std::reverse(candles.begin(), candles.end());

  // Window setup
  unsigned width = 1400, height = 800;
  sf::RenderWindow window(sf::VideoMode({width, height}), "Candlesticks");

  // Font setup
  sf::Font font;
  if (!font.openFromFile("/System/Library/Fonts/SFNSMono.ttf")) {
    std::cerr << "Failed to load font!" << std::endl;
    return 1;
  }

  // Chart dimensions
  const float marginX = width * MARGIN_X_PERCENT;
  const float marginY = height * MARGIN_Y_PERCENT;
  const float chartWidth = width - 2 * marginX;
  const float chartHeight = height - 2 * marginY * 2.f;
  const float chartTop = marginY;
  const float chartBottom = chartTop + chartHeight;

  // View setup
  size_t numCandles = candles.size();
  float viewStart = numCandles > 30 ? numCandles - 30 : 0;
  float viewEnd = numCandles - 1;
  float viewWidth = viewEnd - viewStart + 1;
  float maxViewWidth = static_cast<float>(numCandles);
  float currentViewStart = viewStart, currentViewEnd = viewEnd;
  float viewMinPrice = 0.f, viewMaxPrice = 0.f;
  bool viewChanged = true; // Force initial calculation

  // Annotation variables
  bool isDrawingLine = false;
  ChartLine currentLine;
  std::vector<ChartLine> lines;

  bool isDrawingRect = false;
  ChartRect currentRect;
  std::vector<ChartRect> rects;

  bool isTyping = false;
  ChartText currentText;
  std::string inputBuffer;
  std::vector<ChartText> texts;
  bool ignoreNextT = false;

  // Modal variables
  sf::RectangleShape modalRect;
  sf::Text modalText(font);
  bool showModal = false;

  while (window.isOpen()) {
    // Event handling
    std::optional<sf::Event> event;
    while ((event = window.pollEvent())) {
      if (event->is<sf::Event::Closed>())
        window.close();
      else if (auto *keyEvent = event->getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Q)
          window.close();
        else if (keyEvent->code == sf::Keyboard::Key::T && !isTyping) {
          isTyping = true;
          ignoreNextT = true;
          inputBuffer.clear();
          float mouseX = static_cast<float>(sf::Mouse::getPosition(window).x);
          float mouseY = static_cast<float>(sf::Mouse::getPosition(window).y);
          currentText.candle =
              viewStart + ((mouseX - marginX) / chartWidth) * viewWidth;
          currentText.price =
              viewMaxPrice - ((mouseY - chartTop) / chartHeight) *
                                 (viewMaxPrice - viewMinPrice);
        } else if (keyEvent->code == sf::Keyboard::Key::Enter && isTyping) {
          currentText.text = inputBuffer;
          if (!currentText.text.empty())
            texts.push_back(currentText);
          isTyping = false;
        } else if (keyEvent->code == sf::Keyboard::Key::F) {
          viewStart = 0;
          viewEnd = numCandles - 1;
          viewWidth = maxViewWidth;
          viewChanged = true;
        } else if (keyEvent->code == sf::Keyboard::Key::Left) {
          viewStart = std::max(0.f, viewStart - viewWidth * 0.1f);
          viewEnd = viewStart + viewWidth - 1;
          if (viewEnd >= numCandles) {
            viewEnd = numCandles - 1;
            viewStart = viewEnd - viewWidth + 1;
          }
          viewChanged = true;
        } else if (keyEvent->code == sf::Keyboard::Key::Right) {
          viewEnd = std::min(static_cast<float>(numCandles - 1),
                             viewEnd + viewWidth * 0.1f);
          viewStart = viewEnd - viewWidth + 1;
          if (viewStart < 0) {
            viewStart = 0;
            viewEnd = viewWidth - 1;
          }
          viewChanged = true;
        } else if (keyEvent->code == sf::Keyboard::Key::D && !lines.empty() &&
                   !isTyping) {
          lines.pop_back(); // Delete last line
        } else if (keyEvent->code == sf::Keyboard::Key::R && !rects.empty() &&
                   !isTyping) {
          rects.pop_back(); // Delete last rectangle
        } else if (keyEvent->code == sf::Keyboard::Key::Y && !texts.empty() &&
                   !isTyping) {
          texts.pop_back(); // Delete last text
        }
      } else if (auto *textEvent = event->getIf<sf::Event::TextEntered>()) {
        if (isTyping && textEvent->unicode < 128) {
          char c = static_cast<char>(textEvent->unicode);
          if (ignoreNextT && (c == 't' || c == 'T')) {
            ignoreNextT = false;
            continue;
          }
          if (c == '\b' && !inputBuffer.empty())
            inputBuffer.pop_back();
          else if (c >= 32 && c <= 126)
            inputBuffer += c;
        }
      } else if (auto *scrollEvent =
                     event->getIf<sf::Event::MouseWheelScrolled>()) {
        float mouseX = static_cast<float>(sf::Mouse::getPosition(window).x);
        if (scrollEvent->delta != 0.0f) {
          float zoomFactor = scrollEvent->delta < 0.0f ? 1.05f : 0.95f;
          float focusCandle =
              viewStart + ((mouseX - marginX) / chartWidth) * viewWidth;
          float newViewWidth = viewWidth * zoomFactor;
          newViewWidth =
              std::max(MIN_VIEW_WIDTH, std::min(maxViewWidth, newViewWidth));
          float focusRatioX = (focusCandle - viewStart) / viewWidth;
          viewStart = focusCandle - (newViewWidth * focusRatioX);
          viewEnd = focusCandle + (newViewWidth * (1.f - focusRatioX));
          if (viewStart < 0) {
            viewStart = 0;
            viewEnd = newViewWidth - 1;
          }
          if (viewEnd >= numCandles) {
            viewEnd = numCandles - 1;
            viewStart = viewEnd - newViewWidth + 1;
          }
          viewWidth = viewEnd - viewStart + 1;
          viewChanged = true;
        }
      } else if (auto *mouseEvent =
                     event->getIf<sf::Event::MouseButtonPressed>()) {
        float mouseX = static_cast<float>(mouseEvent->position.x);
        float mouseY = static_cast<float>(mouseEvent->position.y);
        if (mouseEvent->button == sf::Mouse::Button::Left) {
          isDrawingLine = true;
          currentLine.startCandle =
              viewStart + ((mouseX - marginX) / chartWidth) * viewWidth;
          currentLine.startPrice =
              viewMaxPrice - ((mouseY - chartTop) / chartHeight) *
                                 (viewMaxPrice - viewMinPrice);
          currentLine.endCandle = currentLine.startCandle;
          currentLine.endPrice = currentLine.startPrice;
        } else if (mouseEvent->button == sf::Mouse::Button::Right) {
          isDrawingRect = true;
          currentRect.startCandle =
              viewStart + ((mouseX - marginX) / chartWidth) * viewWidth;
          currentRect.startPrice =
              viewMaxPrice - ((mouseY - chartTop) / chartHeight) *
                                 (viewMaxPrice - viewMinPrice);
          currentRect.endCandle = currentRect.startCandle;
          currentRect.endPrice = currentRect.startPrice;
        }
      } else if (auto *mouseEvent =
                     event->getIf<sf::Event::MouseButtonReleased>()) {
        float mouseX = static_cast<float>(mouseEvent->position.x);
        float mouseY = static_cast<float>(mouseEvent->position.y);
        if (mouseEvent->button == sf::Mouse::Button::Left && isDrawingLine) {
          currentLine.endCandle =
              viewStart + ((mouseX - marginX) / chartWidth) * viewWidth;
          currentLine.endPrice =
              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)
                  ? currentLine.startPrice
                  : viewMaxPrice - ((mouseY - chartTop) / chartHeight) *
                                       (viewMaxPrice - viewMinPrice);
          lines.push_back(currentLine);
          isDrawingLine = false;
        } else if (mouseEvent->button == sf::Mouse::Button::Right &&
                   isDrawingRect) {
          currentRect.endCandle =
              viewStart + ((mouseX - marginX) / chartWidth) * viewWidth;
          currentRect.endPrice =
              viewMaxPrice - ((mouseY - chartTop) / chartHeight) *
                                 (viewMaxPrice - viewMinPrice);
          rects.push_back(currentRect);
          isDrawingRect = false;
        }
      }
    }

    // Update active drawing
    if (isDrawingLine || isDrawingRect) {
      float mouseX = static_cast<float>(sf::Mouse::getPosition(window).x);
      float mouseY = static_cast<float>(sf::Mouse::getPosition(window).y);
      if (isDrawingLine) {
        currentLine.endCandle =
            viewStart + ((mouseX - marginX) / chartWidth) * viewWidth;
        currentLine.endPrice =
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)
                ? currentLine.startPrice
                : viewMaxPrice - ((mouseY - chartTop) / chartHeight) *
                                     (viewMaxPrice - viewMinPrice);
      } else if (isDrawingRect) {
        currentRect.endCandle =
            viewStart + ((mouseX - marginX) / chartWidth) * viewWidth;
        currentRect.endPrice =
            viewMaxPrice -
            ((mouseY - chartTop) / chartHeight) * (viewMaxPrice - viewMinPrice);
      }
    }

    // Update view min/max prices if view changed
    if (viewChanged) {
      currentViewStart = viewStart;
      currentViewEnd = viewEnd;
      viewMinPrice = candles[static_cast<size_t>(viewStart)].low;
      viewMaxPrice = candles[static_cast<size_t>(viewStart)].high;
      for (size_t i = static_cast<size_t>(viewStart);
           i <= static_cast<size_t>(viewEnd); ++i) {
        viewMinPrice = std::min(viewMinPrice, candles[i].low);
        viewMaxPrice = std::max(viewMaxPrice, candles[i].high);
      }
      viewChanged = false;
    }

    // Calculate candle dimensions
    float totalWidthPerCandle = chartWidth / viewWidth;
    float candleWidth = totalWidthPerCandle * CANDLE_WIDTH_FACTOR;
    float spacing = totalWidthPerCandle * SPACING_FACTOR;

    // Hover detection (includes wicks)
    sf::Vector2f mousePosF =
        static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
    showModal = false;
    for (size_t i = static_cast<size_t>(viewStart);
         i <= static_cast<size_t>(viewEnd); ++i) {
      const auto &candle = candles[i];
      float x = marginX + ((i - viewStart) * (candleWidth + spacing));
      float highY = chartTop + ((viewMaxPrice - candle.high) /
                                (viewMaxPrice - viewMinPrice) * chartHeight);
      float lowY = chartTop + ((viewMaxPrice - candle.low) /
                               (viewMaxPrice - viewMinPrice) * chartHeight);
      sf::FloatRect candleRect({x, highY}, {candleWidth, lowY - highY});
      if (candleRect.contains(mousePosF)) {
        std::stringstream ss;
        ss << "Date:   " << candle.date << "\n"
           << "Open:   " << std::fixed << std::setprecision(2) << candle.open << "\n"
           << "High:   " << std::fixed << std::setprecision(2) << candle.high << "\n"
           << "Low:    " << std::fixed << std::setprecision(2) << candle.low << "\n"
           << "Close:  " << std::fixed << std::setprecision(2) << candle.close << "\n"
           << "Volume: " << candle.volume;
        modalText = sf::Text(font, ss.str(), 12);
        modalText.setFillColor(sf::Color::Black);
        sf::FloatRect bounds = modalText.getLocalBounds();
        float padding = 5.f;
        modalRect = sf::RectangleShape(
            {bounds.size.x + 2 * padding, bounds.size.y + 2 * padding});
        modalRect.setFillColor(MODAL_FILL);
        modalRect.setOutlineColor(sf::Color::Black);
        modalRect.setOutlineThickness(1);
        sf::Vector2f pos = mousePosF + sf::Vector2f(10.f, 10.f);
        if (pos.x + modalRect.getSize().x > width)
          pos.x = width - modalRect.getSize().x;
        if (pos.y + modalRect.getSize().y > height)
          pos.y = height - modalRect.getSize().y;
        if (pos.x < 0)
          pos.x = 0;
        if (pos.y < 0)
          pos.y = 0;
        modalRect.setPosition(pos);
        modalText.setPosition(pos + sf::Vector2f(padding, padding));
        showModal = true;
        break;
      }
    }

    // Drawing
    window.clear(sf::Color::White);

    drawGridLines(window, viewStart, viewEnd, marginX, chartTop, chartWidth,
                  chartHeight, candleWidth, spacing, viewMinPrice,
                  viewMaxPrice);
    drawCandlesticks(window, candles, viewStart, viewEnd, marginX, chartTop,
                     chartHeight, candleWidth, spacing, viewMinPrice,
                     viewMaxPrice);
    drawDateLabels(window, candles, viewStart, viewEnd, marginX, chartBottom,
                   chartWidth, candleWidth, spacing, font);
    drawLines(window, lines, viewStart, viewEnd, marginX, chartTop, chartWidth,
              chartHeight, viewMinPrice, viewMaxPrice);
    drawRectangles(window, rects, viewStart, viewEnd, marginX, chartTop,
                   chartWidth, chartHeight, viewMinPrice, viewMaxPrice);
    drawTexts(window, texts, viewStart, viewEnd, marginX, chartTop, chartWidth,
              chartHeight, viewMinPrice, viewMaxPrice, font);

    if (isDrawingLine) {
      float startX =
          marginX +
          ((currentLine.startCandle - viewStart) / viewWidth) * chartWidth;
      float startY = chartTop + ((viewMaxPrice - currentLine.startPrice) /
                                 (viewMaxPrice - viewMinPrice) * chartHeight);
      float endX = marginX + ((currentLine.endCandle - viewStart) / viewWidth) *
                                 chartWidth;
      float endY = chartTop + ((viewMaxPrice - currentLine.endPrice) /
                               (viewMaxPrice - viewMinPrice) * chartHeight);
      sf::Vertex activeLine[] = {{{startX, startY}, sf::Color::Blue},
                                 {{endX, endY}, sf::Color::Blue}};
      window.draw(activeLine, 2, sf::PrimitiveType::Lines);
    } else if (isDrawingRect) {
      float startX =
          marginX +
          ((currentRect.startCandle - viewStart) / viewWidth) * chartWidth;
      float startY = chartTop + ((viewMaxPrice - currentRect.startPrice) /
                                 (viewMaxPrice - viewMinPrice) * chartHeight);
      float endX = marginX + ((currentRect.endCandle - viewStart) / viewWidth) *
                                 chartWidth;
      float endY = chartTop + ((viewMaxPrice - currentRect.endPrice) /
                               (viewMaxPrice - viewMinPrice) * chartHeight);
      sf::RectangleShape activeRect;
      activeRect.setPosition({std::min(startX, endX), std::min(startY, endY)});
      activeRect.setSize({std::abs(endX - startX), std::abs(endY - startY)});
      activeRect.setFillColor(sf::Color(0, 0, 0, 51));
      activeRect.setOutlineColor(sf::Color::Black);
      activeRect.setOutlineThickness(1);
      window.draw(activeRect);
    }

    if (isTyping) {
      float x =
          marginX + ((currentText.candle - viewStart) / viewWidth) * chartWidth;
      float y = chartTop + ((viewMaxPrice - currentText.price) /
                            (viewMaxPrice - viewMinPrice) * chartHeight);
      sf::Text textObj(font, inputBuffer + "_", 16);
      textObj.setFillColor(sf::Color::Black);
      textObj.setPosition({x, y});
      window.draw(textObj);
    }

    if (showModal) {
      window.draw(modalRect);
      window.draw(modalText);
    }

    window.display();
  }
}
