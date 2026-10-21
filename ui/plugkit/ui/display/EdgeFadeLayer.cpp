// Copied from eq1's plugin/display/EdgeFadeLayer.cpp (eq1 at dbb1127), in namespace plugkit::ui::display, the four
// fade distances an EdgeFades argument rather than eq1's tokens::layout::fade*.

#include "EdgeFadeLayer.h"

#include <cmath>

namespace plugkit::ui::display
{

EdgeFadeOverlay edgeFadeOverlay (const DisplayGeometry& geometry, const EdgeFades& fades, float scale, const std::function<void (juce::Graphics&)>& paintBehind)
{
    EdgeFadeOverlay overlay;
    overlay.scale = scale;
    const auto w = static_cast<float> (geometry.width), h = static_cast<float> (geometry.height);
    // The display and each fade's inner edge in device pixels, widened to whole pixels.
    const int width = static_cast<int> (std::ceil (w * scale)), height = static_cast<int> (std::ceil (h * scale));
    if (width <= 0 || height <= 0)
        return overlay;
    const int top = std::min (height, static_cast<int> (std::ceil (fades.top * scale)));
    const int bottom = std::max (top, static_cast<int> (std::floor ((h - fades.bottom) * scale)));
    const int left = std::min (width, static_cast<int> (std::ceil (fades.left * scale)));
    const int right = std::max (left, static_cast<int> (std::floor ((w - fades.right) * scale)));

    // How much of the display shows at a distance along one axis of extent: 0 at either edge, 1 past
    // both fades.
    const auto shown = [] (float at, float extent, float fadeIn, float fadeOut) {
        return juce::jlimit (0.0f, 1.0f, at / fadeIn) * juce::jlimit (0.0f, 1.0f, (extent - at) / fadeOut);
    };
    const auto strip = [&] (juce::Rectangle<int> area) -> EdgeFadeOverlay::Strip {
        if (area.isEmpty())
            return {};
        juce::Image image (juce::Image::ARGB, area.getWidth(), area.getHeight(), true);
        {
            juce::Graphics g (image);
            g.addTransform (juce::AffineTransform::scale (scale).translated (static_cast<float> (-area.getX()), static_cast<float> (-area.getY())));
            paintBehind (g);
        }
        const juce::Image::BitmapData pixels (image, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < area.getHeight(); ++y)
        {
            const float down = shown ((static_cast<float> (area.getY() + y) + 0.5f) / scale, h, fades.top, fades.bottom);
            for (int x = 0; x < area.getWidth(); ++x)
            {
                const float across = shown ((static_cast<float> (area.getX() + x) + 0.5f) / scale, w, fades.left, fades.right);
                reinterpret_cast<juce::PixelARGB*> (pixels.getPixelPointer (x, y))->multiplyAlpha (1.0f - down * across);
            }
        }
        return { image, area.getPosition() };
    };
    overlay.strips = { strip ({ 0, 0, width, top }),
                       strip ({ 0, bottom, width, height - bottom }),
                       strip ({ 0, top, left, bottom - top }),
                       strip ({ right, top, width - right, bottom - top }) };
    return overlay;
}

void paintEdgeFades (juce::Graphics& g, const EdgeFadeOverlay& overlay)
{
    for (const auto& strip : overlay.strips)
        if (strip.image.isValid())
            g.drawImageTransformed (strip.image, juce::AffineTransform::translation (strip.at.toFloat()).scaled (1.0f / overlay.scale));
}

} // namespace plugkit::ui::display
