#ifdef __APPLE__
#  define SYM(sym) _ ## sym
#else
#  define SYM(sym) sym
#endif

    .data

  .macro \
    .docroot   var              , uri
    .global    SYM(\var)
    .global    SYM(\var\()_len)
SYM(\var):
    .incbin    "\uri"
SYM(\var\()_len):
    .int       . - SYM(\var)
  .endm

    .docroot   _www_html_index  , www/index.html
    .docroot   _www_ico_favicon , www/favicon.ico
    .docroot   _www_js_bundle   , www/js/client/bundle.min.js
    .docroot   _www_css_base    , www/css/bootstrap.min.css
    .docroot   _www_css_dark    , www/css/bootstrap-theme-dark.min.css
    .docroot   _www_css_light   , www/css/bootstrap-theme.min.css
    .docroot   _www_mp3_audio_0 , www/audio/0.mp3
    .docroot   _www_mp3_audio_1 , www/audio/1.mp3
    .docroot   _www_gzip_bomb   , www/.bomb.gzip

    .end
