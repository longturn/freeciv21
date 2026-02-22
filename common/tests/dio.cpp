// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

// common
#include "dataio_raw.h"

#include <QtTest>

/**
 * Ruleset-related tests
 */
class test_dio : public QObject {
  Q_OBJECT

private slots:
  void get();
  void put();
};

/**
 * Tests dio_get* commands
 */
void test_dio::get()
{
  QByteArrayView din("\xff");
  int value = 0;

  dio_get<std::uint8_t>(din, value);
  QCOMPARE(value, 0xff);
  QCOMPARE(din.size(), 0);

  din = "\x01";
  dio_get<std::int8_t>(din, value);
  QCOMPARE(value, 1);
  QCOMPARE(din.size(), 0);

  din = "\xff";
  dio_get<std::int8_t>(din, value);
  QCOMPARE(value, -1);
  QCOMPARE(din.size(), 0);
}

/**
 * Tests dio_put* commands
 */
void test_dio::put()
{
  QByteArray dout;

  dio_put<std::uint8_t>(dout, 255);
  QCOMPARE(dout.size(), 1);
  QCOMPARE(dout, "\xff");

  dout.clear();
  dio_put<std::int8_t>(dout, 1);
  QCOMPARE(dout.size(), 1);
  QCOMPARE(dout, "\x01");

  dout.clear();
  dio_put<std::int8_t>(dout, -1);
  QCOMPARE(dout.size(), 1);
  QCOMPARE(dout, "\xff");
}

QTEST_MAIN(test_dio)
#include "dio.moc"
