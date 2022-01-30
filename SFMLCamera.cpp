#include "SFMLCamera.hpp"

Camera::Camera(const sf::Vector2u &winSize, const sf::View &view) : m_winSize(winSize)
{
    m_view = view;
    m_viewSize = m_view.getSize();
    m_viewPos = m_view.getCenter() - m_viewSize/2.f;
}

const sf::View &Camera::getView() const
{
    return m_view;
}

void Camera::updateView()
{
    m_view.setCenter(m_viewPos + m_viewSize/2.f);
    m_view.setSize(m_viewSize);
}

void Camera::handleEvent(const sf::Event &event)
{
    switch (event.type)
    {
    case sf::Event::MouseWheelScrolled:
        if(event.mouseWheelScroll.delta > 0)
            zoom(0.8);
        else
            zoom(1.25f);
        break;
    case sf::Event::MouseMoved:
        m_mouseScreenPos.x = event.mouseMove.x;
        m_mouseScreenPos.y = event.mouseMove.y;
        if(sf::Mouse::isButtonPressed(sf::Mouse::Left))
            pan();
        m_lastScreenMousePos = m_mouseScreenPos;
        break;
    case sf::Event::Resized:
        resize(event.size.width, event.size.height);
        break;
    default:
        break;
    }
}

void Camera::update(float dt){}

sf::Vector2f Camera::screenToWorld(const sf::Vector2i &screenPos)
{
    sf::Vector2f relPos;
    relPos.x = (float)screenPos.x / (float)m_winSize.x;
    relPos.y = (float)screenPos.y / (float)m_winSize.y;

    sf::Vector2f worldPos;
    worldPos.x = (relPos.x * m_viewSize.x) + m_viewPos.x;
    worldPos.y = (relPos.y * m_viewSize.y) + m_viewPos.y;

    return worldPos;
}

void Camera::zoom(float f)
{
    sf::Vector2f oldMousePos = screenToWorld(m_mouseScreenPos);

    sf::Vector2f newSize = m_viewSize * f;
    m_viewSize = newSize;

    sf::Vector2f updatedMousePos = screenToWorld(m_mouseScreenPos);
    
    sf::Vector2f offset = oldMousePos - updatedMousePos;
    m_viewPos += offset;

    updateView();
}

void Camera::scale(float f)
{
    sf::Vector2f oldSz = m_viewSize;
    m_viewSize *= f;
    sf::Vector2f offset = (m_viewSize - oldSz)/2.f;
    m_viewPos -= offset;
}

//keep center in center
void Camera::resize(unsigned int newWidth, unsigned int newHeight)
{
    float widthRatio = (float)newWidth/(float)m_winSize.x;
    float heightRatio = (float)newHeight/(float)m_winSize.y;

    float x_offset = ((float)newWidth  - (float)m_winSize.x) / 2.f;
    float y_offset = ((float)newHeight - (float)m_winSize.y) / 2.f;
    x_offset *= m_viewSize.x / (float)m_winSize.x;
    y_offset *= m_viewSize.y / (float)m_winSize.y;

    m_winSize.x = newWidth;
    m_winSize.y = newHeight;

    m_viewSize.x *= widthRatio;
    m_viewSize.y *= heightRatio;

    m_viewPos.x -= x_offset;
    m_viewPos.y -= y_offset;

    updateView();
}

void Camera::pan()
{
    sf::Vector2f pos = screenToWorld(m_mouseScreenPos);
    sf::Vector2f lastPos = screenToWorld(m_lastScreenMousePos);
    sf::Vector2f offset = pos - lastPos;
    m_viewPos -= offset;

    updateView();
}