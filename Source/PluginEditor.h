#pragma once

#include "PluginProcessor.h"

namespace Theme
{
    // Tema "prata": fundos metálicos claros, texto em azul-marinho, acentos escurecidos para contraste
    const juce::Colour bg        { 0xffc9cfd8 };   // janela
    const juce::Colour panel     { 0xffeef1f5 };   // painéis
    const juce::Colour panelHi   { 0xfff8f9fb };   // botões / realces claros
    const juce::Colour well      { 0xfff6f7fa };   // campos e poços claros
    const juce::Colour trackBg   { 0xffb4bcc9 };   // trilhos de sliders/medidores
    const juce::Colour edge      { 0xffaeb7c4 };   // linhas e contornos
    const juce::Colour teal      { 0xff0b8e94 };
    const juce::Colour tealDark  { 0xffbfe4e6 };   // preenchimento teal claro
    const juce::Colour text      { 0xff0f2350 };   // azul-marinho
    const juce::Colour dim       { 0xff4b5f86 };   // azul-acinzentado escuro
    const juce::Colour purple    { 0xff5b49c8 };
    const juce::Colour orange    { 0xffd2690c };
    const juce::Colour amber     { 0xffb8730c };
    const juce::Colour curve     { 0xff0c1f4d };   // curva medida
}

inline juce::Font mkFont (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

// Cores do tema para menus, combo box e sliders
class RefLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RefLookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId, Theme::panelHi);
        setColour (juce::PopupMenu::textColourId, Theme::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::tealDark);
        setColour (juce::PopupMenu::highlightedTextColourId, Theme::text);
        setColour (juce::PopupMenu::headerTextColourId, Theme::dim);
        setColour (juce::ComboBox::backgroundColourId, Theme::well);
        setColour (juce::ComboBox::textColourId, Theme::text);
        setColour (juce::ComboBox::outlineColourId, Theme::edge.brighter (0.2f));
        setColour (juce::ComboBox::arrowColourId, Theme::dim);
        setColour (juce::Slider::trackColourId, Theme::tealDark);
        setColour (juce::Slider::backgroundColourId, Theme::trackBg);
        setColour (juce::Slider::textBoxTextColourId, Theme::text);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::backgroundColourId, Theme::well);
        setColour (juce::TextEditor::textColourId, Theme::text);
        setColour (juce::TextEditor::highlightColourId, Theme::tealDark);
        setColour (juce::TextEditor::outlineColourId, Theme::teal);
        setColour (juce::CaretComponent::caretColourId, Theme::teal);
    }
};

// Botão genérico desenhado por uma função.
class FlatButton : public juce::Button
{
public:
    using Painter = std::function<void (juce::Graphics&, juce::Rectangle<float>, bool over, bool down, bool on)>;
    explicit FlatButton (const juce::String& name) : juce::Button (name) {}
    Painter painter;
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        if (painter) painter (g, getLocalBounds().toFloat(), over, down, getToggleState());
    }
};

// Lista de alvos (barra lateral, dentro de um Viewport)
class TargetList : public juce::Component
{
public:
    struct Item { juce::String name; int index = 0; bool favorite = false; bool builtIn = false; };
    std::function<void (int)> onSelect;
    std::function<void (const Item&)> onContext, onFavorite;
    float s = 1.0f;

    static constexpr float rowBase = 34.0f;
    void setItems (const std::vector<Item>& it, int selectedIndex) { items = it; selected = selectedIndex; repaint(); }
    int  getContentHeight() const { return (int) std::ceil ((float) items.size() * rowBase * s); }
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    std::vector<Item> items;
    int selected = 0;
};

// Gráfico principal (curva tonal, alvo All-Purpose e nós do equalizador)
class GraphView : public juce::Component, private juce::Timer
{
public:
    explicit GraphView (RefProProcessor& p) : proc (p), an (p.analyzer) { startTimerHz (30); }
    ~GraphView() override { stopTimer(); }

    std::function<void (int)> onSelect;      // banda selecionada (-1 = nenhuma)
    std::function<void()>     onNodeMoved;

    void setTarget (const TargetData& t) { target = t; repaint(); }
    void setScale (float v) { s = v; }
    void setBars (bool b)   { bars = b; repaint(); }
    void setExtra (bool e)  { extra = e; repaint(); }
    void setLeveled (bool l) { leveled = l; repaint(); }
    void setSelected (int b) { selected = b; repaint(); }
    juce::Point<float> getNodePosition (int band) const;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent& e) override { hoverX = e.position.x; }
    void mouseExit (const juce::MouseEvent&) override   { hoverX = -1.0f; }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override { repaint(); }
    juce::Rectangle<float> plotRect() const;
    float xOf (float f) const;
    float yOf (float db) const;
    float fOf (float x) const;
    float dbOf (float y) const;
    int  hitNode (juce::Point<float> p) const;
    void select (int b);

    RefProProcessor& proc;
    TonalAnalyzer& an;
    TargetData target;
    float s = 1.0f, hoverX = -1.0f;
    bool bars = false, extra = false, leveled = false;
    int selected = -1, dragBand = -1;
    float topDb() const    { return leveled ? 10.0f : rp::dbTop; }
    float bottomDb() const { return leveled ? -10.0f : rp::dbBottom; }
};

