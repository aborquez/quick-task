#ifndef ALIANALYSISQUICKTASK_H
#define ALIANALYSISQUICKTASK_H

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

#define HomogeneousField  // homogeneous field in z direction, required by KFParticle
#include "KFPTrack.h"
#include "KFPVertex.h"
#include "KFParticle.h"
#include "KFParticleBase.h"
#include "KFVertex.h"

class AliPIDResponse;
class KFParticle;
class KFVertex;

/*
 Auxiliary class to make use of protected function KFParticleBase::GetMeasurement()
 (Copied from `/PWGLF/.../AliAnalysisTaskDoubleHypNucTree.h`)
*/
class KFParticleMother : public KFParticle {
   public:
    Bool_t CheckDaughter(KFParticle daughter) {
        Float_t m[8], mV[36], D[3][3];
        return KFParticleBase::GetMeasurement(daughter, m, mV, D);
    }
};

class AliAnalysisQuickTask : public AliAnalysisTaskSE {
   public:
    AliAnalysisQuickTask();
    AliAnalysisQuickTask(const char* name);
    virtual ~AliAnalysisQuickTask();

    virtual void UserCreateOutputObjects();
    void PrepareTracksHistograms();
    virtual void UserExec(Option_t* option);
    virtual void Terminate(Option_t* option) { return; }

    /* MC Generated */
    void ProcessMCGen();

    /* Cuts */
    void DefineTracksCuts(TString cuts_option);
    void DefineV0Cuts(TString cuts_option);

    /* Tracks */
    void ProcessTracks();
    Bool_t PassesTrackSelection(AliESDtrack* track);
    void PlotStatus(AliESDtrack* track);

    /* V0s */
    void KalmanV0Finder();
    Bool_t PassesV0Cuts(KFParticleMother kfV0, KFParticle kfDaughterNeg, KFParticle kfDaughterPos, TLorentzVector lvV0, TLorentzVector lvTrackNeg,
                        TLorentzVector lvTrackPos);

    /* Mathematical Functions */
    Double_t CosinePointingAngle(TLorentzVector lvParticle, Double_t X, Double_t Y, Double_t Z, Double_t refPointX, Double_t refPointY,
                                 Double_t refPointZ);
    Double_t ArmenterosAlpha(Double_t V0_Px, Double_t V0_Py, Double_t V0_Pz, Double_t Neg_Px, Double_t Neg_Py, Double_t Neg_Pz, Double_t Pos_Px,
                             Double_t Pos_Py, Double_t Pos_Pz);
    Double_t ArmenterosQt(Double_t V0_Px, Double_t V0_Py, Double_t V0_Pz, Double_t Neg_Px, Double_t Neg_Py, Double_t Neg_Pz);
    Double_t LinePointDCA(Double_t V0_Px, Double_t V0_Py, Double_t V0_Pz, Double_t V0_X, Double_t V0_Y, Double_t V0_Z, Double_t refPointX,
                          Double_t refPointY, Double_t refPointZ);

    /* Kalman Filter Utilities */
    KFParticle CreateKFParticle(AliExternalTrackParam& track, Double_t mass, Int_t charge);
    KFVertex CreateKFVertex(const AliVVertex& vertex);
    KFParticle TransportKFParticle(KFParticle kfThis, KFParticle kfOther, Int_t pdgThis, Int_t chargeThis);

   private:
    /* AliRoot Objects */
    AliMCEvent* fMC;                //! MC event
    AliVVertex* fMC_PrimaryVertex;  //! MC gen. (or true) primary vertex
    AliESDEvent* fESD;              //! reconstructed event
    AliPIDResponse* fPIDResponse;   //! pid response object
    AliESDVertex* fPrimaryVertex;   //! primary vertex
    Double_t fMagneticField;        //! magnetic field

    /* ROOT Objects */
    TDatabasePDG fPDG;          //!
    TList* fOutputListOfHists;  //!

    /* Tracks Histograms */
    TH1F* fHist_Tracks_NSigmaProton;  //!
    TH1F* fHist_Tracks_NSigmaPion;    //!
    TH1F* fHist_Tracks_Eta;           //!
    TH1F* fHist_Tracks_Status;        //!
    TH1F* fHist_AntiLambda_Mass;      //!

    /* Containers -- Vectors and Hash Tables */
    std::unordered_map<Int_t, Int_t> getPdgCode_fromMcIdx;  //
    std::vector<Int_t> esdIndicesOfAntiProtonTracks;        //
    std::vector<Int_t> esdIndicesOfPiPlusTracks;            //

    /* Cuts -- Track Selection */
    Float_t kMin_Track_P;                    //
    Float_t kMax_Track_P;                    //
    Float_t kMax_Track_Eta;                  //
    Float_t kMin_Track_NTPCClusters;         //
    Float_t kMax_Track_Chi2PerNTPCClusters;  //

    /* Cuts -- V0 Selection */
    Float_t kMin_V0_Mass;            //
    Float_t kMax_V0_Mass;            //
    Float_t kMin_V0_Pt;              //
    Float_t kMax_V0_Eta;             //
    Float_t kMin_V0_CPAwrtPV;        //
    Float_t kMax_V0_CPAwrtPV;        //
    Float_t kMax_V0_DCAwrtPV;        //
    Float_t kMax_V0_DCAbtwDau;       //
    Float_t kMax_V0_DCAnegV0;        //
    Float_t kMax_V0_DCAposV0;        //
    Float_t kMax_V0_ArmPtOverAlpha;  //
    Float_t kMax_V0_Chi2ndf;         //

    AliAnalysisQuickTask(const AliAnalysisQuickTask&);             // not implemented
    AliAnalysisQuickTask& operator=(const AliAnalysisQuickTask&);  // not implemented

    /// \cond CLASSDEF
    ClassDef(AliAnalysisQuickTask, 6);
    /// \endcond
};

#endif
