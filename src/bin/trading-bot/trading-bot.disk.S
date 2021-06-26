#ifdef __APPLE__
#  define SYM(sym) _ ## sym
#else
#  define SYM(sym)      sym
#endif

    .data

  .macro    \
    .docroot \
               var                    , uri
    .global    \var
    .global    \var\()_len
\var:
    .incbin    "\uri"
\var\()_len:
    .int       . - \var
  .endm

    .docroot   SYM(_file_html_index)  , www/index.html
    .docroot   SYM(_file_ico_favicon) , www/favicon.ico
    .docroot   SYM(_file_js_client)   , www/js/client.min.js
    .docroot   SYM(_file_css_base)    , www/css/bootstrap.min.css
    .docroot   SYM(_file_css_dark)    , www/css/bootstrap-theme-dark.min.css
    .docroot   SYM(_file_css_light)   , www/css/bootstrap-theme.min.css
    .docroot   SYM(_file_woff2_font)  , www/font/beacons.woff2
    .docroot   SYM(_file_mp3_audio_0) , www/audio/0.mp3
    .docroot   SYM(_file_mp3_audio_1) , www/audio/1.mp3
    .docroot   SYM(_file_gzip_bomb)   , www/.bomb.gzip

    .end
