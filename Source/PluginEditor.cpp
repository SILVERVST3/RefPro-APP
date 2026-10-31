#include "PluginEditor.h"

using juce::Colour;
using juce::Rectangle;

//==============================================================================
//  Ícones
//==============================================================================
namespace Icons
{
    static void mic (juce::Graphics& g, Rectangle<float> r, Colour c, float th)
    {
        g.setColour (c);
        const auto cx = r.getCentreX(), w = r.getWidth() * 0.32f;
        g.drawRoundedRectangle (cx - w * 0.5f, r.getY() + r.getHeight() * 0.05f, w, r.getHeight() * 0.50f, w * 0.5f, th);
        juce::Path stand;
        stand.startNewSubPath (cx - r.getWidth() * 0.30f, r.getY() + r.getHeight() * 0.42f);
        stand.quadraticTo (cx - r.getWidth() * 0.30f, r.getY() + r.getHeight() * 0.80f, cx, r.getY() + r.getHeight() * 0.80f);
        stand.quadraticTo (cx + r.getWidth() * 0.30f, r.getY() + r.getHeight() * 0.80f, cx + r.getWidth() * 0.30f, r.getY() + r.getHeight() * 0.42f);
        g.strokePath (stand, juce::PathStrokeType (th));
        g.drawLine (cx, r.getY() + r.getHeight() * 0.80f, cx, r.getBottom() - 1.0f, th);
    }

    static void gauge (juce::Graphics& g, Rectangle<float> r, Colour c, float th)
    {
        g.setColour (c);
        const auto cx = r.getCentreX(), cy = r.getCentreY() + r.getHeight() * 0.12f;
        juce::Path p;
        p.addCentredArc (cx, cy, r.getWidth() * 0.46f, r.getHeight() * 0.46f, 0.0f, -2.2f, 2.2f, true);
        g.strokePath (p, juce::PathStrokeType (th));
        g.drawLine (cx, cy, cx + r.getWidth() * 0.24f, cy - r.getHeight() * 0.22f, th);
        g.fillEllipse (cx - th, cy - th, th * 2.0f, th * 2.0f);
    }

    static void stereo (juce::Graphics& g, Rectangle<float> r, Colour c, float th)
    {
        g.setColour (c);
        const float d = r.getHeight() * 0.62f;
        g.drawEllipse (r.getCentreX() - d * 0.85f, r.getCentreY() - d * 0.5f, d, d, th);
        g.drawEllipse (r.getCentreX() - d * 0.15f, r.getCentreY() - d * 0.5f, d, d, th);
    }

    static void arrow (juce::Graphics& g, Rectangle<float> r, bool up, Colour c, float th)
    {
        g.setColour (c);
        const float cx = r.getCentreX(), t = r.getY() + 2.0f, b = r.getBottom() - 2.0f;
        g.drawLine (cx, t, cx, b, th);
        const float tip = up ? t : b, dir = up ? 1.0f : -1.0f, h = r.getHeight() * 0.22f;
        g.drawLine (cx, tip, cx - h, tip + dir * h, th);
        g.drawLine (cx, tip, cx + h, tip + dir * h, th);
    }

    static void bars (juce::Graphics& g, Rectangle<float> r, bool dense, Colour c, float th)
    {
        g.setColour (c);
        const int n = 5;
        for (int i = 0; i < n; ++i)
        {
            const float x = r.getX() + r.getWidth() * (0.1f + 0.8f * (float) i / (float) (n - 1));
            const float hgt = dense ? r.getHeight() * 0.50f : r.getHeight() * (i % 2 == 0 ? 0.28f : 0.72f);
            g.drawLine (x, r.getCentreY() - hgt * 0.5f, x, r.getCentreY() + hgt * 0.5f, th);
        }
    }

    static void circles (juce::Graphics& g, Rectangle<float> r, int count, Colour c, float th)
    {
        g.setColour (c);
        const float d = r.getHeight() * 0.50f;
        if (count == 1)
            g.drawEllipse (r.getCentreX() - d * 0.5f, r.getCentreY() - d * 0.5f, d, d, th);
        else
        {
            g.drawEllipse (r.getCentreX() - d - 1.0f, r.getCentreY() - d * 0.5f, d, d, th);
            g.drawEllipse (r.getCentreX() + 1.0f,     r.getCentreY() - d * 0.5f, d, d, th);
        }
    }

    static void gear (juce::Graphics& g, Rectangle<float> r, Colour c, float th)
    {
        g.setColour (c);
        const auto cx = r.getCentreX(), cy = r.getCentreY();
        const float R = r.getHeight() * 0.30f;
        g.drawEllipse (cx - R, cy - R, R * 2.0f, R * 2.0f, th);
        g.drawEllipse (cx - R * 0.38f, cy - R * 0.38f, R * 0.76f, R * 0.76f, th);
        for (int i = 0; i < 8; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 8.0f;
            g.drawLine (cx + std::cos (a) * R, cy + std::sin (a) * R,
                        cx + std::cos (a) * R * 1.35f, cy + std::sin (a) * R * 1.35f, th * 1.6f);
        }
    }

    static void curveIcon (juce::Graphics& g, Rectangle<float> r, Colour c, float th)
    {
        g.setColour (c);
        juce::Path p;
        p.startNewSubPath (r.getX() + r.getWidth() * 0.12f, r.getY() + r.getHeight() * 0.35f);
        p.cubicTo (r.getX() + r.getWidth() * 0.40f, r.getY() + r.getHeight() * 0.20f,
                   r.getX() + r.getWidth() * 0.55f, r.getY() + r.getHeight() * 0.80f,
                   r.getX() + r.getWidth() * 0.88f, r.getY() + r.getHeight() * 0.70f);
        g.strokePath (p, juce::PathStrokeType (th * 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void levelIcon (juce::Graphics& g, Rectangle<float> r, Colour c, float th)
    {
        g.setColour (c);
        const float y = r.getCentreY();
        g.drawLine (r.getX() + r.getWidth() * 0.14f, y, r.getRight() - r.getWidth() * 0.14f, y, th * 1.3f);
        juce::Path p;
        p.startNewSubPath (r.getX() + r.getWidth() * 0.14f, y + r.getHeight() * 0.2f);
        p.cubicTo (r.getX() + r.getWidth() * 0.35f, y - r.getHeight() * 0.3f, r.getX() + r.getWidth() * 0.6f, y + r.getHeight() * 0.3f,
                   r.getRight() - r.getWidth() * 0.14f, y - r.getHeight() * 0.18f);
        g.strokePath (p, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static void barsIcon (juce::Graphics& g, Rectangle<float> r, Colour c)
    {
        g.setColour (c);
        const float hs[] = { 0.65f, 0.9f, 0.55f, 0.35f };
        for (int i = 0; i < 4; ++i)
        {
            const float w = r.getWidth() * 0.14f;
            const float x = r.getX() + r.getWidth() * (0.16f + 0.19f * (float) i);
            const float h = r.getHeight() * 0.6f * hs[i];
            g.fillRoundedRectangle (x, r.getCentreY() + r.getHeight() * 0.3f - h, w, h, 1.5f);
        }
    }
}

static void fillPanel (juce::Graphics& g, Rectangle<float> r, float radius)
{
    g.setGradientFill (juce::ColourGradient (Theme::panelHi, 0.0f, r.getY(), Theme::panel.darker (0.06f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
}

//==============================================================================
//  TargetList
//==============================================================================
static void drawHeart (juce::Graphics& g, Rectangle<float> r, bool filled, Colour c)
{
    juce::Path h;
    const float x = r.getX(), y = r.getY(), w = r.getWidth(), hh = r.getHeight();
    h.startNewSubPath (x + w * 0.5f, y + hh * 0.92f);
    h.cubicTo (x - w * 0.15f, y + hh * 0.45f, x + w * 0.10f, y - hh * 0.10f, x + w * 0.5f, y + hh * 0.30f);
    h.cubicTo (x + w * 0.90f, y - hh * 0.10f, x + w * 1.15f, y + hh * 0.45f, x + w * 0.5f, y + hh * 0.92f);
    h.closeSubPath();
    g.setColour (c);
    if (filled) g.fillPath (h); else g.strokePath (h, juce::PathStrokeType (1.3f));
}

void TargetList::paint (juce::Graphics& g)
{
    const float rowH = rowBase * s;
    for (size_t i = 0; i < items.size(); ++i)
    {
        const auto& it = items[i];
        auto row = Rectangle<float> (0.0f, (float) i * rowH, (float) getWidth(), rowH);
        const bool sel = it.index == selected;
        if (sel)
        {
            g.setGradientFill (juce::ColourGradient (Theme::tealDark, row.getX(), 0.0f, Theme::panelHi, row.getRight(), 0.0f, false));
            g.fillRoundedRectangle (row.reduced (2.0f * s, 2.0f * s), 6.0f * s);
        }
        g.setColour (sel ? Theme::text : Theme::text.withAlpha (0.85f));
        g.setFont (mkFont (15.0f * s, sel));
        g.drawText (it.name, row.withTrimmedLeft (12.0f * s).withTrimmedRight (34.0f * s), juce::Justification::centredLeft);
        const auto ic = Rectangle<float> (row.getRight() - 28.0f * s, row.getCentreY() - 6.5f * s, 14.0f * s, 13.0f * s);
        drawHeart (g, ic, it.favorite, it.favorite ? Theme::teal : Theme::dim.withAlpha (0.7f));
    }
}

void TargetList::mouseDown (const juce::MouseEvent& e)
{
    const int row = (int) (e.position.y / (rowBase * s));
    if (row < 0 || row >= (int) items.size()) return;
    const auto& it = items[(size_t) row];
    if (e.mods.isPopupMenu()) { if (onContext) onContext (it); }
    else if (e.position.x > (float) getWidth() - 36.0f * s) { if (onFavorite) onFavorite (it); }
    else if (onSelect) onSelect (it.index);
}

//==============================================================================
//  Helpers de parâmetros (com gestos para automação do host)
//==============================================================================
static void setParam (juce::AudioProcessorValueTreeState& a, const juce::String& id, float plain)
{
    if (auto* p = a.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (plain));
}
static void gesture (juce::AudioProcessorValueTreeState& a, const juce::String& id, bool begin)
{
    if (auto* p = a.getParameter (id)) { if (begin) p->beginChangeGesture(); else p->endChangeGesture(); }
}
static void setParamG (juce::AudioProcessorValueTreeState& a, const juce::String& id, float plain)
{
    gesture (a, id, true); setParam (a, id, plain); gesture (a, id, false);
}
static void setBandType (juce::AudioProcessorValueTreeState& a, int band, int type)
{
    setParamG (a, RefProProcessor::bandId (band, "type"), (float) type);
    if (type == Eq::LowCut || type == Eq::HighCut) setParamG (a, RefProProcessor::bandId (band, "q"), 0.71f);
    if (! Eq::typeHasGain (type))                  setParamG (a, RefProProcessor::bandId (band, "gain"), 0.0f);
}
static void removeBand (juce::AudioProcessorValueTreeState& a, int band)
{
    setParamG (a, RefProProcessor::bandId (band, "on"), 0.0f);
}

//==============================================================================
//  GraphView
//==============================================================================
static juce::String fmtFreq (float f)
{
    return f >= 1000.0f ? juce::String (f / 1000.0f, 2) + " kHz" : juce::String ((int) std::round (f)) + " Hz";
}


juce::Rectangle<float> GraphView::plotRect() const
{
    return getLocalBounds().toFloat().withTrimmedTop (48.0f * s).withTrimmedBottom (26.0f * s).reduced (1.0f, 0.0f);
}
float GraphView::xOf (float f) const
{
    const auto r = plotRect();
    return r.getX() + r.getWidth() * juce::jlimit (0.0f, 1.02f, rp::freqToX01 (f));
}
float GraphView::yOf (float db) const
{
    const auto r = plotRect();
    return r.getY() + r.getHeight() * (topDb() - db) / (topDb() - bottomDb());
}
float GraphView::fOf (float x) const
{
    const auto r = plotRect();
    return rp::fMin * std::pow (10.0f, 3.0f * juce::jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth()));
}
float GraphView::dbOf (float y) const
{
    const auto r = plotRect();
    return topDb() - (topDb() - bottomDb()) * (y - r.getY()) / r.getHeight();
}

juce::Point<float> GraphView::getNodePosition (int band) const
{
    const auto prm = proc.getBandParams (band);
    const auto r = plotRect();
    const float y = Eq::typeHasGain (prm.type) ? yOf (prm.gain) : yOf (0.0f);
    return { xOf (prm.freq), juce::jlimit (r.getY() + 4.0f, r.getBottom() - 4.0f, y) };
}

int GraphView::hitNode (juce::Point<float> p) const
{
    int best = -1; float bd = 14.0f * s;
    for (int b = 0; b < Eq::numBands; ++b)
    {
        if (! proc.getBandParams (b).on) continue;
        const float d = p.getDistanceFrom (getNodePosition (b));
        if (d < bd) { bd = d; best = b; }
    }
    return best;
}

void GraphView::select (int b)
{
    selected = b;
    if (onSelect) onSelect (b);
    repaint();
}

void GraphView::mouseDown (const juce::MouseEvent& e)
{
    const int n = hitNode (e.position);

    if (e.mods.isPopupMenu())
    {
        if (n < 0) return;
        select (n);
        const auto prm = proc.getBandParams (n);
        const auto names = Eq::typeNames();
        juce::PopupMenu m;
        for (int i = 0; i < names.size(); ++i) m.addItem (i + 1, names[i], true, i == prm.type);
        m.addSeparator();
        m.addItem (100, "Delete band");
        m.showMenuAsync (juce::PopupMenu::Options(), [this, n] (int r)
        {
            if (r >= 1 && r <= Eq::numTypes) setBandType (proc.apvts, n, r - 1);
            else if (r == 100) { removeBand (proc.apvts, n); select (-1); }
            if (onNodeMoved) onNodeMoved();
        });
        return;
    }

    if (n >= 0)
    {
        select (n);
        dragBand = n;
        gesture (proc.apvts, RefProProcessor::bandId (n, "freq"), true);
        gesture (proc.apvts, RefProProcessor::bandId (n, "gain"), true);
    }
    else
        select (-1);
}

void GraphView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragBand < 0) return;
    const auto prm = proc.getBandParams (dragBand);
    setParam (proc.apvts, RefProProcessor::bandId (dragBand, "freq"), juce::jlimit (20.0f, 20000.0f, fOf (e.position.x)));
    if (Eq::typeHasGain (prm.type))
        setParam (proc.apvts, RefProProcessor::bandId (dragBand, "gain"), juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)));
    if (onNodeMoved) onNodeMoved();
}

void GraphView::mouseUp (const juce::MouseEvent&)
{
    if (dragBand < 0) return;
    gesture (proc.apvts, RefProProcessor::bandId (dragBand, "freq"), false);
    gesture (proc.apvts, RefProProcessor::bandId (dragBand, "gain"), false);
    dragBand = -1;
}

void GraphView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int n = hitNode (e.position);
    if (n >= 0) { removeBand (proc.apvts, n); select (-1); return; }
    if (! plotRect().contains (e.position)) return;

    for (int b = 0; b < Eq::numBands; ++b)
        if (! proc.getBandParams (b).on)
        {
            setParamG (proc.apvts, RefProProcessor::bandId (b, "type"), (float) Eq::Bell);
            setParamG (proc.apvts, RefProProcessor::bandId (b, "freq"), fOf (e.position.x));
            setParamG (proc.apvts, RefProProcessor::bandId (b, "gain"), juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)));
            setParamG (proc.apvts, RefProProcessor::bandId (b, "q"), 1.0f);
            setParamG (proc.apvts, RefProProcessor::bandId (b, "on"), 1.0f);
            select (b);
            return;
        }
}

