#!/bin/bash

# Analyze sim. with AliAnalysisQuickTask

ATTEMPT_NAME=test8
mkdir -p ${ATTEMPT_NAME}
cp AliAnalysisQuickTask.cxx ${ATTEMPT_NAME}/
cp AliAnalysisQuickTask.h ${ATTEMPT_NAME}/
cp AddTask_QuickTask.C ${ATTEMPT_NAME}/
cp runAnalysis.C ${ATTEMPT_NAME}/

cd ${ATTEMPT_NAME}

aliroot -l -b -q 'runAnalysis.C' 2>&1 | tee analysis.log

rm -v AliAnalysisQuickTask*
rm -v AddTask_QuickTask.C
rm -v runAnalysis.C

cd ..
