/*####################################################################
###      ***                                     ***               ###
###     *****          ⒻⓇⒺⒺⒸⒾⓋ ②①         *****              ###
###      ***                                     ***               ###
#####################################################################*/

#ifndef FC__TOOLTIP_H
#define FC__TOOLTIP_H

#include <QObject>

class QString;
class QVariant;

QString get_tooltip(QVariant qvar);
QString get_tooltip_improvement(const impr_type *building,
                                struct city *pcity = nullptr,
                                bool ext = false);
QString get_tooltip_unit(const struct unit_type *utype, bool ext = false);
QString bold(const QString &text);

class fc_tooltip : public QObject {
  Q_OBJECT
public:
  explicit fc_tooltip(QObject *parent = NULL) : QObject(parent) {}

protected:
  bool eventFilter(QObject *obj, QEvent *event);
};

#endif /* FC__TOOLTIP_H */