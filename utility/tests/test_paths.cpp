#include "shared.h"

#include <QtTest>

/**
 * Tests functions acting on paths
 */
class test_paths : public QObject {
  Q_OBJECT

private slots:
  void is_safe_filename();
};

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
