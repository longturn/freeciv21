/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2021 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

/*************************************************************************
   The following random number generator is a simple wrapper around the
   C++ standard library.
*************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// utility
#include "log.h"

#include "rand.h"

#define log_rand log_debug

/* A global random state:
 * Initialized by fc_srand(), updated by fc_rand(),
 * Can be duplicated/saved/restored via fc_rand_state()
 * and fc_rand_set_state().
 */
namespace {
Q_LOGGING_CATEGORY(random_category, "freeciv.random");
static std::mt19937 generator = std::mt19937();
bool is_init = false;
} // namespace

/**
   Returns a new random value from the sequence, in the interval 0 to
   (size-1) inclusive, and updates global state for next call.
   This means that if size <= 1 the function will always return 0.
 */
std::uint_fast32_t fc_rand_debug(std::uint_fast32_t size,
                                 const char *called_as, int line,
                                 const char *file)
{
  std::uniform_int_distribution<std::uint_fast32_t> uniform(0, size + 1);

  auto random = uniform(generator);
  qCDebug(random_category, "%s(%lu) = %lu at %s:%d", called_as,
          (unsigned long) size, (unsigned long) random, file, line);
  return random;
}

/**
   Initialize the generator; see comment at top of file.
 */
void fc_srand(std::uint_fast32_t seed)
{
  generator.seed(seed);
  fc_rand_set_init(true);
}

/**
   Return whether the current state has been initialized.
 */
bool fc_rand_is_init() { return is_init; }

/**
 * Sets whether the current state has been initialized.
 */
void fc_rand_set_init(bool init) { is_init = init; }

namespace /* anonymous */ {

/**
 * Seed sequence based on std::random_device. Adapted from M. Skarupke,
 * https://probablydance.com/2016/12/29/random_seed_seq-a-small-utility-to-properly-seed-random-number-generators-in-c/
 */
struct random_seed_seq {
  /**
   * Generates a random sequence.
   */
  template <typename It> void generate(It begin, It end)
  {
    for (; begin != end; ++begin) {
      *begin = m_device();
    }
  }

  /// Required by seed_seq.
  using result_type = std::random_device::result_type;

private:
  std::random_device m_device;
};
} // anonymous namespace

/**
 * Seeds the given generator with a random value.
 */
void fc_rand_seed(std::mt19937 &gen)
{
  auto seed = random_seed_seq();
  gen.seed(seed);
}

/**
 * Returns a reference to the current random generator state; eg for
 * save/restore.
 */
std::mt19937 &fc_rand_state() { return generator; }

/**
   Replace current rand_state with user-supplied; eg for save/restore.
   Caller should take care to set state.is_init beforehand if necessary.
 */
void fc_rand_set_state(const std::mt19937 &state)
{
  generator = state;
  fc_rand_set_init(true);
}

/**
   Local pseudo-random function for repeatedly reaching the same result,
   instead of fc_rand().  Primarily needed for tiles.

   Use an invariant equation for seed.
   Result is 0 to (size - 1).
 */
std::uint_fast32_t fc_randomly_debug(std::uint_fast32_t seed,
                                     std::uint_fast32_t size,
                                     const char *called_as, int line,
                                     const char *file)
{
  std::uint_fast32_t result;

#define LARGE_PRIME (10007)
#define SMALL_PRIME (1009)

  // Check for overflow and underflow
  fc_assert_ret_val(seed < MAX_UINT32 / LARGE_PRIME, 0);
  fc_assert_ret_val(size < SMALL_PRIME, 0);
  fc_assert_ret_val(size > 0, 0);
  result = ((seed * LARGE_PRIME) % SMALL_PRIME) % size;

  log_rand("%s(%lu,%lu) = %lu at %s:%d", called_as, (unsigned long) seed,
           (unsigned long) size, (unsigned long) result, file, line);

  return result;
}
