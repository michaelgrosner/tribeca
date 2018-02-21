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

    .docroot   _www_html_index  , var/www/index.html
    .docroot   _www_ico_favicon , var/www/favicon.ico
    .docroot   _www_js_bundle   , var/www/js/client/bundle.min.js
    .docroot   _www_css_base    , var/www/css/bootstrap.min.css
    .docroot   _www_css_dark    , var/www/css/bootstrap-theme-dark.min.css
    .docroot   _www_css_light   , var/www/css/bootstrap-theme.min.css
    .docroot   _www_mp3_audio_0 , var/www/audio/0.mp3
    .docroot   _www_mp3_audio_1 , var/www/audio/1.mp3
    .docroot   _www_gzip_bomb   , ../../src/www/.bomb.gzip
