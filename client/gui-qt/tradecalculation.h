#ifndef FC__TRADECALC_H
#define FC__TRADECALC_H

#include <QList>

struct city;

/**************************************************************************
  Helper item for trade calculation
***************************************************************************/
class trade_city {
public:
  trade_city(struct city *pcity);

  bool done;
  int over_max;
  int poss_trade_num;
  int trade_num; // already created + generated
  QList<struct city *> curr_tr_cities;
  QList<struct city *> new_tr_cities;
  QList<struct city *> pos_cities;
  struct city *city;
  struct tile *tile;
};

/**************************************************************************
  Struct of 2 tiles, used for drawing trade routes.
  Also assigned caravan if it was sent
***************************************************************************/
struct qtiles {
  struct tile *t1;
  struct tile *t2;
  struct unit *autocaravan;

  bool operator==(const qtiles &a) const
  {
    return (t1 == a.t1 && t2 == a.t2 && autocaravan == a.autocaravan);
  }
};

/**************************************************************************
  Class trade generator, used for calulating possible trade routes
***************************************************************************/
class trade_generator {
public:
  trade_generator();

  bool hover_city;
  QList<qtiles> lines;
  QList<struct city *> virtual_cities;
  QList<trade_city *> cities;

  void add_all_cities();
  void add_city(struct city *pcity);
  void add_tile(struct tile *ptile);
  void calculate();
  void clear_trade_planing();
  void remove_city(struct city *pcity);
  void remove_virtual_city(struct tile *ptile);

private:
  bool discard_any(trade_city *tc, int freeroutes);
  bool discard_one(trade_city *tc);
  int find_over_max(struct city *pcity);
  trade_city *find_most_free();
  void check_if_done(trade_city *tc1, trade_city *tc2);
  void discard();
  void discard_trade(trade_city *tc1, trade_city *tc2);
  void find_certain_routes();
};

#endif /* FC__TRADECALC_H */