void GraphView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int n = hitNode (e.position);
    if (n < 0) return;
    const auto prm = proc.getBandParams (n);
    setParamG (proc.apvts, RefProProcessor::bandId (n, "q"), juce::jlimit (0.1f, 10.0f, prm.q * std::exp (w.deltaY * 0.6f)));
    if (onNodeMoved) onNodeMoved();
}

void GraphView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    fillPanel (g, b, 10.0f * s);
    g.setColour (Theme::edge);
    g.drawRoundedRectangle (b.reduced (0.5f), 10.0f * s, 1.0f);

    const auto plot = b.withTrimmedTop (48.0f * s).withTrimmedBottom (26.0f * s).reduced (1.0f, 0.0f);
    auto X = [&] (float f) { return plot.getX() + plot.getWidth() * juce::jlimit (0.0f, 1.02f, rp::freqToX01 (f)); };
    auto Y = [&] (float db) { return plot.getY() + plot.getHeight() * (topDb() - db) / (topDb() - bottomDb()); };
    auto cen  = [&] (size_t i) { return leveled ? 0.0f : target.centre[i]; };            // Leveled: o alvo vira a linha 0 dB
    auto offs = [&] (size_t i) { return leveled ? target.centre[i] : 0.0f; };

    // ---- grade ----
    g.setFont (mkFont (12.0f * s));
    for (int db = leveled ? 10 : 8; db >= (leveled ? -10 : -12); db -= (leveled ? 5 : 4))
    {
        const float y = Y ((float) db);
        g.setColour (db == 0 ? Theme::teal.withAlpha (0.35f) : Theme::edge.withAlpha (0.55f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (Theme::dim);
        g.drawText (juce::String (db), Rectangle<float> (plot.getRight() - 46.0f * s, y - 8.0f * s, 38.0f * s, 16.0f * s),
                    juce::Justification::centredRight);
    }
    g.setColour (Theme::dim);
    g.drawText ("dB", Rectangle<float> (plot.getRight() - 46.0f * s, b.getY() + 8.0f * s, 38.0f * s, 16.0f * s), juce::Justification::centredRight);

    struct FL { float f; const char* t; };
    static const FL fl[] = { {20,"20"},{50,"50"},{100,"100"},{200,"200"},{500,"500"},{1000,"1k"},{2000,"2k"},{5000,"5k"},{10000,"10k"} };
    for (auto& x : fl)
    {
        g.setColour (Theme::edge.withAlpha (0.35f));
        g.drawVerticalLine ((int) X (x.f), plot.getY(), plot.getBottom());
        g.setColour (Theme::dim);
        const float cx = juce::jlimit (b.getX() + 14.0f * s, b.getRight() - 40.0f * s, X (x.f));
        g.drawText (x.t, Rectangle<float> (cx - 20.0f * s, plot.getBottom() + 4.0f * s, 40.0f * s, 16.0f * s), juce::Justification::centred);
    }
    g.drawText ("Hz", Rectangle<float> (plot.getRight() - 40.0f * s, plot.getBottom() + 4.0f * s, 34.0f * s, 16.0f * s), juce::Justification::centredRight);

    // ---- regiões ----
    static const float xo[] = { 200.0f, 1800.0f, 7000.0f };
    static const float lim[] = { 20.0f, 200.0f, 1800.0f, 7000.0f, 24000.0f };
    static const char* names[] = { "Low", "Low-Mid", "High-Mid", "High" };
    for (float f : xo)
    {
        g.setColour (Theme::edge.brighter (0.25f));
        g.drawVerticalLine ((int) X (f), b.getY() + 6.0f * s, plot.getBottom());
    }

    const bool has = an.hasData.load();
    for (int r = 0; r < 4; ++r)
    {
        const float x0 = X (lim[r]), x1 = std::min (X (lim[r + 1]), plot.getRight());
        g.setColour (Theme::text.withAlpha (0.9f));
        g.setFont (mkFont (13.0f * s));
        g.drawText (names[r], Rectangle<float> (x0, b.getY() + 8.0f * s, x1 - x0, 18.0f * s), juce::Justification::centred);

        if (has)
        {
            float sum = 0.0f; int cnt = 0;
            for (int i = 0; i < rp::numBands; ++i)
            {
                const float f = rp::bandFreq (i);
                if (f >= lim[r] && f < lim[r + 1])
                { sum += an.avgDb[(size_t) i].load() - target.centre[(size_t) i]; ++cnt; }
            }
            if (cnt > 0)
            {
                const float dev = sum / (float) cnt;
                g.setColour (std::abs (dev) <= 1.5f ? Theme::teal : Theme::amber);
                g.setFont (mkFont (12.0f * s));
                g.drawText ((dev >= 0 ? "+" : "") + juce::String (dev, 1) + " dB",
                            Rectangle<float> (x0, b.getY() + 26.0f * s, x1 - x0, 16.0f * s), juce::Justification::centred);
            }
        }
    }

    // ---- curvas (recortadas à área de plotagem) ----
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (plot.toNearestInt());

        // faixa-alvo
        juce::Path band, mid;
        for (int i = 0; i < rp::numBands; ++i)
        {
            const float x = X (rp::bandFreq (i));
            const float y = Y (cen ((size_t) i) + target.tol[(size_t) i]);
            i == 0 ? band.startNewSubPath (x, y) : band.lineTo (x, y);
        }
        for (int i = rp::numBands - 1; i >= 0; --i)
            band.lineTo (X (rp::bandFreq (i)), Y (cen ((size_t) i) - target.tol[(size_t) i]));
        band.closeSubPath();
        for (int i = 0; i < rp::numBands; ++i)
        {
            const float x = X (rp::bandFreq (i)), y = Y (cen ((size_t) i));
            i == 0 ? mid.startNewSubPath (x, y) : mid.lineTo (x, y);
        }
        g.setGradientFill (juce::ColourGradient (Theme::teal.withAlpha (0.46f), 0.0f, plot.getY(),
                                                 Theme::teal.withAlpha (0.14f), 0.0f, plot.getBottom(), false));
        g.fillPath (band);
        g.setColour (Theme::teal.withAlpha (0.55f));
        g.strokePath (band, juce::PathStrokeType (1.0f * s));
        g.setColour (Theme::teal.withAlpha (0.45f));
        g.strokePath (mid, juce::PathStrokeType (1.2f * s));

        if (has)
        {
            if (bars)
            {
                const float y0 = Y (0.0f);
                for (int i = 0; i + 2 < rp::numBands; i += 3)
                {
                    const float v = an.avgDb[(size_t) (i + 1)].load() - offs ((size_t) (i + 1));
                    const float c = cen ((size_t) (i + 1)), t = target.tol[(size_t) (i + 1)];
                    const float xa = X (rp::bandFreq (i) * 0.97f), xb = X (rp::bandFreq (i + 2) * 1.03f);
                    g.setColour ((std::abs (v - c) <= t ? Theme::teal : Theme::amber).withAlpha (0.75f));
                    g.fillRect (Rectangle<float> (xa + 0.5f, std::min (y0, Y (v)), xb - xa - 1.0f, std::abs (Y (v) - y0)));
                }
            }
            else
            {
                if (extra)
                {
                    juce::Path f;
                    for (int i = 0; i < rp::numBands; ++i)
                    {
                        const float x = X (rp::bandFreq (i)), y = Y (an.fastDb[(size_t) i].load() - offs ((size_t) i));
                        i == 0 ? f.startNewSubPath (x, y) : f.lineTo (x, y);
                    }
                    g.setColour (Theme::amber.withAlpha (0.55f));
                    g.strokePath (f, juce::PathStrokeType (1.2f * s));
                }

                juce::Path p;
                for (int i = 0; i < rp::numBands; ++i)
                {
                    const float x = X (rp::bandFreq (i)), y = Y (an.avgDb[(size_t) i].load() - offs ((size_t) i));
                    i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
                }
                g.setColour (Theme::curve.withAlpha (0.14f));
                g.strokePath (p, juce::PathStrokeType (6.0f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour (Theme::curve);
                g.strokePath (p, juce::PathStrokeType (2.2f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }

        // ---- curva do equalizador ----
        {
            const bool eqOn = proc.apvts.getRawParameterValue ("eqOn")->load() > 0.5f;
            const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
            std::array<Eq::Coeffs, Eq::numBands> cf {};
            bool any = false;
            for (int k = 0; k < Eq::numBands; ++k)
            {
                const auto prm = proc.getBandParams (k);
                if (Eq::isActive (prm)) { cf[(size_t) k] = Eq::make (prm, sr); any = true; }
                else cf[(size_t) k] = Eq::Coeffs {};
            }
            if (any)
            {
                juce::Path c, fill;
                const int pts = 240;
                for (int i = 0; i < pts; ++i)
                {
                    const float f = rp::fMin * std::pow (1000.0f, (float) i / (float) (pts - 1));
                    double db = 0.0;
                    for (int k = 0; k < Eq::numBands; ++k)
                        if (Eq::isActive (proc.getBandParams (k))) db += Eq::magnitudeDb (cf[(size_t) k], f, sr);
                    const float x = X (f), y = Y ((float) db);
                    i == 0 ? c.startNewSubPath (x, y) : c.lineTo (x, y);
                }
                fill = c;
                fill.lineTo (X (20000.0f), Y (0.0f));
                fill.lineTo (X (20.0f), Y (0.0f));
                fill.closeSubPath();
                const auto col = eqOn ? Theme::amber : Theme::dim;
                g.setColour (col.withAlpha (0.10f));
                g.fillPath (fill);
                g.setColour (col.withAlpha (eqOn ? 0.95f : 0.6f));
                g.strokePath (c, juce::PathStrokeType (1.7f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    }

    {
        juce::Graphics::ScopedSaveState ss2 (g);
        g.reduceClipRegion (plot.toNearestInt());

        // ---- EQ dinâmico: referência adaptada, curva do ganho e faixa por banda ----
        if (proc.apvts.getRawParameterValue ("dynOn")->load() > 0.5f)
        {
            const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
            std::array<Eq::Coeffs, RefProProcessor::dynBands> dc {};
            std::array<float, RefProProcessor::dynBands> gdb {};
            bool any = false;
            float worstCut = 0.0f, worstBoost = 0.0f;
            for (int j = 0; j < RefProProcessor::dynBands; ++j)
            {
                const float gj = proc.dynGainDb[(size_t) j].load();
                gdb[(size_t) j] = gj;
                worstCut = std::min (worstCut, gj);
                worstBoost = std::max (worstBoost, gj);
                if (std::abs (gj) > 0.05f)
                {
                    Eq::Params pp; pp.on = true; pp.type = Eq::Bell; pp.freq = RefProProcessor::dynFreq (j); pp.gain = gj; pp.q = 4.3f;
                    dc[(size_t) j] = Eq::make (pp, sr);
                    any = true;
                }
            }

            // referência adaptada (tracejada): alvo + deslocamento que o Adapt/âncora aplicam
            if (has)
            {
                juce::Path ref;
                bool moved = false;
                for (int j = 0; j < RefProProcessor::dynBands; ++j)
                {
                    const float sh = proc.dynRefShift[(size_t) j].load();
                    if (std::abs (sh) > 0.05f) moved = true;
                    const float x = X (RefProProcessor::dynFreq (j));
                    const float y = Y (cen ((size_t) juce::jmin (rp::numBands - 1, 4 * j)) + sh);
                    j == 0 ? ref.startNewSubPath (x, y) : ref.lineTo (x, y);
                }
                if (moved)
                {
                    juce::Path dashed;
                    const float dl[] = { 5.0f * s, 4.0f * s };
                    juce::PathStrokeType (1.3f * s).createDashedStroke (dashed, ref, dl, 2);
                    g.setColour (Theme::curve.withAlpha (0.55f));
                    g.fillPath (dashed);
                }
            }

            // faixa inferior: corte para baixo (laranja), boost para cima (azul-esverdeado); ±12 dB
            const float stripH = 54.0f * s;
            auto strip = juce::Rectangle<float> (plot.getX(), plot.getBottom() - stripH, plot.getWidth(), stripH);
            const float y0 = strip.getCentreY(), half = stripH * 0.5f - 3.0f * s;
            g.setColour (Theme::well.withAlpha (0.85f));
            g.fillRect (strip);
            g.setColour (Theme::edge.brighter (0.2f));
            g.drawHorizontalLine ((int) strip.getY(), strip.getX(), strip.getRight());
            g.setColour (Theme::edge.brighter (0.45f));
            g.drawHorizontalLine ((int) y0, strip.getX(), strip.getRight());
            g.setColour (Theme::dim);
            g.setFont (mkFont (11.0f * s));
            g.drawText ("Dynamic EQ (dB)", juce::Rectangle<float> (strip.getX() + 6.0f * s, strip.getY() + 2.0f * s, 120.0f * s, 14.0f * s), juce::Justification::centredLeft);
            for (int j = 0; j < RefProProcessor::dynBands; ++j)
            {
                const float fc = RefProProcessor::dynFreq (j), gn = gdb[(size_t) j];
                if (std::abs (gn) < 0.05f) continue;
                const float xa = X (fc * 0.8909f), xb = X (fc * 1.1225f);
                const float h = juce::jlimit (0.0f, 1.0f, std::abs (gn) / 12.0f) * half;
                const float bw = std::max (1.0f, xb - xa - 1.0f);
                if (gn < 0.0f)
                {
                    g.setColour (Theme::orange.withAlpha (0.85f));
                    g.fillRect (juce::Rectangle<float> (xa + 0.5f, y0, bw, h));
                }
                else
                {
                    g.setColour (Theme::teal.withAlpha (0.9f));
                    g.fillRect (juce::Rectangle<float> (xa + 0.5f, y0 - h, bw, h));
                }
            }
            const float comp = proc.dynCompOut.load();
            juce::String txt;
            if (worstCut < -0.05f)   txt << juce::String (worstCut, 1) << " dB  ";
            if (worstBoost > 0.05f)  txt << "+" << juce::String (worstBoost, 1) << " dB  ";
            if (std::abs (comp) > 0.05f) txt << "comp " << (comp > 0 ? "+" : "") << juce::String (comp, 1);
            g.setColour (Theme::text.withAlpha (0.85f));
            g.drawText (txt, juce::Rectangle<float> (strip.getRight() - 260.0f * s, strip.getY() + 2.0f * s, 252.0f * s, 14.0f * s), juce::Justification::centredRight);

            if (any)
            {
                juce::Path c;
                const int pts = 240;
                for (int i = 0; i < pts; ++i)
                {
                    const float f = rp::fMin * std::pow (1000.0f, (float) i / (float) (pts - 1));
                    double db = 0.0;
                    for (int j = 0; j < RefProProcessor::dynBands; ++j)
                        if (std::abs (gdb[(size_t) j]) > 0.05f) db += Eq::magnitudeDb (dc[(size_t) j], f, sr);
                    const float x = X (f), y = Y ((float) db);
                    i == 0 ? c.startNewSubPath (x, y) : c.lineTo (x, y);
                }
                g.setColour (Theme::orange.withAlpha (0.9f));
                g.strokePath (c, juce::PathStrokeType (1.5f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    }

    // ---- nós do equalizador ----
    {
        const bool eqOn = proc.apvts.getRawParameterValue ("eqOn")->load() > 0.5f;
        for (int k = 0; k < Eq::numBands; ++k)
        {
            if (! proc.getBandParams (k).on) continue;
            const auto pos = getNodePosition (k);
            const bool sel = (k == selected);
            const float r = (sel ? 7.5f : 6.0f) * s;
            if (sel)
            {
                g.setColour (Theme::teal.withAlpha (0.25f));
                g.fillEllipse (pos.x - r * 1.9f, pos.y - r * 1.9f, r * 3.8f, r * 3.8f);
            }
            const auto pr = proc.getBandParams (k);
            if (pr.dyn && Eq::typeHasGain (pr.type))
            {
                const float gy = juce::jlimit (plot.getY(), plot.getBottom(), yOf (proc.bandDynGain[(size_t) k].load()));
                g.setColour (Theme::orange.withAlpha (0.7f));
                g.drawLine (pos.x, pos.y, pos.x, gy, 1.5f * s);
                g.fillEllipse (pos.x - 3.5f * s, gy - 3.5f * s, 7.0f * s, 7.0f * s);
                g.drawEllipse (pos.x - r - 3.0f * s, pos.y - r - 3.0f * s, (r + 3.0f * s) * 2.0f, (r + 3.0f * s) * 2.0f, 1.4f * s);
            }
            g.setColour (Theme::well);
            g.fillEllipse (pos.x - r - 1.5f, pos.y - r - 1.5f, (r + 1.5f) * 2.0f, (r + 1.5f) * 2.0f);
            g.setColour (! eqOn ? Theme::dim : (sel ? Theme::text : Theme::teal));
            g.fillEllipse (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f);
            g.setColour (juce::Colours::white);
            g.setFont (mkFont (9.0f * s, true));
            g.drawText (juce::String (k + 1), juce::Rectangle<float> (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f), juce::Justification::centred);
        }
    }

    if (! has)
    {
        g.setColour (Theme::dim);
        g.setFont (mkFont (15.0f * s));
        g.drawText ("Play audio to see your tonal balance", plot.reduced (20.0f * s), juce::Justification::centred);
    }

    // ---- hover ----
    if (hoverX >= plot.getX() && hoverX <= plot.getRight() && has && dragBand < 0)
    {
        const float f = rp::fMin * std::pow (10.0f, 3.0f * (hoverX - plot.getX()) / plot.getWidth());
        const int idx = juce::jlimit (0, rp::numBands - 1, (int) std::round (12.0f * std::log2 (f / rp::fMin)));
        g.setColour (Theme::text.withAlpha (0.35f));
        g.drawVerticalLine ((int) hoverX, plot.getY(), plot.getBottom());

        const float me = an.avgDb[(size_t) idx].load(), tg = target.centre[(size_t) idx];
        const juce::String txt = fmtFreq (f) + "   You " + juce::String (me, 1) + "   Target " + juce::String (tg, 1) + " dB";
        const float w = 250.0f * s;
        auto box = Rectangle<float> (juce::jlimit (plot.getX() + 4.0f, plot.getRight() - w - 4.0f, hoverX + 10.0f * s),
                                     plot.getBottom() - 30.0f * s, w, 22.0f * s);
        g.setColour (Theme::well.withAlpha (0.95f));
        g.fillRoundedRectangle (box, 5.0f * s);
        g.setColour (Theme::text);
        g.setFont (mkFont (12.5f * s));
        g.drawText (txt, box, juce::Justification::centred);
    }
}

//==============================================================================
//  MetersPanel  (kind: 0 = Vocal Balance, 1 = Dynamics, 2 = Stereo Width)
//==============================================================================
void MetersPanel::drawMeter (juce::Graphics& g, Rectangle<float> col, int kind, float dev, bool valid, const juce::String& hint) const
{
    const Colour accent = kind == 0 ? Theme::purple : kind == 1 ? Theme::orange : Theme::teal;
    const char* label   = kind == 0 ? "Vocal Balance" : kind == 1 ? "Dynamics" : "Stereo Width";
    const float th = 1.5f * s;

    // título + ícone
    g.setFont (mkFont (15.0f * s));
    juce::GlyphArrangement ga;
    ga.addLineOfText (mkFont (15.0f * s), label, 0.0f, 0.0f);
    const float tw = ga.getBoundingBox (0, -1, true).getWidth();
    const float cx = col.getCentreX(), ty = col.getY() + 14.0f * s;
    auto iconR = Rectangle<float> (cx - tw * 0.5f - 30.0f * s, ty, 24.0f * s, 24.0f * s);
    if (kind == 0) Icons::mic (g, iconR, Theme::dim, th);
    if (kind == 1) Icons::gauge (g, iconR, Theme::dim, th);
    if (kind == 2) Icons::stereo (g, iconR, Theme::dim, th);
    g.setColour (Theme::text);
    g.drawText (label, Rectangle<float> (cx - tw * 0.5f - 2.0f * s, ty, tw + 8.0f * s, 24.0f * s), juce::Justification::centredLeft);

    // trilho
    const float endW = 26.0f * s;
    auto track = Rectangle<float> (col.getX() + 22.0f * s + endW, col.getY() + 62.0f * s,
                                   col.getWidth() - 44.0f * s - endW * 2.0f, 16.0f * s);
    auto left  = Rectangle<float> (col.getX() + 18.0f * s, track.getCentreY() - 12.0f * s, endW, 24.0f * s);
    auto right = Rectangle<float> (track.getRight() + 6.0f * s, track.getCentreY() - 12.0f * s, endW, 24.0f * s);
    if (kind == 0) { Icons::arrow (g, left, false, Theme::dim, th);  Icons::arrow (g, right, true, Theme::dim, th); }
    if (kind == 1) { Icons::bars (g, left, true, Theme::dim, th);    Icons::bars (g, right, false, Theme::dim, th); }
    if (kind == 2) { Icons::circles (g, left, 1, Theme::dim, th);    Icons::circles (g, right, 2, Theme::dim, th); }

    g.setColour (Theme::trackBg);
    g.fillRoundedRectangle (track, track.getHeight() * 0.5f);
    g.setColour (Theme::edge);
    g.drawRoundedRectangle (track, track.getHeight() * 0.5f, 1.0f);

    const float zoneHalf = (1.5f / 6.0f) * track.getWidth() * 0.5f;
    auto zone = Rectangle<float> (track.getCentreX() - zoneHalf, track.getY() + 2.0f, zoneHalf * 2.0f, track.getHeight() - 4.0f);
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.15f), zone.getX(), 0.0f,
                                             accent.withAlpha (0.15f), zone.getRight(), 0.0f, false));
    juce::ColourGradient gr (accent.withAlpha (0.10f), zone.getX(), 0.0f, accent.withAlpha (0.10f), zone.getRight(), 0.0f, false);
    gr.addColour (0.5, accent.withAlpha (0.75f));
    g.setGradientFill (gr);
    g.fillRoundedRectangle (zone, 3.0f * s);
    g.setColour (accent.withAlpha (0.9f));
    g.drawVerticalLine ((int) zone.getX(), zone.getY(), zone.getBottom());
    g.drawVerticalLine ((int) zone.getRight(), zone.getY(), zone.getBottom());

    if (valid)
    {
        const float pos = juce::jlimit (-1.0f, 1.0f, dev / 6.0f);
        const float x = track.getCentreX() + pos * (track.getWidth() * 0.5f - 8.0f * s);
        const float d = 18.0f * s;
        g.setColour (Colour (0x66000000));
        g.fillEllipse (x - d * 0.5f, track.getCentreY() - d * 0.5f + 1.5f, d, d);
        g.setColour (std::abs (dev) <= 1.5f ? Theme::text : Theme::amber);
        g.fillEllipse (x - d * 0.5f, track.getCentreY() - d * 0.5f, d, d);
        g.setColour (Theme::well);
        g.drawEllipse (x - d * 0.5f, track.getCentreY() - d * 0.5f, d, d, 1.5f);

        if (showValues)
        {
            g.setColour (Theme::dim);
            g.setFont (mkFont (12.5f * s));
            g.drawText ((dev >= 0 ? "+" : "") + juce::String (dev, 1) + " dB vs target" + (hint.isNotEmpty() ? "   |   " + hint : juce::String()),
                        Rectangle<float> (col.getX(), track.getBottom() + 12.0f * s, col.getWidth(), 18.0f * s),
                        juce::Justification::centred);
        }
        else if (hint.isNotEmpty())
        {
            g.setColour (Theme::dim);
            g.setFont (mkFont (12.5f * s));
            g.drawText (hint, Rectangle<float> (col.getX(), track.getBottom() + 12.0f * s, col.getWidth(), 18.0f * s), juce::Justification::centred);
        }
    }
    else if (hint.isNotEmpty())
    {
        g.setColour (Theme::dim);
        g.setFont (mkFont (12.5f * s));
        g.drawText (hint, Rectangle<float> (col.getX(), track.getBottom() + 12.0f * s, col.getWidth(), 18.0f * s),
                    juce::Justification::centred);
    }
}

MetersPanel::MetersPanel (RefProProcessor& p) : proc (p), an (p.analyzer)
{
    addAndMakeVisible (autoBtn);
    autoBtn.setClickingTogglesState (true);
    autoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "vocRide", autoBtn);
    autoBtn.setTooltip ("Automatically nudge the vocal level when it leaves the optimal range");
    autoBtn.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
    {
        g.setColour (on ? Theme::tealDark : (over ? Theme::panelHi : Theme::well));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (on ? Theme::teal : Theme::edge);
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.2f);
        g.setColour (on ? Theme::text : Theme::dim);
        g.setFont (mkFont (12.5f * s));
        g.drawText ("Auto +/-" + juce::String (proc.apvts.getRawParameterValue ("vocMax")->load(), 0) + " dB", r, juce::Justification::centred);
    };
    startTimerHz (20);
}

MetersPanel::~MetersPanel() { stopTimer(); autoAttach.reset(); }

void MetersPanel::resized()
{
    const float w = (float) getWidth() / 3.0f;
    autoBtn.setBounds (juce::Rectangle<float> (w * 0.5f - 62.0f * s, 118.0f * s, 124.0f * s, 24.0f * s).toNearestInt());
}

void MetersPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    fillPanel (g, b, 10.0f * s);
    g.setColour (Theme::edge);
    g.drawRoundedRectangle (b.reduced (0.5f), 10.0f * s, 1.0f);

    const bool valid = an.hasData.load();
    const bool vocalSeen = an.vocalConf.load() >= 0.12f;
    const float devVocal = an.vocalDb.load() - target.vocalDb;
    const float devDyn   = an.avgPsr.load() - target.psr;
    const float devWid   = an.avgWidthDb.load() - target.widthDb;

    const float w = b.getWidth() / 3.0f;
    juce::String vocalHint;
    if (valid && ! vocalSeen) vocalHint = "No vocal detected";
    else if (proc.apvts.getRawParameterValue ("vocRide")->load() > 0.5f)
    {
        const float ride = proc.vocRideOut.load();
        vocalHint = "Auto " + juce::String (ride >= 0 ? "+" : "") + juce::String (ride, 1) + " dB";
    }
    drawMeter (g, Rectangle<float> (b.getX(),            b.getY(), w, b.getHeight()), 0, devVocal, valid && vocalSeen, vocalHint);
    drawMeter (g, Rectangle<float> (b.getX() + w,        b.getY(), w, b.getHeight()), 1, devDyn,   valid, "");
    drawMeter (g, Rectangle<float> (b.getX() + w * 2.0f, b.getY(), w, b.getHeight()), 2, devWid,   valid, "");
}

//==============================================================================
//  BandPanel
//==============================================================================
static void stylePill (FlatButton& b, const juce::String& txt, Colour accent, float& scale)
{
    b.setClickingTogglesState (true);
    b.painter = [txt, accent, &scale] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
    {
        g.setColour (on ? accent.withAlpha (0.22f) : (over ? Theme::panelHi : Theme::well));
        g.fillRoundedRectangle (r, 6.0f * scale);
        g.setColour (on ? accent : Theme::edge);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f * scale, 1.2f);
        g.setColour (on ? Theme::text : Theme::dim);
        g.setFont (mkFont (12.5f * scale, on));
        g.drawText (txt, r, juce::Justification::centred);
    };
}

BandPanel::BandPanel (RefProProcessor& p) : proc (p)
{
    for (auto* c : std::initializer_list<juce::Component*> { &onBtn, &delBtn, &dynBtn, &soloBtn, &typeBox, &slopeBox, &modeBox,
                                                             &freq, &gain, &q, &thresh, &ratio })
        addAndMakeVisible (*c);

    typeBox.addItemList (Eq::typeNames(), 1);
    slopeBox.addItemList ({ "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" }, 1);
    modeBox.addItemList ({ "Stereo", "Mid", "Side" }, 1);
    typeBox.setJustificationType (juce::Justification::centredLeft);

    for (auto* sl : { &freq, &gain, &q, &thresh, &ratio })
    {
        sl->setSliderStyle (juce::Slider::LinearBar);
        sl->setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 20);
        sl->setTextBoxIsEditable (true);
        sl->setColour (juce::Slider::trackColourId, Theme::teal.withAlpha (0.3f));
        sl->setColour (juce::Slider::textBoxTextColourId, Theme::text);
        sl->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        sl->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }
    thresh.setColour (juce::Slider::trackColourId, Theme::orange.withAlpha (0.3f));
    ratio.setColour (juce::Slider::trackColourId, Theme::orange.withAlpha (0.3f));

    onBtn.setClickingTogglesState (true);
    onBtn.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
    {
        const auto c = r.withSizeKeepingCentre (18.0f * s, 18.0f * s);
        g.setColour (on ? Theme::teal : Theme::dim);
        juce::Path arc;
        arc.addCentredArc (c.getCentreX(), c.getCentreY() + 1.0f, c.getWidth() * 0.42f, c.getHeight() * 0.42f, 0.0f, 0.55f, juce::MathConstants<float>::twoPi - 0.55f, true);
        g.strokePath (arc, juce::PathStrokeType (1.8f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (c.getCentreX(), c.getY(), c.getCentreX(), c.getCentreY() - 1.0f, 1.8f * s);
        if (over) { g.setColour (Theme::text.withAlpha (0.1f)); g.fillEllipse (r.reduced (2.0f)); }
    };
    delBtn.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool)
    {
        const auto c = r.withSizeKeepingCentre (12.0f * s, 12.0f * s);
        g.setColour (over ? Theme::text : Theme::dim);
        g.drawLine (c.getX(), c.getY(), c.getRight(), c.getBottom(), 1.6f * s);
        g.drawLine (c.getX(), c.getBottom(), c.getRight(), c.getY(), 1.6f * s);
    };
    delBtn.onClick = [this] { if (onDelete) onDelete(); };

    stylePill (dynBtn, "Dyn", Theme::orange, s);
    stylePill (soloBtn, "Solo", Theme::amber, s);
    dynBtn.setTooltip ("Make this band dynamic: it only acts when the band level crosses the threshold");
    soloBtn.setTooltip ("Listen to this band's frequency region");
    soloBtn.onClick = [this] { proc.soloBand.store (soloBtn.getToggleState() ? band : -1); };
    soloBtn.setClickingTogglesState (true);
    setVisible (false);
}

BandPanel::~BandPanel()
{
    proc.soloBand.store (-1);
    aOn.reset(); aDyn.reset(); aType.reset(); aSlope.reset(); aMode.reset(); aFreq.reset(); aGain.reset(); aQ.reset(); aThresh.reset(); aRatio.reset();
}

void BandPanel::setBand (int b)
{
    aOn.reset(); aDyn.reset(); aType.reset(); aSlope.reset(); aMode.reset(); aFreq.reset(); aGain.reset(); aQ.reset(); aThresh.reset(); aRatio.reset();
    proc.soloBand.store (-1);
    soloBtn.setToggleState (false, juce::dontSendNotification);
    band = b;
    shownType = -1;
    if (b < 0) { setVisible (false); return; }

    auto& a = proc.apvts;
    using A = juce::AudioProcessorValueTreeState;
    aOn     = std::make_unique<A::ButtonAttachment>   (a, RefProProcessor::bandId (b, "on"), onBtn);
    aDyn    = std::make_unique<A::ButtonAttachment>   (a, RefProProcessor::bandId (b, "dyn"), dynBtn);
    aType   = std::make_unique<A::ComboBoxAttachment> (a, RefProProcessor::bandId (b, "type"), typeBox);
    aSlope  = std::make_unique<A::ComboBoxAttachment> (a, RefProProcessor::bandId (b, "slope"), slopeBox);
    aMode   = std::make_unique<A::ComboBoxAttachment> (a, RefProProcessor::bandId (b, "mode"), modeBox);
    aFreq   = std::make_unique<A::SliderAttachment>   (a, RefProProcessor::bandId (b, "freq"), freq);
    aGain   = std::make_unique<A::SliderAttachment>   (a, RefProProcessor::bandId (b, "gain"), gain);
    aQ      = std::make_unique<A::SliderAttachment>   (a, RefProProcessor::bandId (b, "q"), q);
    aThresh = std::make_unique<A::SliderAttachment>   (a, RefProProcessor::bandId (b, "thresh"), thresh);
    aRatio  = std::make_unique<A::SliderAttachment>   (a, RefProProcessor::bandId (b, "ratio"), ratio);
    setVisible (true);
    refresh();
}

void BandPanel::refresh()
{
    if (band < 0) return;
    const auto prm = proc.getBandParams (band);
    if (prm.type != shownType)
    {
        shownType = prm.type;
        const bool hasGain = Eq::typeHasGain (prm.type);
        const bool cut = prm.type == Eq::LowCut || prm.type == Eq::HighCut;
        gain.setEnabled (hasGain);
        gain.setAlpha (hasGain ? 1.0f : 0.35f);
        dynBtn.setVisible (hasGain);
        thresh.setVisible (hasGain);
        ratio.setVisible (hasGain);
        slopeBox.setVisible (cut);
        repaint();
    }
    const float a = prm.dyn ? 1.0f : 0.45f;
    thresh.setAlpha (a);
    ratio.setAlpha (a);
    if (prm.dyn) repaint();
}

void BandPanel::resized()
{
    auto R = [this] (float x, float y, float w, float h) { return juce::Rectangle<float> (x * s, y * s, w * s, h * s).toNearestInt(); };
    onBtn.setBounds   (R (6, 22, 30, 30));
    typeBox.setBounds (R (42, 22, 112, 30));
    freq.setBounds    (R (162, 22, 104, 30));
    gain.setBounds    (R (272, 22, 96, 30));
    q.setBounds       (R (374, 22, 52, 30));
    delBtn.setBounds  (juce::Rectangle<float> ((float) getWidth() - 28.0f * s, 4.0f * s, 24.0f * s, 20.0f * s).toNearestInt());
    dynBtn.setBounds  (R (6, 68, 38, 28));
    thresh.setBounds  (R (50, 68, 92, 28));
    slopeBox.setBounds (R (50, 68, 92, 28));
    ratio.setBounds   (R (148, 68, 76, 28));
    modeBox.setBounds (R (306, 68, 84, 28));
    soloBtn.setBounds (R (396, 68, 50, 28));
}

void BandPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Theme::well.withAlpha (0.97f));
    g.fillRoundedRectangle (b, 8.0f * s);
    g.setColour (Theme::teal.withAlpha (0.55f));
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f * s, 1.0f);

    g.setColour (Theme::dim);
    g.setFont (mkFont (11.5f * s));
    g.drawText ("Band " + juce::String (band + 1), Rectangle<float> (10.0f * s, 4.0f * s, 80.0f * s, 16.0f * s), juce::Justification::centredLeft);
    auto label = [&] (const juce::String& txt, const juce::Component& c, float y)
    {
        auto r = c.getBounds().toFloat();
        g.drawText (txt, Rectangle<float> (r.getX(), y, r.getWidth(), 14.0f * s), juce::Justification::centred);
    };
    label ("Freq", freq, 4.0f * s);
    label ("Gain", gain, 4.0f * s);
    label ("Q", q, 4.0f * s);
    label ("Channel", modeBox, 52.0f * s);
    if (thresh.isVisible()) { label ("Thresh", thresh, 52.0f * s); label ("Ratio", ratio, 52.0f * s); }
    if (slopeBox.isVisible()) label ("Slope", slopeBox, 52.0f * s);

    if (band >= 0 && dynBtn.isVisible() && proc.getBandParams (band).dyn)
    {
        const float dg = proc.bandDynGain[(size_t) band].load();
        g.setColour (Theme::orange);
        g.setFont (mkFont (12.5f * s, true));
        g.drawText ("Dyn " + juce::String (dg, 1) + " dB", Rectangle<float> (230.0f * s, 68.0f * s, 72.0f * s, 28.0f * s), juce::Justification::centred);
    }
}

//==============================================================================
//  BlendPanel
//==============================================================================
BlendPanel::BlendPanel (RefProProcessor& p) : proc (p)
{
    addAndMakeVisible (boxA);
    addAndMakeVisible (boxB);
    addAndMakeVisible (mix);
    mix.setSliderStyle (juce::Slider::LinearBar);
    mix.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 20);
    mix.setColour (juce::Slider::trackColourId, Theme::purple.withAlpha (0.3f));
    mix.setColour (juce::Slider::textBoxTextColourId, Theme::text);
    mix.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    mix.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    aMix = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "blendMix", mix);
    boxA.onChange = [this] { const int id = boxA.getSelectedId(); if (id > 0) setParamG (proc.apvts, "blendA", (float) (id - 1)); };
    boxB.onChange = [this] { const int id = boxB.getSelectedId(); if (id > 0) setParamG (proc.apvts, "blendB", (float) (id - 1)); };
    setVisible (false);
}

BlendPanel::~BlendPanel() { aMix.reset(); }

void BlendPanel::setNames (const juce::StringArray& names)
{
    boxA.clear (juce::dontSendNotification);
    boxB.clear (juce::dontSendNotification);
    for (int i = 0; i < names.size(); ++i) { boxA.addItem (names[i], i + 1); boxB.addItem (names[i], i + 1); }
    refresh();
}

void BlendPanel::refresh()
{
    const int a = (int) std::round (proc.apvts.getRawParameterValue ("blendA")->load());
    const int b = (int) std::round (proc.apvts.getRawParameterValue ("blendB")->load());
    if (boxA.getSelectedId() != a + 1) boxA.setSelectedId (a + 1, juce::dontSendNotification);
    if (boxB.getSelectedId() != b + 1) boxB.setSelectedId (b + 1, juce::dontSendNotification);
}

void BlendPanel::resized()
{
    const float w = (float) getWidth(), lx = 40.0f * s;
    boxA.setBounds (juce::Rectangle<float> (lx, 24.0f * s, w - lx, 22.0f * s).toNearestInt());
    boxB.setBounds (juce::Rectangle<float> (lx, 50.0f * s, w - lx, 22.0f * s).toNearestInt());
    mix.setBounds  (juce::Rectangle<float> (lx, 76.0f * s, w - lx, 22.0f * s).toNearestInt());
}

void BlendPanel::paint (juce::Graphics& g)
{
    g.setColour (Theme::edge);
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
    g.setColour (Theme::text);
    g.setFont (mkFont (16.0f * s, true));
    g.drawText ("Blend", Rectangle<float> (4.0f * s, 3.0f * s, 100.0f * s, 20.0f * s), juce::Justification::centredLeft);
    g.setColour (Theme::dim);
    g.setFont (mkFont (12.5f * s));
    g.drawText ("A", Rectangle<float> (4.0f * s, 24.0f * s, 40.0f * s, 22.0f * s), juce::Justification::centredLeft);
    g.drawText ("B", Rectangle<float> (4.0f * s, 50.0f * s, 40.0f * s, 22.0f * s), juce::Justification::centredLeft);
    g.drawText ("Mix", Rectangle<float> (4.0f * s, 76.0f * s, 40.0f * s, 22.0f * s), juce::Justification::centredLeft);
}

//==============================================================================
//  DynPanel  (barra lateral)
//==============================================================================
DynPanel::DynPanel (RefProProcessor& p) : proc (p)
{
    for (auto* sl : { &amount, &maxCut, &maxBoost, &tol, &adapt })
    {
        addAndMakeVisible (*sl);
        sl->setSliderStyle (juce::Slider::LinearBar);
        sl->setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 20);
        sl->setTextBoxIsEditable (true);
        sl->setColour (juce::Slider::textBoxTextColourId, Theme::text);
        sl->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        sl->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }
    maxCut.setColour   (juce::Slider::trackColourId, Theme::orange.withAlpha (0.35f));
    amount.setColour   (juce::Slider::trackColourId, Theme::orange.withAlpha (0.35f));
    maxBoost.setColour (juce::Slider::trackColourId, Theme::teal.withAlpha (0.35f));
    tol.setColour      (juce::Slider::trackColourId, Theme::teal.withAlpha (0.25f));
    adapt.setColour    (juce::Slider::trackColourId, Theme::purple.withAlpha (0.3f));

    addAndMakeVisible (speed);
    speed.addItemList ({ "Fast", "Medium", "Slow" }, 1);

    auto pill = [this] (FlatButton& b, const char* onTxt, const char* offTxt, Colour accent)
    {
        b.setClickingTogglesState (true);
        b.painter = [this, onTxt, offTxt, accent] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
        {
            g.setColour (on ? accent.withAlpha (0.22f) : (over ? Theme::panelHi : Theme::well));
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            g.setColour (on ? accent : Theme::edge);
            g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.2f);
            g.setColour (on ? Theme::text : Theme::dim);
            g.setFont (mkFont (12.5f * s, on));
            g.drawText (on ? onTxt : offTxt, r, juce::Justification::centred);
        };
        addAndMakeVisible (b);
    };
    pill (power, "On", "Off", Theme::orange);
    pill (comp, "On", "Off", Theme::teal);
    power.setTooltip ("Turn the dynamic EQ on/off");
    comp.setTooltip ("Level compensation: keeps the overall loudness stable");

    auto& a = p.apvts;
    aAmount = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>   (a, "dynAmount", amount);
    aMax    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>   (a, "dynMax", maxCut);
    aBoost  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>   (a, "dynBoost", maxBoost);
    aTol    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>   (a, "dynTol", tol);
    aAdapt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>   (a, "dynAdapt", adapt);
    aSpeed  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, "dynSpeed", speed);
    aComp   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>   (a, "dynComp", comp);
    aPower  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>   (a, "dynOn", power);
}

