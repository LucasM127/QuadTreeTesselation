#ifndef SFML_CAMERA_HPP
#define SFML_CAMERA_HPP

#include <SFML/Graphics.hpp>

class Camera
{
public:
    Camera(const sf::Vector2u &winSize, const sf::View &view);
    void handleEvent(const sf::Event &event);
    void update(float dt);
    
    const sf::View &getView() const;
private:
    sf::View m_view;

    sf::Vector2u m_winSize;
    sf::Vector2f m_viewPos;
    sf::Vector2f m_viewSize;
    sf::Vector2i m_mouseScreenPos, m_lastScreenMousePos;

    sf::Vector2f screenToWorld(const sf::Vector2i &screenPos);

    void zoom(float f);
    void scale(float f);
    void resize(unsigned int newWidth, unsigned int newHeight);
    void pan();

    void updateView();
};//the window?

#endif //SFML_CAMERA_HPP