// Painel de edição da banda selecionada (tipo / freq / ganho / Q / dinâmica / slope / canal / solo)
class BandPanel : public juce::Component
{
public:
    explicit BandPanel (RefProProcessor& p);
    ~BandPanel() override;

    std::function<void()> onDelete;

    void setBand (int b);
    void setScale (float v) { s = v; }
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr float baseHeight = 100.0f;

private:
    RefProProcessor& proc;
    int band = -1;
    float s = 1.0f;
    int shownType = -1;

    FlatButton onBtn { "on" }, delBtn { "del" }, dynBtn { "dyn" }, soloBtn { "solo" };
    juce::ComboBox typeBox, slopeBox, modeBox;
    juce::Slider freq, gain, q, thresh, ratio;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aOn, aDyn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aType, aSlope, aMode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aFreq, aGain, aQ, aThresh, aRatio;
};

// Mistura de dois alvos (aparece quando o alvo "Blend" está selecionado)
class BlendPanel : public juce::Component
{
public:
    explicit BlendPanel (RefProProcessor& p);
    ~BlendPanel() override;
    void setScale (float v) { s = v; }
    void setNames (const juce::StringArray& names);
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr float baseHeight = 98.0f;

private:
    RefProProcessor& proc;
    float s = 1.0f;
    juce::ComboBox boxA, boxB;
    juce::Slider mix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aMix;
};

// Controles do EQ dinâmico (barra lateral)
class DynPanel : public juce::Component
{
public:
    explicit DynPanel (RefProProcessor& p);
    ~DynPanel() override;
    void setScale (float v) { s = v; }
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr float baseHeight = 228.0f;

private:
    RefProProcessor& proc;
    float s = 1.0f;
    bool lastOn = true;
    juce::Slider amount, maxCut, maxBoost, tol, adapt;
    juce::ComboBox speed;
    FlatButton power { "power" }, comp { "comp" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aAmount, aMax, aBoost, aTol, aAdapt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aSpeed;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aComp, aPower;
};

// Medidores de análise: Vocal Balance / Dynamics / Stereo Width
class MetersPanel : public juce::Component, private juce::Timer
{
public:
    explicit MetersPanel (RefProProcessor& p);
    ~MetersPanel() override;

    void setTarget (const TargetData& t) { target = t; repaint(); }
    void setScale (float v)  { s = v; }
    void setValues (bool v)  { showValues = v; repaint(); }
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }
    void drawMeter (juce::Graphics&, juce::Rectangle<float> col, int kind, float dev, bool valid, const juce::String& hint) const;

    RefProProcessor& proc;
    TonalAnalyzer& an;
    TargetData target;
    float s = 1.0f;
    bool showValues = false;
    FlatButton autoBtn { "auto" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAttach;
};

class RefProEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RefProEditor (RefProProcessor&);
    ~RefProEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void syncTarget (bool force);
    void rebuildList();
    void selectTarget (int idx);
    int  currentIndex() const;
    void showTargetMenu();
    void showSettingsMenu();
    void showManageMenu();
    void showTargetContext (const TargetList::Item&);
    void doCapture();
    void selectBand (int b);
    void layoutBandPanel();
    void layoutSidebar();
    void resetEq();
    void analyzeFiles (const juce::Array<juce::File>& files, const juce::String& name);

    RefLookAndFeel laf;
    RefProProcessor& proc;
    float s = 1.0f;

    juce::Viewport listViewport;
    TargetList  list;
    juce::TextEditor search;
    GraphView   graph;
    BandPanel   bandPanel;
    BlendPanel  blendPanel;
    DynPanel    dynPanel;
    MetersPanel meters;
    FlatButton  viewCurve { "curve" }, viewBars { "bars" }, viewLeveled { "leveled" }, targetPill { "target" },
                captureBtn { "capture" }, settingsBtn { "settings" }, extraBtn { "extra" }, eqBtn { "eq" },
                favFilter { "favs" }, manageBtn { "manage" }, abA { "A" }, abB { "B" };
    juce::Slider outGain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> eqAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outAttach;
    std::unique_ptr<juce::FileChooser> fileChooser;

    TargetData current;
    int cachedIndex = -1, cachedVersion = -1, cachedBlend = -1, selectedBand = -1;
    bool showExtra = false, barsView = false;
    juce::Rectangle<float> statusArea, msgArea, stripArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RefProEditor)
};
