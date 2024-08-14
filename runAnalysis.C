#include "TChain.h"
#include "TROOT.h"
#include "TSystem.h"

#include "AliAnalysisAlien.h"
#include "AliAnalysisManager.h"
#include "AliESDInputHandler.h"
#include "AliMCEventHandler.h"

#include "AliAnalysisTaskPIDResponse.h"
#include "AliMultSelectionTask.h"
#include "AliPhysicsSelectionTask.h"

#include "AliAnalysisQuickTask.h"

void runAnalysis(Int_t ChooseNEvents = 0) {

    const Bool_t IS_MC = kTRUE;
    const Int_t N_PASS = 3;  // TEST

    Float_t SexaquarkMass = 1.8;

    TString GRID_DATA_DIR = Form("/alice/sim/2023/LHC23l1a3/A%.1f", SexaquarkMass);
    Int_t GRID_RUN_NUMBER = 297595;
    TString GRID_DATA_PATTERN = "/*/AliESDs.root";

    gInterpreter->ProcessLine(".include $ROOTSYS/include");
    gInterpreter->ProcessLine(".include $ALICE_ROOT/include");
    gInterpreter->ProcessLine(".include $ALICE_PHYSICS/include");
    gInterpreter->ProcessLine(".include $KFPARTICLE_ROOT/include");

    Bool_t local = kFALSE;    // set if you want to run the analysis locally (kTRUE), or on grid (kFALSE)
    Bool_t gridTest = kTRUE;  // if you run on grid, specify test mode (kTRUE) or full grid model (kFALSE)

    AliAnalysisManager *mgr = new AliAnalysisManager("AnalysisManager_QuickTask");

    /* Grid Connection */

    AliAnalysisAlien *alienHandler;

    if (!local) {
        alienHandler = new AliAnalysisAlien();
        alienHandler->SetCheckCopy(kFALSE);
        alienHandler->AddIncludePath("-I. -I$ROOTSYS/include -I$ALICE_ROOT -I$ALICE_ROOT/include -I$ALICE_PHYSICS/include");
        alienHandler->SetAdditionalLibs("AliAnalysisQuickTask.cxx AliAnalysisQuickTask.h");
        alienHandler->SetAnalysisSource("AliAnalysisQuickTask.cxx");
        alienHandler->SetAliPhysicsVersion("vAN-20240807_O2-1");
        alienHandler->SetExecutableCommand("aliroot -l -q -b");
        alienHandler->SetGridDataDir(GRID_DATA_DIR);
        if (!IS_MC) alienHandler->SetRunPrefix("000");
        alienHandler->AddRunNumber(GRID_RUN_NUMBER);
        alienHandler->SetDataPattern(GRID_DATA_PATTERN);
        alienHandler->SetTTL(3600);
        alienHandler->SetOutputToRunNo(kTRUE);
        alienHandler->SetKeepLogs(kTRUE);
        alienHandler->SetMergeViaJDL(kFALSE);
        // alienHandler->SetMaxMergeStages(1);
        alienHandler->SetGridWorkingDir("work");
        alienHandler->SetGridOutputDir("quick_task");
        alienHandler->SetJDLName("QuickTask.jdl");
        alienHandler->SetExecutable("QuickTask.sh");
        mgr->SetGridHandler(alienHandler);
    }

    /* Input Handlers */

    AliESDInputHandler *esdH = new AliESDInputHandler();
    esdH->SetNeedField();  // necessary to get GoldenChi2
    mgr->SetInputEventHandler(esdH);

    AliMCEventHandler *mcH = new AliMCEventHandler();
    mcH->SetReadTR(kFALSE);
    mgr->SetMCtruthEventHandler(mcH);

    /* Add Helper Tasks */

    TString TaskPIDResponse_Options = Form("(%i, 1, 1, %i)", (Int_t)IS_MC, N_PASS);
    AliAnalysisTaskPIDResponse *TaskPIDResponse = reinterpret_cast<AliAnalysisTaskPIDResponse *>(
        gInterpreter->ExecuteMacro("$ALICE_ROOT/ANALYSIS/macros/AddTaskPIDResponse.C" + TaskPIDResponse_Options));
    if (!TaskPIDResponse) return;

    TString TaskPhysicsSelection_Options = Form("(%i)", (Int_t)IS_MC);
    AliPhysicsSelectionTask *TaskPhysicsSelection = reinterpret_cast<AliPhysicsSelectionTask *>(
        gInterpreter->ExecuteMacro("$ALICE_PHYSICS/OADB/macros/AddTaskPhysicsSelection.C" + TaskPhysicsSelection_Options));
    if (!TaskPhysicsSelection) return;

    TString TaskMultSelection_Options = "";  // no options yet
    AliMultSelectionTask *TaskMultSelection = reinterpret_cast<AliMultSelectionTask *>(
        gInterpreter->ExecuteMacro("$ALICE_PHYSICS/OADB/COMMON/MULTIPLICITY/macros/AddTaskMultSelection.C" + TaskMultSelection_Options));
    if (!TaskMultSelection) return;

    /* Add My Task */

    gInterpreter->LoadMacro("AliAnalysisQuickTask.cxx++g");

    TString AddQuickTask_Options = Form("(%f, %i, %i)", SexaquarkMass, 1, 1);
    AliAnalysisQuickTask *task = reinterpret_cast<AliAnalysisQuickTask *>(gInterpreter->ExecuteMacro("AddTask_QuickTask.C" + AddQuickTask_Options));
    if (!task) return;

    /* Init Analysis Manager */

    mgr->SetDebugLevel(3);
    if (!mgr->InitAnalysis()) return;

    /* Start Analysis */

    if (!local) {
        if (gridTest) {
            alienHandler->SetNtestFiles(1);
            alienHandler->SetRunMode("test");
        } else {
            alienHandler->SetNtestFiles(5);
            alienHandler->SetSplitMaxInputFileNumber(5);
            alienHandler->SetRunMode("full");
        }
        mgr->StartAnalysis("grid");
    } else {
        TChain *chain = new TChain("esdTree");
        if (!ChooseNEvents) {
            std::vector<TString> localFiles = {"/home/ceres/borquez/some/sims/LHC20e3a/297595/001/AliESDs.root"};
            for (Int_t ifile = 0; ifile < (Int_t)localFiles.size(); ifile++) {
                chain->AddFile(localFiles[ifile]);
            }
            mgr->StartAnalysis("local", chain);  // read all events
        } else {
            mgr->StartAnalysis("local", chain, ChooseNEvents);  // read first NEvents
        }
    }
}
