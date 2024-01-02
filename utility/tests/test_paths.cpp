#include "shared.h"

#include <QtTest>

/**
 * Tests functions acting on paths
 */
class test_paths : public QObject {
  Q_OBJECT

private slots:
  void interpret_tilde();
  void is_safe_filename();
};

/**
 * Tests \ref ::interpret_tilde
 */
void test_paths::interpret_tilde()
{
  QCOMPARE(::interpret_tilde(QLatin1String("~")), QDir::homePath());
  QCOMPARE(::interpret_tilde(QLatin1String("test")), QLatin1String("test"));
}

/**
 * Tests \ref ::is_safe_filename
 */
void test_paths::is_safe_filename()
{
  QCOMPARE(::is_safe_filename(QLatin1String("")), false);
  QCOMPARE(::is_safe_filename(QLatin1String("abcABC_-._")), true);
  QCOMPARE(::is_safe_filename(QLatin1String("a/b")), false);
  QCOMPARE(::is_safe_filename(QLatin1String("..")), false);
}

QTEST_MAIN(test_paths)
#include "test_paths.moc"
