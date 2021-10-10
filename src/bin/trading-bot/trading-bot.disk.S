//! \file
//! \brief Lazy disk file builder using global external linkage.
//! \note  Line 11 can be removed; just exemplifies each column.
//! \note  See src/lib/Krypto.ninja-disk.S for info about DISK()
//! \note  Webserver 404 page is loaded using an empty url path.
//! \note  Filesystem paths at www/* are included from lib/, at:
//!          - /var/lib/K/assets/www/*
//!          - ./src/lib/Krypto.ninja-client/www/*

#define DISK(file) \
/*file( ids , www/filesystem/path/to/files    , /webserver/url/paths         )*/\
  file( 001 , www/index.html                  , /                            )  \
  file( 002 , www/favicon.ico                 , /favicon.ico                 )  \
  file( 003 , www/js/client.min.js            , /js/client.min.js            )  \
  file( 004 , www/css/bootstrap.min.css       , /css/bootstrap.min.css       )  \
  file( 005 , www/css/bootstrap-dark.min.css  , /css/bootstrap-dark.min.css  )  \
  file( 006 , www/css/bootstrap-light.min.css , /css/bootstrap-light.min.css )  \
  file( 007 , www/font/beacons.woff2          , /font/beacons.woff2          )  \
  file( 008 , www/audio/0.mp3                 , /audio/0.mp3                 )  \
  file( 009 , www/audio/1.mp3                 , /audio/1.mp3                 )  \
  file( 000 , www/.bomb.gzip                  ,                              )