DynPanel::~DynPanel()
{
    aAmount.reset(); aMax.reset(); aBoost.reset(); aTol.reset(); aAdapt.reset(); aSpeed.reset(); aComp.reset(); aPower.reset();
}

void DynPanel::refresh()
{
    const bool on = proc.apvts.getRawParameterValue ("dynOn")->load() > 0.5f;
    if (on == lastOn) return;
    lastOn = on;
    for (auto* c : std::initializer_list<juce::Component*> { &amount, &maxCut, &maxBoost, &tol, &adapt, &speed, &comp })
        c->setAlpha (on ? 1.0f : 0.5f);
}

void DynPanel::resized()
{
    const float w = (float) getWidth(), labelW = 74.0f * s, rowH = 22.0f * s, pitch = 26.0f * s, y0 = 42.0f * s;
    power.setBounds (juce::Rectangle<float> (w - 56.0f * s, 6.0f * s, 56.0f * s, 24.0f * s).toNearestInt());
    int i = 0;
    for (auto* sl : { &amount, &maxCut, &maxBoost, &tol, &adapt })
        sl->setBounds (juce::Rectangle<float> (labelW, y0 + pitch * (float) i++, w - labelW, rowH).toNearestInt());
    speed.setBounds (juce::Rectangle<float> (labelW, y0 + pitch * 5.0f, w - labelW, rowH).toNearestInt());
    comp.setBounds  (juce::Rectangle<float> (labelW, y0 + pitch * 6.0f, 56.0f * s, rowH).toNearestInt());
}

