
#include "page_main.h"
// Qt
#include <QApplication>
#include <QGridLayout>
#include <QPainter>
// utility
#include "fcintl.h"
// common
#include "version.h"
// gui-qt
#include "colors.h"
#include "dialogs.h"
#include "fc_client.h"

/**********************************************************************/ /**
   Creates buttons and layouts for start page.
 **************************************************************************/
void fc_client::create_main_page(void)
{
  QPixmap main_graphics(tileset_main_intro_filename(tileset));
  QLabel *free_main_pic = new QLabel;
  QPainter painter(&main_graphics);
  QStringList buttons_names;
  int buttons_nr;
  char msgbuf[128];
  const char *rev_ver;
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  int row = 0;
#if IS_BETA_VERSION
  QPalette warn_color;
  QLabel *beta_label = new QLabel(beta_message());
#endif /* IS_BETA_VERSION */

  pages_layout[PAGE_MAIN] = new QGridLayout;

  painter.setPen(Qt::white);

  rev_ver = fc_git_revision();

  if (rev_ver == NULL) {
    /* TRANS: "version 2.6.0, Qt client" */
    fc_snprintf(msgbuf, sizeof(msgbuf), _("%s%s, Qt client"), word_version(),
                VERSION_STRING);
  } else {
    fc_snprintf(msgbuf, sizeof(msgbuf), "%s%s", word_version(),
                VERSION_STRING);
    painter.drawText(
        10, main_graphics.height() - fm.descent() - fm.height() * 2, msgbuf);

    /* TRANS: "commit: [modified] <git commit id>" */
    fc_snprintf(msgbuf, sizeof(msgbuf), _("commit: %s"), rev_ver);
    painter.drawText(10, main_graphics.height() - fm.descent() - fm.height(),
                     msgbuf);

    strncpy(msgbuf, _("Qt client"), sizeof(msgbuf) - 1);
  }

  painter.drawText(main_graphics.width() - fm.horizontalAdvance(msgbuf) - 10,
                   main_graphics.height() - fm.descent(), msgbuf);
  free_main_pic->setPixmap(main_graphics);
  pages_layout[PAGE_MAIN]->addWidget(free_main_pic, row++, 0, 1, 2,
                                     Qt::AlignCenter);

#if IS_BETA_VERSION
  warn_color.setColor(QPalette::WindowText, Qt::red);
  beta_label->setPalette(warn_color);
  beta_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
  beta_label->setAlignment(Qt::AlignCenter);
  pages_layout[PAGE_MAIN]->addWidget(beta_label, row++, 0, 1, 2,
                                     Qt::AlignHCenter);
#endif

  buttons_names << _("Start new game") << _("Start scenario game")
                << _("Load saved game") << _("Connect to network game")
                << _("Options") << _("Quit");

  buttons_nr = buttons_names.count();

  for (int iter = 0; iter < buttons_nr; iter++) {
    button = new QPushButton(buttons_names[iter]);

    switch (iter) {
    case 0:
      pages_layout[PAGE_MAIN]->addWidget(button, row + 1, 0);
      connect(button, &QAbstractButton::clicked, this,
              &fc_client::start_new_game);
      break;
    case 1:
      pages_layout[PAGE_MAIN]->addWidget(button, row + 2, 0);
      QObject::connect(button, &QPushButton::clicked,
                       [this]() { switch_page(PAGE_SCENARIO); });
      break;
    case 2:
      pages_layout[PAGE_MAIN]->addWidget(button, row + 3, 0);
      QObject::connect(button, &QPushButton::clicked,
                       [this]() { switch_page(PAGE_LOAD); });
      break;
    case 3:
      pages_layout[PAGE_MAIN]->addWidget(button, row + 1, 1);
      QObject::connect(button, &QPushButton::clicked,
                       [this]() { switch_page(PAGE_NETWORK); });
      break;
    case 4:
      pages_layout[PAGE_MAIN]->addWidget(button, row + 2, 1);
      connect(button, &QAbstractButton::clicked, this,
              &fc_client::popup_client_options);
      break;
    case 5:
      pages_layout[PAGE_MAIN]->addWidget(button, row + 3, 1);
      QObject::connect(button, &QAbstractButton::clicked, this,
                       &fc_client::quit);
      break;
    default:
      break;
    }
  }
}
