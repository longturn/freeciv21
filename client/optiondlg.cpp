/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

// Qt
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QSpinBox>
// utility
#include "log.h"
// client
#include "fc_client.h"
#include "optiondlg.h"
#include "options.h"
#include "page_pregame.h"

enum {
  RESPONSE_CANCEL,
  RESPONSE_OK,
  RESPONSE_APPLY,
  RESPONSE_RESET,
  RESPONSE_REFRESH,
  RESPONSE_SAVE
};

// global value to store pointers to opened config dialogs
QHash<const struct option_set *, option_dialog *> dialog_list;

/**
   Splits long text to 80 characters
 */
QString split_text(const QString &text, bool cut)
{
  QStringList sl;
  QString st, str, result;
  int i;
  int j = 0;

  sl = text.split(QStringLiteral("\n"));
  for (const QString &s : qAsConst(sl)) {
    st = s;
    while (st.count() >= 80) {
      str = st.left(80);
      i = str.lastIndexOf(' ');
      if (i == -1) {
        i = 80;
      }
      result = result + str.left(i) + '\n';
      /* Skip last space - at (i + 1)
         unless there there was no space */
      if (i != 80) {
        st.remove(0, i + 1);
      } else {
        st.remove(0, i);
      }
    }
    str = st;
    if (str.left(str.count()) != QLatin1String("")) {
      result = result + str.left(str.count()) + '\n';
    }
    j++;
    if (j >= 12 && cut) {
      result = result + _("... read more in help") + "\n";
      break;
    }
  }
  result.remove(result.lastIndexOf('\n'), 1);
  return result;
}

/**
   Remove some text from given text(help text) to show as tooltip
 */
QString cut_helptext(const QString &text)
{
  QStringList sl;
  QString ret_str;

  // Remove all lines from help which has '*' in first 3 chars
  sl = text.split('\n');
  for (const QString &s : qAsConst(sl)) {
    if (s.count() > 2) {
      if (s.at(0) != '*' && s.at(1) != '*' && s.at(2) != '*') {
        ret_str = ret_str + s + '\n';
      }
    } else {
      ret_str = ret_str + s + '\n';
    }
  }
  return ret_str;
}

/**
   Constructor for options dialog.
 */
