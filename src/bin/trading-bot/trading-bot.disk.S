//! \file
//! \brief Lazy disk file builder using global external linkage.
//! \note  Line 10 can be removed; just exemplifies each column.
//! \note  See src/lib/Krypto.ninja-disk.S for info about DISK()
//! \note  Filesystem paths at www/* are included from lib/, at:
//!          - /var/lib/K/assets/www/*
//!          - ./src/lib/Krypto.ninja-client/www/*

#define DISK(file) \
/*file( example_var , www/filesystem/path/to/file     , /webserver/url/path          )*/\
  file( html_index  , www/index.html                  , /                            )  \
  file( ico_favicon , www/favicon.ico                 , /favicon.ico                 )  \
  file( js_client   , www/js/client.min.js            , /js/client.min.js            )  \
  file( css_base    , www/css/bootstrap.min.css       , /css/bootstrap.min.css       )  \
  file( css_dark    , www/css/bootstrap-dark.min.css  , /css/bootstrap-dark.min.css  )  \
  file( css_light   , www/css/bootstrap-light.min.css , /css/bootstrap-light.min.css )  \
  file( woff2_font  , www/font/beacons.woff2          , /font/beacons.woff2          )  \
  file( mp3_audio_0 , www/audio/0.mp3                 , /audio/0.mp3                 )  \
  file( mp3_audio_1 , www/audio/1.mp3                 , /audio/1.mp3                 )  \
  file( gzip_bomb   , www/.bomb.gzip                  ,                              )