void DynPanel::paint (juce::Graphics& g)
{
    g.setColour (Theme::edge);
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
    g.setColour (Theme::text);
    g.setFont (mkFont (16.0f * s, true));
    g.drawText ("Dynamic EQ", juce::Rectangle<float> (4.0f * s, 6.0f * s, 130.0f * s, 24.0f * s), juce::Justification::centredLeft);

    g.setColour (Theme::dim);
    g.setFont (mkFont (12.5f * s));
    const char* labels[] = { "Amount", "Max Cut", "Max Boost", "Tolerance", "Adapt", "Speed", "Level Comp" };
    juce::Component* cs[] = { &amount, &maxCut, &maxBoost, &tol, &adapt, &speed, &comp };
    for (int i = 0; i < 7; ++i)
    {
        auto r = cs[i]->getBounds().toFloat();
        g.drawText (labels[i], juce::Rectangle<float> (4.0f * s, r.getY(), 70.0f * s, r.getHeight()), juce::Justification::centredLeft);
    }
}

//==============================================================================
//  Editor
//==============================================================================
static const char* kAudioPattern = "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3";

RefProEditor::RefProEditor (RefProProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), graph (p), bandPanel (p), blendPanel (p), dynPanel (p), meters (p)
{
    setLookAndFeel (&laf);

    listViewport.setViewedComponent (&list, false);
    listViewport.setScrollBarsShown (true, false);
    listViewport.setScrollBarThickness (8);
    addAndMakeVisible (listViewport);
    addAndMakeVisible (search);
    addAndMakeVisible (graph);
    addAndMakeVisible (bandPanel);
    addAndMakeVisible (blendPanel);
    addAndMakeVisible (dynPanel);
    addAndMakeVisible (meters);
    for (auto* b : { &viewCurve, &viewBars, &viewLeveled, &targetPill, &captureBtn, &settingsBtn, &extraBtn, &eqBtn,
                     &favFilter, &manageBtn, &abA, &abB })
        addAndMakeVisible (*b);
    addAndMakeVisible (outGain);

    // ---- lista de alvos ----
    list.onSelect   = [this] (int idx) { selectTarget (idx); };
    list.onContext  = [this] (const TargetList::Item& it) { showTargetContext (it); };
    list.onFavorite = [this] (const TargetList::Item& it) { proc.setFavorite (it.name, ! it.favorite); syncTarget (true); };

    search.setTextToShowWhenEmpty ("Search targets...", Theme::dim);
    search.setFont (mkFont (14.0f));
    search.setColour (juce::TextEditor::focusedOutlineColourId, Theme::teal);
    search.onTextChange = [this] { rebuildList(); };

    favFilter.setClickingTogglesState (true);
    favFilter.setTooltip ("Show only favorite targets");
    favFilter.onClick = [this] { rebuildList(); };
    favFilter.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
    {
        g.setColour (on ? Theme::tealDark : (over ? Theme::panelHi : Theme::well));
        g.fillRoundedRectangle (r, 6.0f * s);
        g.setColour (on ? Theme::teal : Theme::edge);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f * s, 1.0f);
        drawHeart (g, r.withSizeKeepingCentre (14.0f * s, 13.0f * s), on, on ? Theme::teal : Theme::dim);
    };
    manageBtn.setTooltip ("Import / export targets, analyze audio files");
    manageBtn.onClick = [this] { showManageMenu(); };
    manageBtn.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool)
    {
        g.setColour (over ? Theme::panelHi : Theme::well);
        g.fillRoundedRectangle (r, 6.0f * s);
        g.setColour (Theme::edge);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f * s, 1.0f);
        g.setColour (Theme::dim);
        for (int i = -1; i <= 1; ++i)
            g.fillEllipse (r.getCentreX() + (float) i * 5.0f * s - 1.8f * s, r.getCentreY() - 1.8f * s, 3.6f * s, 3.6f * s);
    };

    // ---- gráfico / EQ ----
    graph.onSelect = [this] (int b) { selectBand (b); };
    graph.onNodeMoved = [this] { layoutBandPanel(); };
    bandPanel.onDelete = [this]
    {
        if (selectedBand >= 0) setParamG (proc.apvts, RefProProcessor::bandId (selectedBand, "on"), 0.0f);
        selectBand (-1);
    };

    eqBtn.setClickingTogglesState (true);
    eqAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "eqOn", eqBtn);
    eqBtn.setTooltip ("Turn the 10-band equalizer on/off");
    eqBtn.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
    {
        g.setColour (on ? Theme::tealDark : (over ? Theme::panelHi.brighter (0.15f) : Theme::panelHi));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (on ? Theme::teal : Theme::edge.brighter (0.25f));
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.2f);
        g.setColour (on ? Theme::teal : Theme::dim);
        g.fillEllipse (r.getX() + 16.0f * s, r.getCentreY() - 4.5f * s, 9.0f * s, 9.0f * s);
        g.setColour (on ? Theme::text : Theme::dim);
        g.setFont (mkFont (15.0f * s));
        g.drawText ("Equalizer", r.withTrimmedLeft (20.0f * s), juce::Justification::centred);
    };

    for (auto* ab : { &abA, &abB })
    {
        const int slot = ab == &abA ? 0 : 1;
        ab->setTooltip (slot == 0 ? "EQ setting A" : "EQ setting B (compare)");
        ab->onClick = [this, slot] { proc.abSwitch (slot); abA.repaint(); abB.repaint(); };
        ab->painter = [this, slot] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool)
        {
            const bool on = proc.abSlot() == slot;
            g.setColour (on ? Theme::tealDark : (over ? Theme::panelHi : Theme::well));
            g.fillRoundedRectangle (r, 7.0f * s);
            g.setColour (on ? Theme::teal : Theme::edge);
            g.drawRoundedRectangle (r.reduced (0.5f), 7.0f * s, 1.2f);
            g.setColour (on ? Theme::text : Theme::dim);
            g.setFont (mkFont (14.0f * s, on));
            g.drawText (slot == 0 ? "A" : "B", r, juce::Justification::centred);
        };
    }

    outGain.setSliderStyle (juce::Slider::LinearBar);
    outGain.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 20);
    outGain.setColour (juce::Slider::trackColourId, Theme::teal.withAlpha (0.3f));
    outGain.setColour (juce::Slider::textBoxTextColourId, Theme::text);
    outGain.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    outGain.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    outGain.setTextBoxIsEditable (true);
    outAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "outGain", outGain);

    // ---- visões ----
    viewCurve.setClickingTogglesState (false);
    viewBars.setClickingTogglesState (false);
    viewCurve.setToggleState (true, juce::dontSendNotification);
    viewCurve.onClick = [this] { barsView = false; graph.setBars (false); viewCurve.setToggleState (true, juce::dontSendNotification); viewBars.setToggleState (false, juce::dontSendNotification); };
    viewBars.onClick  = [this] { barsView = true;  graph.setBars (true);  viewCurve.setToggleState (false, juce::dontSendNotification); viewBars.setToggleState (true, juce::dontSendNotification); };
    viewLeveled.setClickingTogglesState (true);
    viewLeveled.onClick = [this] { graph.setLeveled (viewLeveled.getToggleState()); };
    viewCurve.setTooltip ("Curve view");
    viewBars.setTooltip ("Bars view");
    viewLeveled.setTooltip ("Leveled view: the target becomes the 0 dB line and the music is shown as the deviation from it");

    viewCurve.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
    {
        if (on) { g.setColour (Theme::panelHi.brighter (0.15f)); g.fillRoundedRectangle (r.reduced (2.0f), 14.0f * s); }
        Icons::curveIcon (g, r, (on || over) ? Theme::text : Theme::dim, 1.4f * s);
    };
    viewBars.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
    {
        if (on) { g.setColour (Theme::panelHi.brighter (0.15f)); g.fillRoundedRectangle (r.reduced (2.0f), 14.0f * s); }
        Icons::barsIcon (g, r, (on || over) ? Theme::text : Theme::dim);
    };
    viewLeveled.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
    {
        if (on) { g.setColour (Theme::tealDark); g.fillRoundedRectangle (r.reduced (2.0f), 14.0f * s); }
        Icons::levelIcon (g, r, (on || over) ? Theme::text : Theme::dim, 1.4f * s);
    };

    targetPill.onClick = [this] { showTargetMenu(); };
    targetPill.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool)
    {
        g.setGradientFill (juce::ColourGradient (Theme::tealDark.brighter (over ? 0.2f : 0.0f), r.getX(), 0.0f,
                                                 Theme::tealDark.darker (0.3f), r.getRight(), 0.0f, false));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (Theme::teal.withAlpha (0.8f));
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.2f);
        g.setColour (Theme::text);
        g.setFont (mkFont (15.0f * s));
        g.drawText (current.name, r, juce::Justification::centred);
        juce::Path a;
        a.startNewSubPath (r.getRight() - 30.0f * s, r.getCentreY() - 3.0f * s);
        a.lineTo (r.getRight() - 25.0f * s, r.getCentreY() + 2.0f * s);
        a.lineTo (r.getRight() - 20.0f * s, r.getCentreY() - 3.0f * s);
        g.strokePath (a, juce::PathStrokeType (1.5f * s));
        g.drawEllipse (r.getX() + 16.0f * s, r.getCentreY() - 8.0f * s, 16.0f * s, 16.0f * s, 1.3f);
        g.drawEllipse (r.getX() + 20.0f * s, r.getCentreY() - 4.0f * s, 8.0f * s, 8.0f * s, 1.3f);
    };

    captureBtn.onClick = [this] { doCapture(); };
    captureBtn.setTooltip ("Save the current average curve as a new target");
    captureBtn.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool down, bool)
    {
        g.setColour (over ? Theme::panelHi.brighter (0.15f) : Theme::panelHi);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (Theme::edge.brighter (0.2f));
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
        g.setColour (down ? Theme::amber : Theme::text);
        g.fillEllipse (r.getX() + 18.0f * s, r.getCentreY() - 5.0f * s, 10.0f * s, 10.0f * s);
        g.setFont (mkFont (15.0f * s));
        g.drawText ("Capture", r.withTrimmedLeft (22.0f * s), juce::Justification::centred);
    };

    settingsBtn.onClick = [this] { showSettingsMenu(); };
    settingsBtn.painter = [] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool)
    {
        Icons::gear (g, r, over ? Theme::text : Theme::dim, 1.4f);
    };

    extraBtn.onClick = [this]
    {
        showExtra = ! showExtra;
        extraBtn.setToggleState (showExtra, juce::dontSendNotification);
        graph.setExtra (showExtra);
        meters.setValues (showExtra);
    };
    extraBtn.painter = [this] (juce::Graphics& g, Rectangle<float> r, bool over, bool, bool on)
    {
        g.setColour (on ? Theme::tealDark.brighter (0.2f) : (over ? Theme::tealDark.brighter (0.1f) : Theme::tealDark));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (Theme::teal.withAlpha (on ? 1.0f : 0.7f));
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.2f);
        g.setColour (Theme::text);
        g.setFont (mkFont (15.0f * s));
        g.drawText ("Extra Meters", r, juce::Justification::centred);
    };

    setResizable (true, true);
    setResizeLimits (840, 555, 1680, 1110);
    getConstrainer()->setFixedAspectRatio (1120.0 / 740.0);
    setSize (1120, 740);

    syncTarget (true);
    startTimerHz (10);
}

