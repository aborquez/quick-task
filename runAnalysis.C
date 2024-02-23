#include "TChain.h"
#include "TROOT.h"
#include "TSystem.h"

#include "AliAnalysisManager.h"
#include "AliAnalysisTaskPIDResponse.h"
#include "AliESDInputHandler.h"
#include "AliMCEventHandler.h"

#include "AliAnalysisQuickTask.h"

void runAnalysis(Bool_t IsMC, TString RunPeriod, Int_t ChooseNEvents = 0) {

    // tell root where to look for headers
    gInterpreter->ProcessLine(".include ${ROOTSYS}/include");
    gInterpreter->ProcessLine(".include ${ALICE_ROOT}/include");
    gInterpreter->ProcessLine(".include ${ALICE_PHYSICS}/include");

    // create the analysis manager
    AliAnalysisManager *mgr = new AliAnalysisManager("AnalysisTaskExample");

    AliESDInputHandler *esdH = new AliESDInputHandler();
    mgr->SetInputEventHandler(esdH);
    esdH->SetNeedField();  // necessary to get GoldenChi2

    AliMCEventHandler *mcHand = new AliMCEventHandler();
    mgr->SetMCtruthEventHandler(mcHand);

    // choose options depending on data period
    Int_t n_pass;
    if (RunPeriod == "LHC15o") {
        n_pass = 2;
    } else if (RunPeriod == "LHC18q" || RunPeriod == "LHC18r") {
        n_pass = 3;
    } else {
        printf("Data period not recognized");
        return;
    }

    // CDB CONNECT
    /*
    // - to use this, one needs to clone https://gitlab.cern.ch/alisw/AliRootOCDB
    // - then, one needs to set the environment variable ALIROOT_OCDB_ROOT to the directory where the repo is cloned
    AliTaskCDBconnect *taskCDB = reinterpret_cast<AliTaskCDBconnect *>(
        gInterpreter->ExecuteMacro("${ALICE_PHYSICS}/PWGPP/PilotTrain/AddTaskCDBconnect.C(\"local://${ALIROOT_OCDB_ROOT}/OCDB\")"));
    taskCDB->SetFallBackToRaw(kTRUE);
    */

    // load PID task
    TString pid_response_path = gSystem->ExpandPathName("${ALICE_ROOT}/ANALYSIS/macros/AddTaskPIDResponse.C");
    AliAnalysisTaskPIDResponse *PIDresponseTask = reinterpret_cast<AliAnalysisTaskPIDResponse *>(
        gInterpreter->ExecuteMacro(Form("%s(%i, 1, 1, \"%i\")", pid_response_path.Data(), (Int_t)IsMC, n_pass)));

    // load task
    gInterpreter->LoadMacro("AliAnalysisQuickTask.cxx++g");
    AliAnalysisQuickTask *task = reinterpret_cast<AliAnalysisQuickTask *>(
        gInterpreter->ExecuteMacro(Form("AddTask_QuickTask.C(\"name\", %i)", (Int_t)IsMC)));

    if (!mgr->InitAnalysis()) {
        return;
    }

    // print info on screen
    mgr->SetDebugLevel(3);
    mgr->PrintStatus();

    // if you want to run locally, we need to define some input
    TChain *chain = new TChain("esdTree");
    // add a few files to the chain (change this so that your local files are added)
    chain->Add("AliESDs.root");

    if (!ChooseNEvents) {
        // read all events
        mgr->StartAnalysis("local", chain);
    } else {
        // read the first NEvents
        mgr->StartAnalysis("local", chain, ChooseNEvents);
    }
}
