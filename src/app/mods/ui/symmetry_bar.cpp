// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/ui/symmetry_bar.h"

#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/mods/symmetry/symmetry_settings.h"
#include "app/pref/preferences.h"
#include "app/tools/symmetry.h"
#include "app/ui/button_set.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "ui/combobox.h"
#include "ui/int_entry.h"
#include "ui/label.h"
#include "ui/menu.h"
#include "ui/tooltips.h"

#include <algorithm>

namespace app { namespace mods {

using namespace ui;

namespace {

// The order of the entries in the mode list.
enum Choice {
  kOff,
  kHorizontal,
  kVertical,
  kDual,
  kDiagonal45,
  kDiagonalMinus45,
  kBothDiagonals,
  kEightWay,
  kCenter,
  kRadial,
  kMandala,
  kSpiral,
  kChoiceCount,
};

// The built-in mode behind each of the first entries.
constexpr gen::SymmetryMode kOfficialModes[] = {
  gen::SymmetryMode::NONE,       gen::SymmetryMode::HORIZONTAL, gen::SymmetryMode::VERTICAL,
  gen::SymmetryMode::BOTH,       gen::SymmetryMode::RIGHT_DIAG, gen::SymmetryMode::LEFT_DIAG,
  gen::SymmetryMode::BOTH_DIAG,  gen::SymmetryMode::ALL,
};

std::string choice_text(const int choice)
{
  switch (choice) {
    case kOff:             return Strings::symmetry_mods_off();
    case kHorizontal:      return Strings::symmetry_mods_horizontal();
    case kVertical:        return Strings::symmetry_mods_vertical();
    case kDual:            return Strings::symmetry_mods_dual();
    case kDiagonal45:      return Strings::symmetry_mods_diagonal_45();
    case kDiagonalMinus45: return Strings::symmetry_mods_diagonal_minus_45();
    case kBothDiagonals:   return Strings::symmetry_mods_both_diagonals();
    case kEightWay:        return Strings::symmetry_mods_eight_way();
    case kCenter:          return Strings::symmetry_mods_center();
    case kRadial:          return Strings::symmetry_mods_radial();
    case kMandala:         return Strings::symmetry_mods_mandala();
    case kSpiral:          return Strings::symmetry_mods_spiral();
  }
  return std::string();
}

int choice_for_mods_mode(const SymMode mode)
{
  switch (mode) {
    case SymMode::Center:  return kCenter;
    case SymMode::Radial:  return kRadial;
    case SymMode::Mandala: return kMandala;
    case SymMode::Spiral:  return kSpiral;
    case SymMode::Official:
    default:               return kOff;
  }
}

Doc* active_doc()
{
  return UIContext::instance()->activeDocument();
}

} // anonymous namespace

// A number field that tells us when it changes.
class SymmetryCountEntry : public IntEntry {
public:
  SymmetryCountEntry(const int min, const int max) : IntEntry(min, max) {}

  obs::signal<void()> ValueChanged;

protected:
  void onValueChange() override
  {
    IntEntry::onValueChange();
    ValueChanged();
  }
};

// The "..." that opens the menu for moving the center.
class SymmetryOptionsButton : public ButtonSet {
public:
  SymmetryOptionsButton() : ButtonSet(1)
  {
    auto theme = skin::SkinTheme::get(this);
    addItem("...", theme->styles.symmetryOptions());
  }