RefProEditor::~RefProEditor()
{
    stopTimer();
    eqAttach.reset();
    outAttach.reset();
    listViewport.setViewedComponent (nullptr, false);
    setLookAndFeel (nullptr);
}

int RefProEditor::currentIndex() const
{
    const int raw = (int) std::round (proc.apvts.getRawParameterValue ("target")->load());
    if (raw == RefProProcessor::blendIndex) return raw;
    return juce::jlimit (0, proc.getNumTargets() - 1, raw);
}

void RefProEditor::rebuildList()
{
    std::vector<TargetList::Item> items;
    const auto q = search.getText().trim();
    const bool favOnly = favFilter.getToggleState();
    auto add = [&] (int index, const juce::String& name, bool builtIn)
    {
        if (q.isNotEmpty() && ! name.containsIgnoreCase (q)) return;
        const bool fav = proc.isFavorite (name);
        if (favOnly && ! fav) return;
        items.push_back ({ name, index, fav, builtIn });
    };
    add (0, "All-Purpose", true);
    add (RefProProcessor::blendIndex, "Blend", true);
    for (int i = 1; i < proc.getNumTargets(); ++i) add (i, proc.getTarget (i).name, false);
    list.setItems (items, currentIndex());
    layoutSidebar();
}

void RefProEditor::syncTarget (bool force)
{
    const int idx = currentIndex(), ver = proc.targetsVersion.load(), bv = proc.blendVersion.load();
    if (! force && idx == cachedIndex && ver == cachedVersion && bv == cachedBlend) return;
    const bool namesChanged = force || ver != cachedVersion;
    cachedIndex = idx; cachedVersion = ver; cachedBlend = bv;

    current = proc.getTarget (idx);
    graph.setTarget (current);
    meters.setTarget (current);
    targetPill.repaint();

    if (namesChanged)
    {
        juce::StringArray names;
        for (int i = 0; i < proc.getNumTargets(); ++i) names.add (proc.getTarget (i).name);
        blendPanel.setNames (names);
    }
    rebuildList();
    blendPanel.setVisible (idx == RefProProcessor::blendIndex);
    repaint (stripArea.toNearestInt());
}

