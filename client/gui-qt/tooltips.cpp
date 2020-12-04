/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

// Qt
#include <QAbstractItemView>
#include <QHelpEvent>
#include <QModelIndex>
#include <QToolTip>
// utility
#include "fcintl.h"
#include "support.h"
// client
#include "client_main.h"
#include "helpdata.h"
#include "movement.h"
// gui-qt
#include "fc_client.h"
#include "tooltips.h"

extern QString split_text(const QString &text, bool cut);
extern QString cut_helptext(const QString &text);

/************************************************************************/ /**
   Event filter for catching tooltip events
 ****************************************************************************/
bool fc_tooltip::eventFilter(QObject *obj, QEvent *ev)
{
  QHelpEvent *help_event;
  QString item_tooltip;
  QRect rect;

  if (ev->type() == QEvent::ToolTip) {
    QAbstractItemView *view =
        qobject_cast<QAbstractItemView *>(obj->parent());

    if (!view) {
      return false;
    }

    help_event = static_cast<QHelpEvent *>(ev);
    QPoint pos = help_event->pos();
    QModelIndex index = view->indexAt(pos);

    if (!index.isValid()) {
      return false;
    }

    item_tooltip = view->model()->data(index, Qt::ToolTipRole).toString();
    rect = view->visualRect(index);
    rect.setX(rect.x() + help_event->globalPos().x());
    rect.setY(rect.y() + help_event->globalPos().y());

    if (!item_tooltip.isEmpty()) {
      QToolTip::showText(help_event->globalPos(), item_tooltip, view, rect);
    } else {
      QToolTip::hideText();
    }

    return true;
  }

  return false;
}

/**************************************************************************
  'text' is assumed to have already been HTML-escaped if necessary
**************************************************************************/
QString bold(const QString &text) { return QString("<b>" + text + "</b>"); }

/************************************************************************/ /**
   Returns improvement properties to append in tooltip
   ext is used to get extra info from help
 ****************************************************************************/
QString get_tooltip_improvement(const impr_type *building,
                                struct city *pcity, bool ext)
{
  QString def_str;
  QString upkeep;
  QString s1, s2, str;
  const char *req = skip_intl_qualifier_prefix(_("?tech:None"));

  if (pcity != nullptr) {
    upkeep = QString::number(city_improvement_upkeep(pcity, building));
  } else {
    upkeep = QString::number(building->upkeep);
  }
  requirement_vector_iterate(&building->obsolete_by, pobs)
  {
    if (pobs->source.kind == VUT_ADVANCE) {
      req = advance_name_translation(pobs->source.value.advance);
      break;
    }
  }
  requirement_vector_iterate_end;
  s2 = QString(req);
  str = _("Obsolete by:");
  str = str + " " + s2;
  def_str = "<p style='white-space:pre'><b>"
            + QString(improvement_name_translation(building)).toHtmlEscaped()
            + "</b>\n";
  if (pcity != nullptr) {
    def_str += QString(_("Cost: %1, Upkeep: %2\n"))
                   .arg(impr_build_shield_cost(pcity, building))
                   .arg(upkeep)
                   .toHtmlEscaped();
  } else {
    int cost_est =
        impr_estimate_build_shield_cost(client.conn.playing, NULL, building);

    def_str += QString(_("Cost Estimate: %1, Upkeep: %2\n"))
                   .arg(cost_est)
                   .arg(upkeep)
                   .toHtmlEscaped();
  }
  if (s1.compare(s2) != 0) {
    def_str = def_str + str.toHtmlEscaped() + "\n";
  }
  def_str = def_str + "\n";
  if (ext) {
    char buffer[8192];

    str = helptext_building(buffer, sizeof(buffer), client.conn.playing,
                            NULL, building);
    str = cut_helptext(str);
    str = split_text(str, true);
    str = str.trimmed();
    def_str = def_str + str.toHtmlEscaped();
  }
  return def_str;
}

/************************************************************************/ /**
   Returns unit properties to append in tooltip
   ext is used to get extra info from help
 ****************************************************************************/
