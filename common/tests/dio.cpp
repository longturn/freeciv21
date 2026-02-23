// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

// common
#include "dataio_raw.h"

// std
#include <string>

// Qt
#include <QByteArray>
#include <QByteArrayView>
#include <QtTest>

/**
 * Ruleset-related tests
 */
class test_dio : public QObject {
  Q_OBJECT

private slots:
  void get_int();
  void put_int();

  void boolean();
  void string();
};

/**
 * Tests dio_get<> integer functions
 */
void test_dio::get_int()
{
  QByteArrayView din("\xff");
  int value = 0;

  dio_get<std::uint8_t>(din, value);
  QCOMPARE(value, 255);
  QCOMPARE(din.size(), 0);

  din = "\x01";
  dio_get<std::int8_t>(din, value);
  QCOMPARE(value, 1);
  QCOMPARE(din.size(), 0);

  din = "\xff";
  dio_get<std::int8_t>(din, value);
  QCOMPARE(value, -1);
  QCOMPARE(din.size(), 0);

  // 2-bytes
  din = QByteArrayView("\x00\x01", 2);
  dio_get<std::uint16_t>(din, value);
  QCOMPARE(value, 1);
  QCOMPARE(din.size(), 0);

  din = QByteArrayView("\xff\xfe", 2);
  dio_get<std::uint16_t>(din, value);
  QCOMPARE(value, 0xfffe);
  QCOMPARE(din.size(), 0);

  din = QByteArrayView("\x00\x01", 2);
  dio_get<std::int16_t>(din, value);
  QCOMPARE(value, 1);
  QCOMPARE(din.size(), 0);

  din = QByteArrayView("\xff\xfe", 2);
  dio_get<std::int16_t>(din, value);
  QCOMPARE(value, -2);
  QCOMPARE(din.size(), 0);

  // 4-bytes
  din = QByteArrayView("\x00\x00\x00\x01", 4);
  dio_get<std::uint32_t>(din, value);
  QCOMPARE(value, 1);
  QCOMPARE(din.size(), 0);

  din = QByteArrayView("\x00\x00\xff\xfe", 4);
  dio_get<std::uint32_t>(din, value);
  QCOMPARE(value, 0xfffe);
  QCOMPARE(din.size(), 0);

  din = QByteArrayView("\x00\x00\x00\x01", 4);
  dio_get<std::int32_t>(din, value);
  QCOMPARE(value, 1);
  QCOMPARE(din.size(), 0);

  din = QByteArrayView("\xff\xff\xff\xfe", 4);
  dio_get<std::int32_t>(din, value);
  QCOMPARE(value, -2);
  QCOMPARE(din.size(), 0);
}

/**
 * Tests dio_put<> integer functions
 */
void test_dio::put_int()
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

  // 2-bytes
  dout.clear();
  dio_put<std::uint16_t>(dout, 1);
  QCOMPARE(dout.size(), 2);
  QCOMPARE(dout, QByteArrayView("\x00\x01", 2));

  dout.clear();
  dio_put<std::uint16_t>(dout, 0xfffe);
  QCOMPARE(dout.size(), 2);
  QCOMPARE(dout, QByteArrayView("\xff\xfe", 2));

  dout.clear();
  dio_put<std::int16_t>(dout, 1);
  QCOMPARE(dout.size(), 2);
  QCOMPARE(dout, QByteArrayView("\x00\x01", 2));

  dout.clear();
  dio_put<std::int16_t>(dout, 0xfffe);
  QCOMPARE(dout.size(), 2);
  QCOMPARE(dout, QByteArrayView("\xff\xfe", 2));

  // 4-bytes
  dout.clear();
  dio_put<std::uint32_t>(dout, 1);
  QCOMPARE(dout.size(), 4);
  QCOMPARE(dout, QByteArrayView("\x00\x00\x00\x01", 4));

  dout.clear();
  dio_put<std::uint32_t>(dout, 0xfffe);
  QCOMPARE(dout.size(), 4);
  QCOMPARE(dout, QByteArrayView("\x00\x00\xff\xfe", 4));

  dout.clear();
  dio_put<std::int32_t>(dout, 1);
  QCOMPARE(dout.size(), 4);
  QCOMPARE(dout, QByteArrayView("\x00\x00\x00\x01", 4));

  dout.clear();
  dio_put<std::int32_t>(dout, -2);
  QCOMPARE(dout.size(), 4);
  QCOMPARE(dout, QByteArrayView("\xff\xff\xff\xfe", 4));
}

/**
 * Tests dio_put and dio_get bool
 */
void test_dio::boolean()
{
  // true
  QByteArray dout;
  dio_put(dout, true);
  QCOMPARE(dout.size(), 1);
  QCOMPARE(dout, "\x01");

  QByteArrayView din(dout);
  bool value;
  dio_get(din, value);
  QCOMPARE(value, true);
  QCOMPARE(din.size(), 0);

  // false
  dout.clear();
  dio_put(dout, false);
  QCOMPARE(dout.size(), 1);
  QCOMPARE(dout, QByteArrayView("\x00", 1));

  din = QByteArrayView(dout);
  dio_get(din, value);
  QCOMPARE(value, false);
  QCOMPARE(din.size(), 0);
}

/**
 * Tests dio_put and dio_get string
 */
void test_dio::string()
{
  std::string value = "Hello";
  QByteArray dout;
  dio_put(dout, value.data());
  QCOMPARE(dout.size(), 6);
  QCOMPARE(dout, QByteArrayView("Hello\0", 6));

  value.clear(); // Erase the value
  value.resize(10);
  QByteArrayView din(dout);
  dio_get(din, value.data(), 10);
  // Compare char*'s, we don't care about the value of bytes 6-10
  QCOMPARE(value.data(), "Hello");
  QCOMPARE(din.size(), 0);
}

QTEST_MAIN(test_dio)
#include "dio.moc"