void RefProEditor::selectTarget (int idx)
{
    if (auto* prm = proc.apvts.getParameter ("target"))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) juce::jlimit (0, RefProProcessor::blendIndex, idx)));
        prm->endChangeGesture();
    }
    syncTarget (true);
}

void RefProEditor::showTargetMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "All-Purpose", true, currentIndex() == 0);
    m.addItem (RefProProcessor::blendIndex + 1, "Blend", true, currentIndex() == RefProProcessor::blendIndex);
    for (int i = 1; i < proc.getNumTargets(); ++i)
        m.addItem (i + 1, proc.getTarget (i).name, true, i == currentIndex());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&targetPill),
                     [this] (int r) { if (r > 0) selectTarget (r - 1); });
}

void RefProEditor::showTargetContext (const TargetList::Item& it)
{
    juce::PopupMenu m;
    m.addItem (1, it.favorite ? "Remove from favorites" : "Add to favorites");
    if (! it.builtIn) m.addItem (2, "Rename...");
    m.addItem (3, "Export target...");
    if (! it.builtIn) { m.addSeparator(); m.addItem (4, "Delete"); }

    juce::Component::SafePointer<RefProEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [safe, it] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& pr = safe->proc;
        if (r == 1) { pr.setFavorite (it.name, ! it.favorite); safe->syncTarget (true); }
        else if (r == 4)
        {
            const int sel = safe->currentIndex();
            pr.deleteCapture (it.index);
            safe->selectTarget (sel == it.index ? 0 : (sel != RefProProcessor::blendIndex && sel > it.index ? sel - 1 : sel));
        }
        else if (r == 2)
        {
            auto* w = new juce::AlertWindow ("Rename target", "Enter a new name:", juce::MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", it.name);
            w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
            w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            const int idx = it.index;
            w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w, idx] (int res)
            {
                if (res == 1 && safe != nullptr) { safe->proc.renameTarget (idx, w->getTextEditorContents ("name")); safe->syncTarget (true); }
            }), true);
        }
        else if (r == 3)
        {
            safe->fileChooser = std::make_unique<juce::FileChooser> ("Export target", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                                       .getChildFile (it.name + ".rptarget"), "*.rptarget");
            const int idx = it.index;
            safe->fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                              | juce::FileBrowserComponent::warnAboutOverwriting,
                                            [safe, idx] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (safe != nullptr && f != juce::File() && ! safe->proc.exportTarget (idx, f.withFileExtension ("rptarget")))
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "RefPro SILVER", "Could not write the file.");
            });
        }
    });
}

