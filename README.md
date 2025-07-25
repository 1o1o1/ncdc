Ncdc 1.25
=========

DESCRIPTION

  Ncdc is a modern and lightweight direct connect client with a friendly
  ncurses interface.

REQUIREMENTS

*  ncursesw
*  zlib
*  bzip2
*  sqlite  >= 3.3.9
*  glib    >= 2.32.0
*  gnutls  >= 3.0
*  libmaxminddb (optional)
*  libloc       (optional)



BUILDING FROM A RELEASE TARBALL

  If you managed to fetch an ncdc tarball from somewhere, then you will need
  the following to build ncdc:
  - A C compiler
  - Development files for the libraries mentioned above
  - make
  - pkg-config
  - pod2man (optional, comes with a default Perl installation)
  - makeheaders (optional, included in the distribution if you don't have it)

  And the usual commands (with GeoIP support or not) will get you up and running:

    ./configure --prefix=/usr --with-geoip
    make
    (sudo) make install



BUILDING FROM THE GIT REPOSITORY

  To build the latest and greatest version from the git repository, you will
  need the stuff mentioned above in addition to GNU autoconf and automake.
  After checking out the git repo, run the following command:

    autoreconf -i

  ...and you can use same tricks to build ncdc as with using a release tarball.



CROSS COMPILING

  You may run into some trouble when the binaries of the target system don't
  run on the build system. To avoid that, build the 'makeheaders' and 'gendoc'
  utilities manually before running the regular 'make'. For example:

    ./configure
    cc deps/makeheaders.c -o makeheaders
    cc -I. doc/gendoc.c -o gendoc
    make

  Replace 'cc' with a compiler that builds binaries that can be run on the
  build system.

  Always make sure you run the latest version. You can check for updates and
  find more information at https://dev.yorhel.nl/ncdc

CONTACT

 * Email: projects@yorhel.nl
 * Web: https://dev.yorhel.nl/ncdc
 * DC: adcs://dc.blicky.net:2780/

