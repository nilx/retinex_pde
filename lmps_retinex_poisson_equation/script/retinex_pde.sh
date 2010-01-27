#!/bin/sh
#
# copyright (C) 2009 
# Nicolas Limare, CMLA, ENS Cachan <nicolas.limare@cmla.ens-cachan.fr>
#
# licensed under GPLv3, see http://www.gnu.org/licenses/gpl-3.0.html

set -e

PATH=$BINPATH:$PATH

retinex_pde -v > version
retinex_pde $* -d input.tiff balanced.tiff retinex.tiff 2>&1 > stderr.txt

exit