void RefProEditor::analyzeFiles (const juce::Array<juce::File>& files, const juce::String& name)
{
    if (files.isEmpty()) return;
    proc.startBuild (files, name);
}

void RefProEditor::showManageMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Analyze audio files...");
    m.addItem (2, "Analyze folder...");
    m.addSeparator();
    m.addItem (3, "Import target file...");
    m.addItem (4, "Export current target...");
    if (proc.buildBusy.load()) { m.addSeparator(); m.addItem (5, "Cancel analysis"); }

    juce::Component::SafePointer<RefProEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&manageBtn), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto* self = safe.getComponent();
        using FB = juce::FileBrowserComponent;
        if (r == 5) { self->proc.cancelBuild(); return; }
        if (r == 1)
        {
            self->fileChooser = std::make_unique<juce::FileChooser> ("Select audio files to build a target from", juce::File(), kAudioPattern);
            self->fileChooser->launchAsync (FB::openMode | FB::canSelectFiles | FB::canSelectMultipleItems, [safe] (const juce::FileChooser& fc)
            {
                if (safe == nullptr) return;
                const auto res = fc.getResults();
                if (res.isEmpty()) return;
                const auto name = res.size() == 1 ? res[0].getFileNameWithoutExtension()
                                                  : res[0].getParentDirectory().getFileName() + " (" + juce::String (res.size()) + " files)";
                safe->analyzeFiles (res, name);
            });
        }
        else if (r == 2)
        {
            self->fileChooser = std::make_unique<juce::FileChooser> ("Select a folder of reference tracks", juce::File());
            self->fileChooser->launchAsync (FB::openMode | FB::canSelectDirectories, [safe] (const juce::FileChooser& fc)
            {
                if (safe == nullptr) return;
                const auto dir = fc.getResult();
                if (! dir.isDirectory()) return;
                const auto files = dir.findChildFiles (juce::File::findFiles, true, kAudioPattern);
                if (files.isEmpty())
                {
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "RefPro SILVER", "No audio files found in that folder.");
                    return;
                }
                safe->analyzeFiles (files, dir.getFileName());
            });
        }
        else if (r == 3)
        {
            self->fileChooser = std::make_unique<juce::FileChooser> ("Import target file(s)", juce::File(), "*.rptarget;*.json");
            self->fileChooser->launchAsync (FB::openMode | FB::canSelectFiles | FB::canSelectMultipleItems, [safe] (const juce::FileChooser& fc)
            {
                if (safe == nullptr) return;
                int failed = 0;
                for (auto& f : fc.getResults())
                    if (safe->proc.importTargetFile (f) < 0) ++failed;
                if (failed > 0)
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "RefPro SILVER",
                                                            juce::String (failed) + " file(s) could not be imported (invalid format or target list full).");
            });
        }
        else if (r == 4)
        {
            const int idx = self->currentIndex();
            self->fileChooser = std::make_unique<juce::FileChooser> ("Export target", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                                       .getChildFile (self->current.name + ".rptarget"), "*.rptarget");
            self->fileChooser->launchAsync (FB::saveMode | FB::canSelectFiles | FB::warnAboutOverwriting, [safe, idx] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (safe != nullptr && f != juce::File() && ! safe->proc.exportTarget (idx, f.withFileExtension ("rptarget")))
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "RefPro SILVER", "Could not write the file.");
            });
        }
    });
}

void RefProEditor::selectBand (int b)
{
    selectedBand = b;
    graph.setSelected (b);
    bandPanel.setBand (b);
    layoutBandPanel();
}

void RefProEditor::layoutBandPanel()
{
    if (selectedBand < 0) { bandPanel.setVisible (false); return; }

    const float w = 452.0f * s, h = BandPanel::baseHeight * s;
    const auto gb = graph.getBounds().toFloat();
    const auto node = graph.getNodePosition (selectedBand) + gb.getPosition();
    const float x = juce::jlimit (gb.getX() + 6.0f * s, gb.getRight() - w - 6.0f * s, node.x - w * 0.5f);
    float y = node.y + 20.0f * s;
    if (y + h > gb.getBottom() - 30.0f * s) y = node.y - 20.0f * s - h;
    y = juce::jmax (gb.getY() + 48.0f * s, y);
    bandPanel.setScale (s);
    bandPanel.setBounds (juce::Rectangle<float> (x, y, w, h).toNearestInt());
    bandPanel.setVisible (true);
    bandPanel.toFront (false);
}

void RefProEditor::layoutSidebar()
{
    if (getWidth() <= 0) return;
    const float m = 12.0f * s, sideW = 218.0f * s;
    auto R = [s = s] (float x, float y, float w, float h) { return juce::Rectangle<float> (x * s, y * s, w * s, h * s).toNearestInt(); };

    search.setBounds    (R (22, 64, 134, 28));
    favFilter.setBounds (R (160, 64, 28, 28));
    manageBtn.setBounds (R (192, 64, 28, 28));
    listViewport.setBounds (R (16, 102, 206, 166));
    list.s = s;
    list.setSize (listViewport.getMaximumVisibleWidth(), std::max (list.getContentHeight(), (int) (170.0f * s)));

    stripArea = juce::Rectangle<float> (m + 10.0f * s, m + 274.0f * s, sideW - 20.0f * s, BlendPanel::baseHeight * s);
    blendPanel.setScale (s);
    blendPanel.setBounds (stripArea.toNearestInt());
    dynPanel.setScale (s);
    dynPanel.setBounds (juce::Rectangle<float> (m + 10.0f * s, m + 380.0f * s, sideW - 20.0f * s, DynPanel::baseHeight * s).toNearestInt());
    msgArea = juce::Rectangle<float> (m + 14.0f * s, (float) getHeight() - m - 62.0f * s, sideW - 28.0f * s, 54.0f * s);
}