QString get_tooltip_unit(const struct unit_type *utype, bool ext)
{
  QString def_str;
  QString obsolete_str;
  QString str;
  const struct unit_type *obsolete;
  struct advance *tech;

  def_str = "<b>" + QString(utype_name_translation(utype)).toHtmlEscaped()
            + "</b>\n";
  obsolete = utype->obsoleted_by;
  if (obsolete) {
    tech = obsolete->require_advance;
    obsolete_str = QStringLiteral("</td></tr><tr><td colspan=\"3\">");
    if (tech && tech != advance_by_number(0)) {
      /* TRANS: this and nearby literal strings are interpreted
       * as (Qt) HTML */
      obsolete_str = obsolete_str
                     + QString(_("Obsoleted by %1 (%2)."))
                           .arg(utype_name_translation(obsolete),
                                advance_name_translation(tech))
                           .toHtmlEscaped();
    } else {
      obsolete_str = obsolete_str
                     + QString(_("Obsoleted by %1."))
                           .arg(utype_name_translation(obsolete))
                           .toHtmlEscaped();
    }
  }
  def_str +=
      "<table width=\"100\%\"><tr><td>" + bold(QString(_("Attack:"))) + " "
      + QString::number(utype->attack_strength).toHtmlEscaped()
      + QStringLiteral("</td><td>") + bold(QString(_("Defense:"))) + " "
      + QString::number(utype->defense_strength).toHtmlEscaped()
      + QStringLiteral("</td><td>") + bold(QString(_("Move:"))) + " "
      + QString(move_points_text(utype->move_rate, TRUE)).toHtmlEscaped()
      + QStringLiteral("</td></tr><tr><td>") + bold(QString(_("Cost:")))
      + " "
      + QString::number(utype_build_shield_cost_base(utype)).toHtmlEscaped()
      + QStringLiteral("</td><td colspan=\"2\">")
      + bold(QString(_("Basic Upkeep:"))) + " "
      + QString(helptext_unit_upkeep_str(utype)).toHtmlEscaped()
      + QStringLiteral("</td></tr><tr><td>") + bold(QString(_("Hitpoints:")))
      + " " + QString::number(utype->hp).toHtmlEscaped()
      + QStringLiteral("</td><td>") + bold(QString(_("FirePower:"))) + " "
      + QString::number(utype->firepower).toHtmlEscaped()
      + QStringLiteral("</td><td>") + bold(QString(_("Vision:"))) + " "
      + QString::number((int) sqrt((double) utype->vision_radius_sq))
            .toHtmlEscaped()
      + obsolete_str
      + QStringLiteral("</td></tr></table><p style='white-space:pre'>");
  if (ext) {
    char buffer[8192];
    char buf2[1];

    buf2[0] = '\0';
    str = helptext_unit(buffer, sizeof(buffer), client.conn.playing, buf2,
                        utype);
    str = cut_helptext(str);
    str = split_text(str, true);
    str = str.trimmed().toHtmlEscaped();
    def_str = def_str + str;
  }

  return def_str;
};

/************************************************************************/ /**
   Returns shortened help for given universal ( stored in qvar )
 ****************************************************************************/
QString get_tooltip(QVariant qvar)
{
  QString str, def_str, ret_str;
  QStringList sl;
  char buffer[8192];
  char buf2[1];
  struct universal *target;

  buf2[0] = '\0';
  target = reinterpret_cast<universal *>(qvar.value<void *>());

  if (target == NULL) {
  } else if (VUT_UTYPE == target->kind) {
    def_str = get_tooltip_unit(target->value.utype);
    str = helptext_unit(buffer, sizeof(buffer), client.conn.playing, buf2,
                        target->value.utype);
  } else {
    if (!improvement_has_flag(target->value.building, IF_GOLD)) {
      def_str = get_tooltip_improvement(target->value.building);
    }

    str = helptext_building(buffer, sizeof(buffer), client.conn.playing,
                            NULL, target->value.building);
  }

  /* Remove all lines from help which has '*' in first 3 chars */
  ret_str = cut_helptext(str);
  ret_str = split_text(ret_str, true);
  ret_str = ret_str.trimmed();
  ret_str = def_str + ret_str.toHtmlEscaped();

  return ret_str;
}
