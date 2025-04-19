// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv and Freeciv21 Contributors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "audio_qt.h"

// utility
#include "log.h"

// client
#include "audio.h"

// Qt
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QSoundEffect>
#include <QtAudio>

namespace {
static QMediaPlayer *player = nullptr;
static QAudioOutput *output = nullptr;
static double audio_volume = 1.0;
audio_finished_callback current_callback;
static std::map<QString, QSoundEffect> cache = {};
} // namespace

/**
 * Set the volume.
 */
static void qt_audio_set_volume(double volume)
{
  audio_volume = QtAudio::convertVolume(
      volume, QtAudio::LogarithmicVolumeScale, QtAudio::LinearVolumeScale);
  output->setVolume(audio_volume);
}

/**
 * Get the volume.
 */
static double qt_audio_get_volume()
{
  return QtAudio::convertVolume(audio_volume, QtAudio::LinearVolumeScale,
                                QtAudio::LogarithmicVolumeScale);
}

/**
 * Play sound
 */
static bool qt_audio_play(const QString &tag, const QString &fullpath,
                          bool repeat, audio_finished_callback cb)
{
  if (fullpath.isEmpty()) {
    return false;
  }

  if (repeat) {
    // Repeating media is used for music. Since latency isn't critical here,
    // we use QMediaPlayer.
    player->setSource(QUrl::fromLocalFile(fullpath));

    if (!cb) {
      player->setLoops(QMediaPlayer::Infinite);
    }
    current_callback = cb;

    player->play();

    qDebug() << "Playing music file" << fullpath;

  } else {
    // Non-repeating media is used for sound effects. Latency is more
    // important for this, so we use QSoundEffect.
    if (cache.count(fullpath) == 0) {
      // Load the data.
      qDebug() << "Loading sound effect" << fullpath;
      auto &effect = cache[fullpath]; // Instantiates it.
      QObject::connect(&effect, &QSoundEffect::statusChanged, [&effect] {
        if (effect.status() == QSoundEffect::Error) {
          qCritical() << "Error playing sound effect"
                      << effect.source().toDisplayString();
        }
      });
      effect.setSource(QUrl::fromLocalFile(fullpath));
    }

    // It's already loaded.
    qDebug() << "Playing sound effect" << fullpath;
    cache[fullpath].setVolume(audio_volume);
    cache[fullpath].play();
  }

  return true;
}

/**
 * Stop music.
 */
static void qt_audio_stop()
{
  if (player) {
    player->stop();
  }

  for (auto &[_, effect] : cache) {
    if (effect.isPlaying()) {
      effect.stop();
    }
  }
}

/**
 * Wait for audio to die on all channels.
 */
static void qt_audio_wait()
{
  // noop
}

/**
 * Quit Qt audio. This frees up resources.
 */
static void quit_qt_audio()
{
  delete player;
  player = nullptr;
  delete output;
  output = nullptr;
  cache.clear();
}

/**
 * Clean up.
 */
static void qt_audio_shutdown()
{
  qt_audio_stop();
  qt_audio_wait();
  quit_qt_audio();
}

/**
 * Initialize.
 */
static bool qt_audio_init()
{
  output = new QAudioOutput;
  player = new QMediaPlayer;
  player->setAudioOutput(output);
  QObject::connect(player, &QMediaPlayer::mediaStatusChanged,
                   [](auto status) {
                     switch (status) {
                     case QMediaPlayer::InvalidMedia:
                       qCritical() << "Invalid media"
                                   << player->source().toDisplayString()
                                   << ":" << player->errorString();
                       [[fallthrough]];
                     case QMediaPlayer::EndOfMedia:
                       if (current_callback) {
                         current_callback();
                       }
                       break;
                     default:
                       break;
                     }
                   });

  return true;
}

/**
 * Initialize. Note that this function is called very early at the
 * client startup. So for example logging isn't available.
 */
void audio_qt_init()
{
  struct audio_plugin self;

  self.name = QStringLiteral("qt");
  self.descr = QStringLiteral("Qt Multimedia mixer plugin");
  self.init = qt_audio_init;
  self.shutdown = qt_audio_shutdown;
  self.stop = qt_audio_stop;
  self.wait = qt_audio_wait;
  self.play = qt_audio_play;
  self.set_volume = qt_audio_set_volume;
  self.get_volume = qt_audio_get_volume;
  audio_add_plugin(&self);
}