option_dialog::option_dialog(const QString &name, const option_set *options,
                             QWidget *parent)
    : qfc_dialog(parent)
{
  setWindowFlag(Qt::Tool); // Stays on top without blocking input

  QPushButton *but;

  curr_options = options;
  setWindowTitle(name);
  tab_widget = new QTabWidget;

  button_box = new QDialogButtonBox();
  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton),
                        _("Cancel"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  QObject::connect(but, &QPushButton::clicked,
                   [this]() { apply_option(RESPONSE_CANCEL); });

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogResetButton),
                        _("Reset"));
  button_box->addButton(but, QDialogButtonBox::ResetRole);
  QObject::connect(but, &QPushButton::clicked,
                   [this]() { apply_option(RESPONSE_RESET); });

  but = new QPushButton(QIcon::fromTheme(QStringLiteral("view-refresh")),
                        _("Refresh"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  QObject::connect(but, &QPushButton::clicked,
                   [this]() { apply_option(RESPONSE_REFRESH); });

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogApplyButton),
                        _("Apply"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  QObject::connect(but, &QPushButton::clicked,
                   [this]() { apply_option(RESPONSE_APPLY); });

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogSaveButton),
                        _("Save"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  QObject::connect(but, &QPushButton::clicked,
                   [this]() { apply_option(RESPONSE_SAVE); });

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton),
                        _("Ok"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  QObject::connect(but, &QPushButton::clicked,
                   [this]() { apply_option(RESPONSE_OK); });

  main_layout = new QVBoxLayout;
  main_layout->addWidget(tab_widget);
  categories.clear();
  fill(options);
  main_layout->addWidget(button_box);
  setLayout(main_layout);

  resize(800, 700);

  setAttribute(Qt::WA_DeleteOnClose);
}

/**
   Destructor for options dialog.
 */
option_dialog::~option_dialog()
{
  while (::dialog_list.contains(curr_options)) {
    ::dialog_list.remove(curr_options);
  }
  destroy();
}

/**
   Apply desired action depending on user's request (clicked button).
 */
void option_dialog::apply_option(int response)
{
  switch (response) {
  case RESPONSE_APPLY:
    apply_options();
    break;
  case RESPONSE_CANCEL:
    ::dialog_list.remove(curr_options);
    close();
    break;
  case RESPONSE_OK:
    apply_options();
    ::dialog_list.remove(curr_options);
    close();
    break;
  case RESPONSE_SAVE:
    desired_settable_options_update();
    options_save(nullptr);
    break;
  case RESPONSE_RESET:
    full_reset();
    break;
  case RESPONSE_REFRESH:
    full_refresh();
    break;
  }
}

/**
   Return selected colors (for highlighting chat).
 */
void option_dialog::get_color(struct option *poption, QByteArray &a1,
                              QByteArray &a2)
{
  QPalette pal;
  QColor col1, col2;
  QWidget *w;
  QPushButton *but;

  w = reinterpret_cast<QPushButton *>(option_get_gui_data(poption));
  but = w->findChild<QPushButton *>(QStringLiteral("text_color"));
  pal = but->palette();
  col1 = pal.color(QPalette::Button);
  but = w->findChild<QPushButton *>(QStringLiteral("text_background"));
  pal = but->palette();
  col2 = pal.color(QPalette::Button);
  a1 = col1.name().toUtf8();
  a2 = col2.name().toUtf8();
}

/**
   Apply all options.
 */
void option_dialog::apply_options()
{
  QByteArray ba1, ba2;

  options_iterate(curr_options, poption)
  {
    switch (option_type(poption)) {
    case OT_BOOLEAN:
      option_bool_set(poption, get_bool(poption));
      break;
    case OT_INTEGER:
      option_int_set(poption, get_int(poption));
      break;
    case OT_STRING:
      option_str_set(poption, get_string(poption).constData());
      break;
    case OT_ENUM:
      option_enum_set_int(poption, get_enum(poption));
      break;
    case OT_BITWISE:
      option_bitwise_set(poption, get_bitwise(poption));
      break;
    case OT_FONT:
      option_font_set(poption, get_button_font(poption));
      break;
    case OT_COLOR:
      get_color(poption, ba1, ba2);
      option_color_set(poption, ft_color_construct(ba1.data(), ba2.data()));
      break;
    }
  }
  options_iterate_end;
}

/**
   Set the boolean value of the option.
 */
void option_dialog::set_bool(struct option *poption, bool value)
{
  QCheckBox *c;

  c = reinterpret_cast<QCheckBox *>(option_get_gui_data(poption));
  if (value) {
    c->setCheckState(Qt::Checked);
  } else {
    c->setCheckState(Qt::Unchecked);
  }
}

/**
   Get the boolean value from checkbox.
 */
bool option_dialog::get_bool(struct option *poption)
{
  QCheckBox *c;

  c = reinterpret_cast<QCheckBox *>(option_get_gui_data(poption));
  return c->checkState() == Qt::Checked;
}

/**
   Set the integer value of the option.
 */
void option_dialog::set_int(struct option *poption, int value)
{
  QSpinBox *s;

  s = reinterpret_cast<QSpinBox *>(option_get_gui_data(poption));
  s->setValue(value);
}

/**
   Sets desired font name in button text and sets global font for that option
   That function is not executed when user changes font, but when applying or
   resetting options.
 */
void option_dialog::set_font(struct option *poption, const QFont &font)
{
  qApp->processEvents();
  auto qp = reinterpret_cast<QPushButton *>(option_get_gui_data(poption));
  qp->setText(
      QStringLiteral("%1 %2").arg(font.family()).arg(font.pointSize()));
  qp->setFont(font);
}

/**
   Get int value from spinbox.
 */
int option_dialog::get_int(struct option *poption)
{
  QSpinBox *s;

  s = reinterpret_cast<QSpinBox *>(option_get_gui_data(poption));
  return s->value();
}

/**
   Set the string value of the option.
 */
void option_dialog::set_string(struct option *poption, const char *string)
{
  int i;
  QComboBox *cb;
  QLineEdit *le;

  if (option_str_values(poption) != nullptr) {
    cb = reinterpret_cast<QComboBox *>(option_get_gui_data(poption));
    i = cb->findText(string);
    if (i != -1) {
      cb->setCurrentIndex(i);
    }
  } else {
    le = reinterpret_cast<QLineEdit *>(option_get_gui_data(poption));
    le->setText(string);
  }
}

/**
   Get string for desired option from combobox or lineedit.
 */
QByteArray option_dialog::get_string(struct option *poption)
{
  QComboBox *cb;
  QLineEdit *le;

  if (option_str_values(poption) != nullptr) {
    cb = reinterpret_cast<QComboBox *>(option_get_gui_data(poption));
    return cb->currentText().toUtf8();
  } else {
    le = reinterpret_cast<QLineEdit *>(option_get_gui_data(poption));
    return le->displayText().toUtf8();
  }
}

/**
   Set desired index(text) in combobox.
 */
void option_dialog::set_enum(struct option *poption, int index)
{
  QComboBox *cb;

  cb = reinterpret_cast<QComboBox *>(option_get_gui_data(poption));
  cb->setCurrentIndex(index);
}

/**
   Get indexed value from combobox.
 */
int option_dialog::get_enum(struct option *poption)
{
  QComboBox *cb;

  cb = reinterpret_cast<QComboBox *>(option_get_gui_data(poption));
  return cb->currentIndex();
}

/**
   Set the enum value of the option.
 */
void option_dialog::set_bitwise(struct option *poption, unsigned int value)
{
  QGroupBox *gb;
  int i;
  QList<QCheckBox *> check_buttons;

  gb = reinterpret_cast<QGroupBox *>(option_get_gui_data(poption));
  check_buttons = gb->findChildren<QCheckBox *>();

  for (i = 0; i < check_buttons.count(); i++) {
    if (value & (1 << i)) {
      check_buttons[i]->setCheckState(Qt::Checked);
    } else {
      check_buttons[i]->setCheckState(Qt::Unchecked);
    }
  }
}

/**
   Return the enum value from groupbox.
 */
unsigned int option_dialog::get_bitwise(struct option *poption)
{
  QGroupBox *gb;
  int i;
  unsigned int value = 0;
  QList<QCheckBox *> check_buttons;

  gb = reinterpret_cast<QGroupBox *>(option_get_gui_data(poption));
  check_buttons = gb->findChildren<QCheckBox *>();

  for (i = 0; i < check_buttons.count(); i++) {
    if (check_buttons[i]->checkState() == Qt::Checked) {
      value |= 1 << i;
    }
  }
  return value;
}

/**
   Find option indicating colors.
 */
struct option *option_dialog::get_color_option()
{
  options_iterate(curr_options, poption)
  {
    if (option_type(poption) == OT_COLOR) {
      return poption;
    }
  }
  options_iterate_end;
  return nullptr;
}

/**
   Set color of the buttons depending on given colors.
 */
void option_dialog::set_color(struct option *poption, struct ft_color color)
{
  QColor col;
  QWidget *w;
  QPushButton *but;
  QString s1 = QStringLiteral("QPushButton { background-color: ");
  QString s2 = QStringLiteral(";}");

  w = reinterpret_cast<QPushButton *>(option_get_gui_data(poption));
  but = w->findChild<QPushButton *>(QStringLiteral("text_color"));
  if (nullptr != but && nullptr != color.foreground
      && '\0' != color.foreground[0]) {
    col.setNamedColor(color.foreground);
    but->setStyleSheet(s1 + col.name() + s2);
  }
  but = w->findChild<QPushButton *>(QStringLiteral("text_background"));
  if (nullptr != but && nullptr != color.background
      && '\0' != color.background[0]) {
    col.setNamedColor(color.background);
    but->setStyleSheet(s1 + col.name() + s2);
  }
}

/**
   Refresh one given option for option dialog.
 */
void option_dialog::option_dialog_refresh(struct option *poption)
{
  switch (option_type(poption)) {
  case OT_BOOLEAN:
    set_bool(poption, option_bool_get(poption));
    break;
  case OT_INTEGER:
    set_int(poption, option_int_get(poption));
    break;
  case OT_STRING:
    set_string(poption, option_str_get(poption));
    break;
  case OT_ENUM:
    set_enum(poption, option_enum_get_int(poption));
    break;
  case OT_BITWISE:
    set_bitwise(poption, option_bitwise_get(poption));
    break;
  case OT_FONT:
    set_font(poption, option_font_get(poption));
    break;
  case OT_COLOR:
    set_color(poption, option_color_get(poption));
    break;
  }
}

/**
   Refresh all options.
 */
void option_dialog::full_refresh()
{
  options_iterate(curr_options, poption) { option_dialog_refresh(poption); }
  options_iterate_end;
}

/**
   Reset all options.
 */
void option_dialog::full_reset()
{
  options_iterate(curr_options, poption) { option_dialog_reset(poption); }
  options_iterate_end;
}

/**
   Reset one option.
 */
void option_dialog::option_dialog_reset(struct option *poption)
{
  switch (option_type(poption)) {
  case OT_BOOLEAN:
    set_bool(poption, option_bool_def(poption));
    break;
  case OT_INTEGER:
    set_int(poption, option_int_def(poption));
    break;
  case OT_STRING:
    set_string(poption, option_str_def(poption));
    break;
  case OT_ENUM:
    set_enum(poption, option_enum_def_int(poption));
    break;
  case OT_BITWISE:
    set_bitwise(poption, option_bitwise_def(poption));
    break;
  case OT_FONT:
    set_font(poption, option_font_def(poption));
    break;
  case OT_COLOR:
    set_color(poption, option_color_def(poption));
    break;
  }
}

/**
   Create all widgets.
 */
void option_dialog::fill(const struct option_set *poptset)
{
  options_iterate(poptset, poption) { add_option(poption); }
  options_iterate_end;
}

/**
   Create widget for option.
 */
void option_dialog::add_option(struct option *poption)
{
  QWidget *widget;
  QWidget *lwidget;
  QWidget *twidget;
  QString category_name, description, qstr;
  const QVector<QString> *values;
  QVBoxLayout *twidget_layout;
  QHBoxLayout *hbox_layout;
  QVBoxLayout *vbox_layout;
  QLabel *label;
  QScrollArea *scroll;
  QSpinBox *spin;
  QComboBox *combo;
  QLineEdit *edit;
  QGroupBox *group;
  QCheckBox *check;
  QPushButton *button;
  int min, max, i;
  unsigned int j;

  category_name = option_category_name(poption);
  widget = nullptr;

  if (!categories.contains(category_name)) {
    twidget = new QWidget();
    twidget->setProperty("doomed", true);
    scroll = new QScrollArea();
    scroll->setProperty("doomed", true);
    scroll->setWidgetResizable(true);
    twidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    twidget_layout = new QVBoxLayout();
    twidget_layout->setSpacing(0);
    twidget->setLayout(twidget_layout);
    scroll->setWidget(twidget);
    tab_widget->addTab(scroll, category_name);
    categories.append(category_name);
    widget_map[category_name] = twidget;
  } else {
    twidget = widget_map[category_name];
  }

  description = option_description(poption);
  switch (option_type(poption)) {
  case OT_BOOLEAN:
    widget = new QCheckBox();
    break;

  case OT_INTEGER:
    min = option_int_min(poption);
    max = option_int_max(poption);
    spin = new QSpinBox();
    spin->setMinimum(min);
    spin->setMaximum(max);
    spin->setSingleStep(MAX((max - min) / 50, 1));
    widget = spin;
    break;

  case OT_STRING:
    values = option_str_values(poption);
    if (nullptr != values) {
      combo = new QComboBox();
      for (const auto &value : *values) {
        combo->addItem(value);
      }
      widget = combo;
    } else {
      edit = new QLineEdit();
      widget = edit;
    }
    break;

  case OT_ENUM:
    combo = new QComboBox();
    i = 0;
    while (true) {
      QString s = option_enum_int_to_str(poption, i);
      if (s.isEmpty()) {
        break;
      }
      // we store enum value in QVariant
      combo->addItem(_(qUtf8Printable(s)), i);
      i++;
    }
    widget = combo;
    break;

  case OT_BITWISE:
    group = new QGroupBox();
    values = option_bitwise_values(poption);
    vbox_layout = new QVBoxLayout();
    for (j = 0; j < values->count(); j++) {
      check = new QCheckBox(_(qUtf8Printable(values->at(j))));
      vbox_layout->addWidget(check);
    }
    group->setLayout(vbox_layout);
    widget = group;
    break;

  case OT_FONT: {
    button = new QPushButton();
    auto font = option_font_get(poption);
    button->setFont(font);
    button->setText(
        QStringLiteral("%1 %2").arg(font.family()).arg(font.pointSize()));
    connect(button, &QAbstractButton::clicked, this,
            QOverload<>::of(&option_dialog::set_font));
    widget = button;
  } break;

  case OT_COLOR:
    button = new QPushButton();
    button->setToolTip(_("Select the text color"));
    button->setObjectName(QStringLiteral("text_color"));
    button->setAutoFillBackground(true);
    button->setAutoDefault(false);
    connect(button, &QAbstractButton::clicked, this,
            QOverload<>::of(&option_dialog::set_color));
    hbox_layout = new QHBoxLayout();
    hbox_layout->addWidget(button);
    button = new QPushButton();
    button->setToolTip(_("Select the background color"));
    button->setObjectName(QStringLiteral("text_background"));
    button->setAutoFillBackground(true);
    button->setAutoDefault(false);
    connect(button, &QAbstractButton::clicked, this,
            QOverload<>::of(&option_dialog::set_color));
    hbox_layout->addWidget(button);
    widget = new QWidget();
    widget->setLayout(hbox_layout);
    break;
  }

  if (widget != nullptr) {
    hbox_layout = new QHBoxLayout();
    hbox_layout->setAlignment(Qt::AlignRight);
    label = new QLabel(description);
    label->setToolTip(split_text(option_help_text(poption), false));
    hbox_layout->addWidget(label, 1, Qt::AlignLeft);
    hbox_layout->addStretch();
    hbox_layout->addWidget(widget, 1, Qt::AlignRight);
    lwidget = new QWidget();
    lwidget->setLayout(hbox_layout);
    twidget_layout = qobject_cast<QVBoxLayout *>(twidget->layout());
    twidget_layout->addWidget(lwidget);
  }

  widget->setEnabled(option_is_changeable(poption));
  widget->setToolTip(split_text(option_help_text(poption), false));
  option_set_gui_data(poption, widget);
  option_dialog_refresh(poption);
}

/**
   Popup the option dialog for the option set.
 */
void option_dialog_popup(const char *name, const struct option_set *poptset)
{
  option_dialog *opt_dialog;

  if (::dialog_list.contains(poptset)) {
    opt_dialog = dialog_list[poptset];
    opt_dialog->show();
  } else {
    opt_dialog = new option_dialog(name, poptset, king()->central_wdg);
    ::dialog_list.insert(poptset, opt_dialog);
    opt_dialog->show();
  }
}

/**
   Sets font and text in pushbutton (user just chosen font)
 */
void option_dialog::set_font()
{
  QStringList ql;
  bool ok;
  QFont qf;
  QPushButton *pb;

  pb = (QPushButton *) QObject::sender();
  qf = pb->font();
  qf = QFontDialog::getFont(&ok, qf, this);
  pb->setFont(qf);
  ql = qf.toString().split(QStringLiteral(","));
  pb->setText(ql[0] + " " + ql[1]);
}

/**
   Get font from pushbutton.
 */
QFont option_dialog::get_button_font(struct option *poption)
{
  return reinterpret_cast<QPushButton *>(option_get_gui_data(poption))
      ->font();
}

/**
   Set color of buttons (user just changed colors).
 */
void option_dialog::set_color()
{
  QPushButton *but;
  QColor color, c;
  struct option *color_option;
  struct ft_color ft_color;
  QPalette pal;

  color_option = get_color_option();
  ft_color = option_color_get(color_option);
  but = qobject_cast<QPushButton *>(QObject::sender());

  if (but->objectName() == QLatin1String("text_color")) {
    c.setNamedColor(ft_color.foreground);
    color = QColorDialog::getColor(c, this);
    if (color.isValid()) {
      pal.setColor(QPalette::Button, color);
      but->setPalette(pal);
    }
  } else if (but->objectName() == QLatin1String("text_background")) {
    c.setNamedColor(ft_color.background);
    color = QColorDialog::getColor(c, this);
    if (color.isValid()) {
      pal.setColor(QPalette::Button, color);
      but->setPalette(pal);
    }
  }
}

/**
   Popdown the option dialog for the option set.
 */
void option_dialog_popdown(const struct option_set *poptset)
{
  option_dialog *opt_dialog;

  while (::dialog_list.contains(poptset)) {
    opt_dialog = ::dialog_list[poptset];
    opt_dialog->close();
    ::dialog_list.remove(poptset);
  }
}

/**
   Update the GUI for the option.
 */
void option_gui_update(struct option *poption)
{
  option_dialog *dial;

  if (::dialog_list.contains(option_optset(poption))) {
    dial = ::dialog_list[option_optset(poption)];
    dial->option_dialog_refresh(poption);
  }

  if (option_optset(poption) == server_optset) {
    if (strcmp(option_name(poption), "nationset") == 0) {
      update_nationset_combo();
    }
    if (strcmp(option_name(poption), "aifill") == 0) {
      // sveinung
      // qobject_cast<page_pregame
      // *>(king()->pages[PAGE_START])->pr_options->set_aifill(option_int_get(poption));
    }
  }
}
