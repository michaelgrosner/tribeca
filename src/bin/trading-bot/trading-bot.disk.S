//! \file
//! \brief Lazy disk file builder using global external linkage.
//! \note  Line 11 can be removed; just exemplifies each column.
//! \note  See src/lib/Krypto.ninja-disk.S for info about DISK()
//! \note  Webserver 404 page is loaded using an empty url path.
//! \note  Filesystem paths at www/* are included from lib/, at:
//!          - /var/lib/K/client/www/*
//!          - ./src/lib/Krypto.ninja-client/www/*

#define DISK(file) \
/*file( id , www/filesystem/path/to/files    , /webserver/url/paths         )*/\
  file( 01 , www/index.html                  , /                            )  \
  file( 02 , www/favicon.ico                 , /favicon.ico                 )  \
  file( 03 , www/js/client.min.js            , /js/client.min.js            )  \
  file( 04 , www/css/bootstrap.min.css       , /css/bootstrap.min.css       )  \
  file( 05 , www/css/bootstrap-dark.min.css  , /css/bootstrap-dark.min.css  )  \
  file( 06 , www/css/bootstrap-light.min.css , /css/bootstrap-light.min.css )  \
  file( 07 , www/font/beacons.woff2          , /font/beacons.woff2          )  \
  file( 08 , www/audio/0.mp3                 , /audio/0.mp3                 )  \
  file( 09 , www/audio/1.mp3                 , /audio/1.mp3                 )  \
  file( 00 , www/.bomb.gzip                  ,                              )
