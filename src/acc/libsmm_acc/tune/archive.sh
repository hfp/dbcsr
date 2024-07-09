#!/bin/bash -e
####################################################################################################
# Copyright (C) by the DBCSR developers group - All rights reserved                                #
# This file is part of the DBCSR library.                                                          #
#                                                                                                  #
# For information on the license, see the LICENSE file.                                            #
# For further information please visit https://dbcsr.cp2k.org                                      #
# SPDX-License-Identifier: GPL-2.0+                                                                #
####################################################################################################

echo "removing unneeded files...."
rm -f tune_*/*.job tune_*/Makefile tune_*/*_exe? tune_*/*_part*.cu tune_*/*_part*.cpp tune_*/*.o

fn="../libsmm_acc_tuning_`date +'%F'`.tgz"

if [ -f $fn ]; then
   echo "Archive file exists already, aborting!"
   exit 1
fi

set -x
tar czf $fn .