void RefProEditor::resetEq()
{
    selectBand (-1);
    for (int b = 0; b < Eq::numBands; ++b)
        for (const char* id : { "on", "type", "freq", "gain", "q", "dyn", "thresh", "ratio", "slope", "mode" })
            if (auto* prm = proc.apvts.getParameter (RefProProcessor::bandId (b, id)))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (prm->getDefaultValue());
                prm->endChangeGesture();
            }
    if (auto* prm = proc.apvts.getParameter ("outGain")) { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->getDefaultValue()); prm->endChangeGesture(); }
}

void RefProEditor::showSettingsMenu()
{
    juce::PopupMenu avg;
    const int cur = (int) proc.apvts.getRawParameterValue ("avg")->load();
    const char* labels[] = { "Short (3 s)", "Medium (10 s)", "Long (30 s)", "Infinite (whole song)" };
    for (int i = 0; i < 4; ++i) avg.addItem (10 + i, labels[i], true, i == cur);

    const int post = (int) proc.apvts.getRawParameterValue ("anPost")->load();
    juce::PopupMenu pt;
    pt.addItem (20, "Pre-EQ (input)", true, post == 0);
    pt.addItem (21, "Post-EQ (output)", true, post == 1);

    juce::PopupMenu m;
    m.addSubMenu ("Averaging", avg);
    m.addSubMenu ("Analysis point", pt);
    m.addSeparator();
    if (proc.isStandaloneApp())
        m.addItem (30, "Analyze system audio (Spotify / YouTube)", proc.isLoopbackAvailable(), proc.isLoopbackOn());
    m.addItem (1, "Reset analysis");
    m.addItem (2, "Reset equalizer");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsBtn), [this] (int r)
    {
        if (r == 1) proc.analyzer.requestReset();
        else if (r == 30)
        {
            if (proc.isLoopbackOn()) proc.stopLoopback();
            else
            {
                juce::String err;
                if (! proc.startLoopback (err))
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "RefPro SILVER", err);
                else
                    proc.analyzer.requestReset();
            }
        }
        else if (r == 2) resetEq();
        else if (r == 20 || r == 21)
        {
            setParamG (proc.apvts, "anPost", (float) (r - 20));
            proc.analyzer.requestReset();
        }
        else if (r >= 10 && r <= 13)
            setParamG (proc.apvts, "avg", (float) (r - 10));
    });
}

void RefProEditor::doCapture()
{
    using R = RefProProcessor::CaptureResult;
    switch (proc.captureCurrent())
    {
        case R::ok:     selectTarget (proc.getNumTargets() - 1); break;
        case R::noData: juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "RefPro SILVER",
                            "Nothing to capture yet. Play some audio first."); break;
        case R::full:   juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "RefPro SILVER",
                            "Target limit reached (32). Right-click a target in the list to delete it."); break;
    }
}

void RefProEditor::timerCallback()
{
    syncTarget (false);
    if (selectedBand >= 0) bandPanel.refresh();
    dynPanel.refresh();
    if (blendPanel.isVisible()) blendPanel.refresh();
    const int ni = proc.consumeNewIndex();
    if (ni >= 0) selectTarget (ni);
    abA.repaint(); abB.repaint();
    repaint (statusArea.toNearestInt());
    repaint (msgArea.toNearestInt());
}

void RefProEditor::resized()
{
    s = (float) getWidth() / 1120.0f;
    list.s = s; graph.setScale (s); meters.setScale (s); bandPanel.setScale (s);

    auto W = (float) getWidth(), H = (float) getHeight();
    const float m = 12.0f * s;
    const float x0 = m + 218.0f * s + 10.0f * s;

    layoutSidebar();

    const float hy = m, hh = 44.0f * s;
    viewCurve.setBounds   (juce::Rectangle<float> (x0 + 236.0f * s, hy, 46.0f * s, hh).toNearestInt());
    viewBars.setBounds    (juce::Rectangle<float> (x0 + 282.0f * s, hy, 46.0f * s, hh).toNearestInt());
    viewLeveled.setBounds (juce::Rectangle<float> (x0 + 328.0f * s, hy, 46.0f * s, hh).toNearestInt());
    settingsBtn.setBounds (juce::Rectangle<float> (W - m - 46.0f * s, hy, 46.0f * s, hh).toNearestInt());
    captureBtn.setBounds  (juce::Rectangle<float> (W - m - 46.0f * s - 148.0f * s, hy + 3.0f * s, 140.0f * s, hh - 6.0f * s).toNearestInt());
    const float pillX = x0 + 392.0f * s;
    targetPill.setBounds (juce::Rectangle<float> (pillX, hy + 1.0f * s, captureBtn.getX() - pillX - 10.0f * s, hh - 2.0f * s).toNearestInt());

    const float gy = hy + hh + 10.0f * s;
    const float gH = 440.0f * s;
    graph.setBounds (juce::Rectangle<float> (x0, gy, W - m - x0, gH).toNearestInt());

    const float ry = gy + gH + 10.0f * s;
    const float by = ry + 7.0f * s;
    extraBtn.setBounds (juce::Rectangle<float> (x0, by, 116.0f * s, 36.0f * s).toNearestInt());
    eqBtn.setBounds    (juce::Rectangle<float> (x0 + 124.0f * s, by, 104.0f * s, 36.0f * s).toNearestInt());
    abA.setBounds      (juce::Rectangle<float> (x0 + 238.0f * s, by + 3.0f * s, 34.0f * s, 30.0f * s).toNearestInt());
    abB.setBounds      (juce::Rectangle<float> (x0 + 276.0f * s, by + 3.0f * s, 34.0f * s, 30.0f * s).toNearestInt());
    outGain.setBounds  (juce::Rectangle<float> (x0 + 326.0f * s, ry + 18.0f * s, 124.0f * s, 28.0f * s).toNearestInt());
    statusArea = juce::Rectangle<float> (x0 + 462.0f * s, ry, W - m - x0 - 462.0f * s, 50.0f * s);

    const float my = ry + 50.0f * s + 10.0f * s;
    meters.setBounds (juce::Rectangle<float> (x0, my, W - m - x0, H - m - my).toNearestInt());
    layoutBandPanel();
}

void RefProEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (Theme::bg.brighter (0.12f), 0.0f, 0.0f, Theme::bg.darker (0.1f), 0.0f, (float) getHeight(), false));
    g.fillAll();

    const float m = 12.0f * s, sideW = 218.0f * s;
    auto side = Rectangle<float> (m, m, sideW, (float) getHeight() - 2.0f * m);
    fillPanel (g, side, 10.0f * s);
    g.setColour (Theme::edge);
    g.drawRoundedRectangle (side.reduced (0.5f), 10.0f * s, 1.0f);

    g.setColour (Theme::text);
    g.setFont (mkFont (20.0f * s, true));
    g.drawText ("Target", Rectangle<float> (side.getX() + 14.0f * s, side.getY() + 12.0f * s, 120.0f * s, 28.0f * s), juce::Justification::centredLeft);
    g.setColour (Theme::edge);
    g.drawHorizontalLine ((int) (side.getY() + 46.0f * s), side.getX() + 10.0f * s, side.getRight() - 10.0f * s);

    // dica quando o painel Blend não está visível
    if (! blendPanel.isVisible() && ! stripArea.isEmpty())
    {
        g.setColour (Theme::dim);
        g.setFont (mkFont (12.0f * s));
        g.drawFittedText ("Select \"Blend\" to mix two targets. Right-click a target for more options; the ... button imports, exports and analyzes audio files.",
                          stripArea.reduced (4.0f * s, 8.0f * s).toNearestInt(), juce::Justification::topLeft, 5);
    }

    // mensagem / progresso da análise de arquivos
    if (! msgArea.isEmpty())
    {
        const bool busy = proc.buildBusy.load();
        const auto msg = proc.getBuildMessage();
        if (busy)
        {
            g.setColour (Theme::text);
            g.setFont (mkFont (12.5f * s));
            g.drawText ("Analyzing audio... " + juce::String ((int) std::round (proc.buildProgress.load() * 100.0f)) + " %",
                        msgArea.withHeight (18.0f * s), juce::Justification::centredLeft);
            auto bar = Rectangle<float> (msgArea.getX(), msgArea.getY() + 22.0f * s, msgArea.getWidth(), 7.0f * s);
            g.setColour (Theme::trackBg);
            g.fillRoundedRectangle (bar, 3.5f * s);
            g.setColour (Theme::teal);
            g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, proc.buildProgress.load())), 3.5f * s);
        }
        else if (msg.isNotEmpty())
        {
            g.setColour (Theme::dim);
            g.setFont (mkFont (12.0f * s));
            g.drawFittedText (msg, msgArea.toNearestInt(), juce::Justification::bottomLeft, 3);
        }
    }

    // logotipo
    const float x0 = m + sideW + 10.0f * s, hy = m, hh = 44.0f * s;
    const float cx = x0 + 22.0f * s, cy = hy + hh * 0.5f, R = 17.0f * s;
    g.setColour (Theme::teal);
    g.drawEllipse (cx - R, cy - R, R * 2.0f, R * 2.0f, 2.0f * s);
    g.fillEllipse (cx - 5.0f * s, cy - 4.0f * s, 10.0f * s, 10.0f * s);
    g.setColour (Theme::text);
    g.fillEllipse (cx + 6.0f * s, cy - 11.0f * s, 5.0f * s, 5.0f * s);

    g.setColour (Theme::text);
    g.setFont (mkFont (19.0f * s, true));
    g.drawText ("REFPRO", Rectangle<float> (x0 + 48.0f * s, hy + 2.0f * s, 170.0f * s, 22.0f * s), juce::Justification::centredLeft);
    g.setColour (Theme::teal);
    g.drawText ("SILVER", Rectangle<float> (x0 + 48.0f * s, hy + 22.0f * s, 170.0f * s, 20.0f * s), juce::Justification::centredLeft);

    auto grp = Rectangle<float> (x0 + 236.0f * s, hy + 2.0f * s, 138.0f * s, hh - 4.0f * s);
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (grp, grp.getHeight() * 0.5f);
    g.setColour (Theme::edge.brighter (0.2f));
    g.drawRoundedRectangle (grp.reduced (0.5f), grp.getHeight() * 0.5f, 1.0f);

    // rótulo do ganho de saída e status
    if (outGain.getWidth() > 0)
    {
        g.setColour (Theme::dim);
        g.setFont (mkFont (11.0f * s));
        g.drawText ("EQ Out Gain", Rectangle<float> ((float) outGain.getX(), (float) outGain.getY() - 15.0f * s, (float) outGain.getWidth(), 14.0f * s), juce::Justification::centred);
    }
    if (! statusArea.isEmpty())
    {
        static const char* avgNames[] = { "3 s", "10 s", "30 s", "whole song" };
        const int a = juce::jlimit (0, 3, (int) proc.apvts.getRawParameterValue ("avg")->load());
        const bool sig = proc.analyzer.signalActive.load();
        g.setFont (mkFont (13.0f * s));
        g.setColour (Theme::dim);
        g.drawText (juce::String ("Averaging: ") + avgNames[a] + (proc.isLoopbackOn() ? juce::String ("  |  System audio: ") + proc.getLoopbackDevice()
                                                                                  : juce::String (proc.apvts.getRawParameterValue ("anPost")->load() > 0.5f ? "  |  Post-EQ" : "  |  Pre-EQ")),
                    statusArea.withTrimmedRight (30.0f * s), juce::Justification::centredRight);
        g.setColour (sig ? Theme::teal : Theme::edge.brighter (0.3f));
        g.fillEllipse (statusArea.getRight() - 18.0f * s, statusArea.getCentreY() - 4.5f * s, 9.0f * s, 9.0f * s);
    }
}