  obs::signal<void()> Clicked;

protected:
  void onItemChange(Item* item) override
  {
    ButtonSet::onItemChange(item);
    if (at(0)->isSelected()) {
      at(0)->setSelected(false);
      Clicked();
    }
  }
};

SymmetryBar::SymmetryBar(TooltipManager* tooltipManager) : m_tooltips(tooltipManager)
{
  setExpansive(true);

  // Takes all the spare room, which pushes the controls to the right edge.
  // Not a bare Widget: a generic widget paints itself black.
  addChild(new BoxFiller());

  addChild(m_mode = new ComboBox());
  for (int i = 0; i < kChoiceCount; ++i)
    m_mode->addItem(choice_text(i));

  addChild(m_countLabel = new Label(Strings::symmetry_mods_count()));
  addChild(m_count = new SymmetryCountEntry(kMinSymCount, kMaxSymCount));
  addChild(m_angleLabel = new Label(Strings::symmetry_mods_angle()));
  addChild(m_angle = new SymmetryCountEntry(-180, 180));
  addChild(m_options = new SymmetryOptionsButton());

  m_mode->Change.connect([this] { applyChoice(m_mode->getSelectedItemIndex()); });
  m_count->ValueChanged.connect([this] { applyCount(m_count->getValue()); });
  m_angle->ValueChanged.connect([this] { applyAngle(m_angle->getValue()); });
  m_options->Clicked.connect([this] { showPositionMenu(); });

  UIContext::instance()->add_observer(this);

  m_enabledConn = Preferences::instance().symmetryMode.enabled.AfterChange.connect(
    [this] { refresh(); });
  m_languageConn = Strings::instance()->LanguageChange.connect([this] {
    retranslate();
    if (parent())
      parent()->layout(); // The new texts may be wider or narrower.
  });

  retranslate();
  refresh();
}

SymmetryBar::~SymmetryBar()
{
  UIContext::instance()->remove_observer(this);
}

void SymmetryBar::onActiveSiteChange(const Site&)
{
  refresh();
}

void SymmetryBar::onInitTheme(InitThemeEvent& ev)
{
  HBox::onInitTheme(ev);
  updateMinSize();
}

// This bar is expansive so that it sits at the right edge, but a box that runs
// out of room takes the shortfall from its expansive children first, which
// would squeeze the controls to nothing. Never let it get smaller than they need.
void SymmetryBar::updateMinSize()
{
  setMinSize(gfx::Size(0, 0));
  setMinSize(sizeHint());
}

void SymmetryBar::retranslate()
{
  const bool wasUpdating = m_updating;
  m_updating = true;

  for (int i = 0; i < kChoiceCount; ++i)
    m_mode->setItemText(i, choice_text(i));
  // The text shown for the selected item is a copy; setItemText() leaves it.
  const int selected = m_mode->getSelectedItemIndex();
  if (selected >= 0)
    m_mode->getEntryWidget()->setText(choice_text(selected));
  m_countLabel->setText(Strings::symmetry_mods_count());
  m_angleLabel->setText(Strings::symmetry_mods_angle());

  m_tooltips->addTooltipFor(m_mode, Strings::symmetry_mods_tooltip_mode(), BOTTOM);
  m_tooltips->addTooltipFor(m_count, Strings::symmetry_mods_tooltip_count(), BOTTOM);
  m_tooltips->addTooltipFor(m_angle,
                            symmetry_settings().mode == SymMode::Spiral ?
                              Strings::symmetry_mods_tooltip_angle_spiral() :
                              Strings::symmetry_mods_tooltip_angle_mandala(),
                            BOTTOM);
  m_tooltips->addTooltipFor(m_options->at(0), Strings::symmetry_show_options(), BOTTOM);

  m_updating = wasUpdating;
}

// Makes the controls say what is currently in effect.
void SymmetryBar::refresh()
{
  const SymmetrySettings& s = symmetry_settings();
  Doc* doc = active_doc();

  int choice = kOff;
  if (Preferences::instance().symmetryMode.enabled()) {
    if (s.mode != SymMode::Official) {
      choice = choice_for_mods_mode(s.mode);
    }
    else if (doc) {
      // Some combinations of built-in modes are drawn as the eight-way one.
      const gen::SymmetryMode mode = tools::Symmetry::resolveMode(
        Preferences::instance().document(doc).symmetry.mode());
      for (int i = kHorizontal; i <= kEightWay; ++i) {
        if (kOfficialModes[i] == mode)
          choice = i;
      }
    }
  }

  const bool hasCount = (choice == kRadial || choice == kMandala || choice == kSpiral);
  const bool hasAngle = (choice == kMandala || choice == kSpiral);

  m_updating = true;
  m_mode->setSelectedItemIndex(choice);
  m_count->setValue(s.count);
  m_angle->setValue(s.mode == SymMode::Spiral ? s.spiralAngle : s.mandalaAngle);
  m_updating = false;

  const bool changed = (m_countLabel->isVisible() != hasCount ||
                        m_angleLabel->isVisible() != hasAngle);
  m_countLabel->setVisible(hasCount);
  m_count->setVisible(hasCount);
  m_angleLabel->setVisible(hasAngle);
  m_angle->setVisible(hasAngle);
  m_options->setVisible(choice != kOff);
  m_mode->setEnabled(doc != nullptr);

  // The angle means something different for each mode.
  m_tooltips->addTooltipFor(m_angle,
                            s.mode == SymMode::Spiral ?
                              Strings::symmetry_mods_tooltip_angle_spiral() :
                              Strings::symmetry_mods_tooltip_angle_mandala(),
                            BOTTOM);

  updateMinSize();
  if (changed && parent())
    parent()->layout();
}

void SymmetryBar::applyChoice(const int choice)
{
  if (m_updating || choice < 0 || choice >= kChoiceCount)
    return;

  auto& pref = Preferences::instance();
  SymmetrySettings& s = symmetry_settings();
  Doc* doc = active_doc();

  if (choice == kOff) {
    // Only switch it off: switching it back on, here or with the built-in
    // command, brings back whatever was chosen last.
    pref.symmetryMode.enabled(false);
  }
  else if (choice <= kEightWay) {
    s.mode = SymMode::Official;
    save_symmetry_settings();
    if (doc)
      pref.document(doc).symmetry.mode(kOfficialModes[choice]);
    pref.symmetryMode.enabled(true);
  }
  else {
    switch (choice) {
      case kCenter:  s.mode = SymMode::Center; break;
      case kRadial:  s.mode = SymMode::Radial; break;
      case kMandala: s.mode = SymMode::Mandala; break;
      case kSpiral:  s.mode = SymMode::Spiral; break;
    }
    save_symmetry_settings();
    pref.symmetryMode.enabled(true);
  }

  refresh();
  if (doc)
    doc->notifyGeneralUpdate(); // Redraw the guides.
}

void SymmetryBar::applyCount(const int count)
{
  if (m_updating)
    return;

  symmetry_settings().count = std::clamp(count, kMinSymCount, kMaxSymCount);
  save_symmetry_settings();
  if (Doc* doc = active_doc())
    doc->notifyGeneralUpdate();
}

void SymmetryBar::applyAngle(const int angle)
{
  if (m_updating)
    return;

  SymmetrySettings& s = symmetry_settings();
  const int value = std::clamp(angle, -180, 180);
  if (s.mode == SymMode::Spiral)
    s.spiralAngle = value;
  else
    s.mandalaAngle = value;
  save_symmetry_settings();
  if (Doc* doc = active_doc())
    doc->notifyGeneralUpdate();
}

void SymmetryBar::showPositionMenu()
{
  Doc* doc = active_doc();
  if (!doc)
    return;

  DocumentPreferences& docPref = Preferences::instance().document(doc);

  Menu menu;
  MenuItem resetToCenter(Strings::symmetry_reset_position());
  MenuItem resetToViewCenter(Strings::symmetry_reset_position_to_view_center());
  menu.addChild(&resetToCenter);
  menu.addChild(&resetToViewCenter);

  resetToCenter.Click.connect([doc, &docPref] {
    docPref.symmetry.xAxis(doc->sprite()->width() / 2.0);
    docPref.symmetry.yAxis(doc->sprite()->height() / 2.0);
    doc->notifyGeneralUpdate();
  });

  resetToViewCenter.Click.connect([doc, &docPref] {
    if (auto* editor = Editor::activeEditor()) {
      const gfx::Rect& bounds = editor->getViewportBounds();
      docPref.symmetry.xAxis(bounds.x + bounds.w / 2.0);
      docPref.symmetry.yAxis(bounds.y + bounds.h / 2.0);
      doc->notifyGeneralUpdate();
    }
  });

  const gfx::Rect bounds = m_options->bounds();
  menu.showPopup(gfx::Point(bounds.x, bounds.y2()), display());
}

}} // namespace app::mods
