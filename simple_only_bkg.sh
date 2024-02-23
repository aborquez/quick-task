#!/bin/bash

# Analyze sim. with AliAnalysisQuickTask

TOPDIR=/home/ceres/borquez/some/sims

SIM_SET=("LHC20e3a")
# RUN_NUMBERS=(295585 295586 295588 295589 295610)
RUN_NUMBERS=(295585)
DATA_SET=("LHC18q")

mkdir -p test
cp AliAnalysisQuickTask.cxx test/
cp AliAnalysisQuickTask.h test/
cp AddTask_QuickTask.C test/
cp runAnalysis.C test/

cd test

for SS in ${SIM_SET[@]}; do
    for ((i=0; i<${#RUN_NUMBERS[@]}; i++)); do

        RN=${RUN_NUMBERS[i]}
        DS=${DATA_SET[i]}

        RUNDIR=${TOPDIR}/${SS}/${RN}

        for SIMDIR in ${RUNDIR}/*/; do

            DN=$(basename ${SIMDIR})

            if [[ ! -e ${SIMDIR}/AliESDs.root ||
                ! -e ${SIMDIR}/Kinematics.root ||
                ! -e ${SIMDIR}/galice.root ]]; then
                continue
            fi

            ln -s ${SIMDIR}/AliESDs.root
            ln -s ${SIMDIR}/Kinematics.root
            ln -s ${SIMDIR}/galice.root

            aliroot -l -b -q 'runAnalysis.C(1, "'${DS}'", 0)' 2>&1 | tee analysis.log

            mv -v AnalysisResults.root AnalysisResults_${RN}_${DN}.root
            mv -v analysis.log analysis_${RN}_${DN}.log

            rm -v AliESDs.root Kinematics.root galice.root

        done
    done
done

rm -v AliAnalysisQuickTask*
rm -v AddTask_QuickTask.C runAnalysis.C

cd ..
