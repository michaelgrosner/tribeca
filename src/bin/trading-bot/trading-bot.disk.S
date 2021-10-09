//! \file
//! \brief Lazy disk file builder using global external linkage.
//! \note  See src/lib/Krypto.ninja-disk.S.
//! \note  Resources at www/* are included from both locations:
//!          - /var/lib/K/assets/www/*
//!          - src/lib/Krypto.ninja-client/www/*

#define DISK(code) \
  code( html_index  , www/index.html                       ) \
  code( ico_favicon , www/favicon.ico                      ) \
  code( js_client   , www/js/client.min.js                 ) \
  code( css_base    , www/css/bootstrap.min.css            ) \
  code( css_dark    , www/css/bootstrap-theme-dark.min.css ) \
  code( css_light   , www/css/bootstrap-theme.min.css      ) \
  code( woff2_font  , www/font/beacons.woff2               ) \
  code( mp3_audio_0 , www/audio/0.mp3                      ) \
  code( mp3_audio_1 , www/audio/1.mp3                      ) \
  code( gzip_bomb   , www/.bomb.gzip                       )
