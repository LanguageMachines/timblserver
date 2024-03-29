[![GitHub build](https://github.com/LanguageMachines/timblserver/actions/workflows/timblserver.yml/badge.svg?branch=master)](https://github.com/LanguageMachines/timblserver/actions/)
[![Language Machines Badge](http://applejack.science.ru.nl/lamabadge.php/timblserver)](http://applejack.science.ru.nl/languagemachines/
)
[![DOI](https://zenodo.org/badge/20526237.svg)](https://zenodo.org/badge/latestdoi/20526237)

===========================================
TimblServer 1.18 (c) CLS/ILK/CLiPS 1998 - 2024
===========================================

    Tilburg Memory Based Learner -- Server
    Centre for Language Studies, Radboud University Nijmegen
    Induction of Linguistic Knowledge Research Group, Tilburg University and
    Centre for Dutch Language and Speech, University of Antwerp


**Website:** https://languagemachines.github.io/timbl/


This is the Server extension for Timbl.

TiMBL is an open source software package implementing several memory-based
learning algorithms, among which IB1-IG, an implementation of k-nearest
neighbor classification with feature weighting suitable for symbolic feature
spaces, and IGTree, a decision-tree approximation of IB1-IG. All implemented
algorithms have in common that they store some representation of the training
set explicitly in memory. During testing, new cases are classified by
extrapolation from the most similar stored cases.

For over fifteen years TiMBL has been mostly used in natural language
processing as a machine learning classifier component, but its use extends to
virtually any supervised machine learning domain. Due to its particular
decision-tree-based implementation, TiMBL is in many cases far more efficient
in classification than a standard k-nearest neighbor algorithm would be.


-----------------------------------------------------------------------

Comments and bug-reports are welcome at our issue tracker at
https://github.com/LanguageMachines/timblserver/issues or by mailing
lamasoftware (at) science.ru.nl.
Updates and more info may be found on https://languagemachines.github.io/timbl .

TimblServer is distributed under the GNU Public Licence v3
  (see the file COPYING).

-----------------------------------------------------------------------

This software has been tested on:
- Intel platforms running several versions of Linux, including Ubuntu, Debian,
  Arch Linux, Fedora (both 32 and 64 bits)
- MAC platform running OS X 10.10

Compilers:
 - GCC (use 7.0 or later)
 - Clang

Contents of this distribution:
- Sources
- Licensing information ( COPYING )
- Installation instructions ( INSTALL )
- Build system based on GNU Autotools
- Example data files ( in the demos directory )
- Documentation ( in the docs directory )

Dependencies:
To be able to succesfully build TiMBL from the tarball, you need the
following pakages:
- ticcutils (https://github.com/LanguageMachines/ticcutils)
- timbl (https://github.com/LanguageMachines/timbl)
- pkg-config
- libxml2-dev


To install TimblServer, first consult whether your distribution's package manager has an up-to-date package for TimblServer.
If not, for easy installation of everything concerning TiMBL and all dependencies, it is included as part of our software
distribution LaMachine: https://proycon.github.io/LaMachine .

To compile and install manually from source instead, provided you have all the dependencies installed:

 $ bash bootstrap.sh
 $ ./configure
 $ make
 $ make install

(See the INSTALL file for more details)
