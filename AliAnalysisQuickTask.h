#ifndef AliAnalysisQuickTask_H
#define AliAnalysisQuickTask_H

#ifndef ALIANALYSISTASKSE_H
#include "AliAnalysisTaskSE.h"
#endif

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <tuple>
#include <vector>

#include "TArray.h"
#include "TChain.h"
#include "TDatabasePDG.h"
#include "TH1.h"
#include "TH1F.h"
#include "TList.h"
#include "TLorentzVector.h"
#include "TROOT.h"
#include "TString.h"
#include "TTree.h"
#include "TVector3.h"

#include "AliAnalysisManager.h"
#include "AliAnalysisTask.h"
#include "AliExternalTrackParam.h"
#include "AliHelix.h"
#include "AliInputEventHandler.h"
#include "AliLog.h"
#include "AliPIDResponse.h"

#include "AliESD.h"
#include "AliESDEvent.h"
#include "AliESDInputHandler.h"
#include "AliESDVertex.h"
#include "AliESDtrack.h"
#include "AliESDv0.h"

#include "AliMCEvent.h"
#include "AliMCEventHandler.h"
#include "AliMCParticle.h"
#include "AliVVertex.h"

class AliPIDResponse;

class AliAnalysisQuickTask : public AliAnalysisTaskSE {
   public:
    AliAnalysisQuickTask();
    AliAnalysisQuickTask(const char* name, Bool_t IsMC);
    virtual ~AliAnalysisQuickTask();

   public:
    virtual void UserCreateOutputObjects();
    void PrepareTracksHistograms();
    virtual void UserExec(Option_t* option);
    virtual void Terminate(Option_t* option) { return; }

   public:
    /* MC Generated */
    void ProcessMCGen();

   public:
    /* Cuts */
    void DefineTracksCuts(TString cuts_option);

   public:
    /* Tracks */
    void ProcessTracks();
    Bool_t PassesTrackSelection(AliESDtrack* track);
    void PlotStatus(AliESDtrack* track);

   private:
    /* Input options */
    Bool_t fIsMC;                    //

   private:
    /* AliRoot objects */
    AliMCEvent* fMC;                // MC event
    AliVVertex* fMC_PrimaryVertex;  // MC gen. (or true) primary vertex
    AliESDEvent* fESD;              // reconstructed event
    AliPIDResponse* fPIDResponse;   // pid response object
    AliESDVertex* fPrimaryVertex;   // primary vertex
    Double_t fMagneticField;        // magnetic field

   private:
    /* ROOT objects */
    TList* fOutputListOfHists;

   private:
    /* Tracks Histograms */
    TH1F* fHist_Tracks_Eta;
    TH1F* fHist_Tracks_Status;

   private:
    /* Containers -- vectors and hash tables */
    std::unordered_map<Int_t, Int_t> getPdgCode_fromMcIdx;                    //

   private:
    /*** Cuts ***/

    /* Track selection */
    Float_t kMin_Track_P;
    Float_t kMax_Track_P;
    Float_t kMax_Track_Eta;
    Float_t kMin_Track_NTPCClusters;
    Float_t kMax_Track_Chi2PerNTPCClusters;

    AliAnalysisQuickTask(const AliAnalysisQuickTask&);             // not implemented
    AliAnalysisQuickTask& operator=(const AliAnalysisQuickTask&);  // not implemented

    ClassDef(AliAnalysisQuickTask, 1);
};

#endif
