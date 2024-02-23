#include "AliAnalysisQuickTask.h"

ClassImp(AliAnalysisQuickTask);

/*
 Empty I/O constructor. Non-persistent members are initialized to their default values from here.
*/
AliAnalysisQuickTask::AliAnalysisQuickTask()
    : AliAnalysisTaskSE(),
      fIsMC(0),
      fOutputListOfHists(0),
      fMC(0),
      fESD(0),
      fPIDResponse(0),
      kMax_Track_Eta(0.),
      kMin_Track_NTPCClusters(0.),
      kMax_Track_Chi2PerNTPCClusters(0.) {}

/*
 Constructor, called locally.
*/
AliAnalysisQuickTask::AliAnalysisQuickTask(const char* name, Bool_t IsMC)
    : AliAnalysisTaskSE(name),
      fIsMC(IsMC),
      fOutputListOfHists(0),
      fMC(0),
      fESD(0),
      fPIDResponse(0),
      kMax_Track_Eta(0.),
      kMin_Track_NTPCClusters(0.),
      kMax_Track_Chi2PerNTPCClusters(0.) {
    DefineInput(0, TChain::Class());
    DefineOutput(1, TList::Class());
}

/*
 Destructor.
*/
AliAnalysisQuickTask::~AliAnalysisQuickTask() {
    if (fOutputListOfHists) {
        delete fOutputListOfHists;
    }
}

/*
 Create output objects, called once at RUNTIME ~ execution on Grid
*/
void AliAnalysisQuickTask::UserCreateOutputObjects() {

    /** Add mandatory routines **/

    AliAnalysisManager* man = AliAnalysisManager::GetAnalysisManager();
    if (!man) {
        AliFatal("ERROR: AliAnalysisManager couldn't be found.");
    }

    AliESDInputHandler* inputHandler = (AliESDInputHandler*)(man->GetInputEventHandler());
    if (!inputHandler) {
        AliFatal("ERROR: AliESDInputHandler couldn't be found.");
    }

    fPIDResponse = inputHandler->GetPIDResponse();

    /** Prepare output histograms */

    fOutputListOfHists = new TList();
    fOutputListOfHists->SetOwner(kTRUE);

    fHist_Tracks_Eta = new TH1F("Eta", "", 100, -0.8, 0.8);
    fOutputListOfHists->Add(fHist_Tracks_Eta);

    fHist_Tracks_Status = new TH1F("Status", "", 20, 0., 20);
    fOutputListOfHists->Add(fHist_Tracks_Status);

    PostData(1, fOutputListOfHists);
}

/*
 Main function, called per each event at RUNTIME ~ execution on Grid
 - Uses: `fIsMC`, `fMC_PrimaryVertex`, `fESD`, `fMagneticField`, `fPrimaryVertex`, `fSourceOfV0s`, `fReactionChannel`, `fOutputListOfTrees`,
 `fOutputListOfHists`
*/
void AliAnalysisQuickTask::UserExec(Option_t*) {

    fMC = MCEvent();

    if (!fMC) {
        AliFatal("ERROR: AliMCEvent couldn't be found.");
    }

    fMC_PrimaryVertex = const_cast<AliVVertex*>(fMC->GetPrimaryVertex());

    fESD = dynamic_cast<AliESDEvent*>(InputEvent());

    if (!fESD) {
        AliFatal("ERROR: AliESDEvent couldn't be found.");
    }

    fMagneticField = fESD->GetMagneticField();

    fPrimaryVertex = const_cast<AliESDVertex*>(fESD->GetPrimaryVertex());

    DefineTracksCuts("");

    if (fIsMC) ProcessMCGen();

    ProcessTracks();

    getPdgCode_fromMcIdx.clear();

    // stream the results the analysis of this event to the output manager
    PostData(1, fOutputListOfHists);
}

/*
 Define track selection cuts.
 - Input: `cuts_option`
*/
void AliAnalysisQuickTask::DefineTracksCuts(TString cuts_option) {

    kMin_Track_P = 1.;
    kMax_Track_P = 2.;
    kMax_Track_Eta = 0.8;
    kMin_Track_NTPCClusters = 50;
    kMax_Track_Chi2PerNTPCClusters = 7.;
}

/*
 Loop over MC particles in a single event. Store the indices of the signal particles.
 - Uses: `fMC`, `fPDG`, `fMC_PrimaryVertex`
*/
void AliAnalysisQuickTask::ProcessMCGen() {

    AliMCParticle* mcPart;
    Int_t pdg_mc;

    for (Int_t mcIdx = 0; mcIdx < fMC->GetNumberOfTracks(); mcIdx++) {

        mcPart = (AliMCParticle*)fMC->GetTrack(mcIdx);
        pdg_mc = mcPart->PdgCode();

        if (pdg_mc != 2212) continue;
        if (!mcPart->IsPhysicalPrimary()) continue;

        getPdgCode_fromMcIdx[mcIdx] = pdg_mc;
    }
}

/*
 Loop over the reconstructed tracks in a single event.
*/
void AliAnalysisQuickTask::ProcessTracks() {

    AliESDtrack* track;

    Int_t mcIdx;
    Int_t mcPdgCode;
    Int_t mcIdxOfTrueV0;

    Float_t pt, pz, eta;
    Float_t impar_pv[2], dca_wrt_pv;
    Float_t n_tpc_clusters;
    Float_t chi2_over_nclusters;
    Float_t n_sigma_proton;
    Float_t n_sigma_kaon;
    Float_t n_sigma_pion;
    Float_t golden_chi2;

    /* Loop over tracks in a single event */

    for (Int_t esdIdxTrack = 0; esdIdxTrack < fESD->GetNumberOfTracks(); esdIdxTrack++) {

        /* Get track */

        track = static_cast<AliESDtrack*>(fESD->GetTrack(esdIdxTrack));

        /* Get MC info */

        mcIdx = TMath::Abs(track->GetLabel());
        mcPdgCode = getPdgCode_fromMcIdx[mcIdx];

        /* Apply track selection */

        if (!PassesTrackSelection(track)) continue;

        if (mcPdgCode != 2212) continue;

        /* Fill histograms */

        fHist_Tracks_Eta->Fill(track->Eta());
        PlotStatus(track);
    }  // end of loop over tracks
}

/*
 Determine if current AliESDtrack passes track selection,
 and fill the bookkeeping histograms to measure the effect of the cuts.
 - Input: `track`
 - Return: `kTRUE` if the candidate passes the cuts, `kFALSE` otherwise
*/
Bool_t AliAnalysisQuickTask::PassesTrackSelection(AliESDtrack* track) {

    // if (!track->GetInnerParam()) return kFALSE;

    // >> p
    // if (track->GetInnerParam()->GetP() > kMax_Track_P) return kFALSE;
    // if (track->GetInnerParam()->GetP() < kMin_Track_P) return kFALSE;

    // >> eta
    if (TMath::Abs(track->Eta()) > kMax_Track_Eta) return kFALSE;

    // >> TPC clusters
    if (track->GetTPCNcls() < kMin_Track_NTPCClusters) return kFALSE;

    // >> chi2 per TPC cluster
    if (track->GetTPCchi2() / (Double_t)track->GetTPCNcls() > kMax_Track_Chi2PerNTPCClusters) return kFALSE;

    return kTRUE;
}

/*
 Plot the status of the track.
 - Input: `track`, `stage`, `esdPdgCode`
*/
void AliAnalysisQuickTask::PlotStatus(AliESDtrack* track) {

    ULong64_t StatusCollection[20] = {AliESDtrack::kITSin,   AliESDtrack::kITSout,     AliESDtrack::kITSrefit,    AliESDtrack::kITSpid,
                                      AliESDtrack::kTPCin,   AliESDtrack::kTPCout,     AliESDtrack::kTPCrefit,    AliESDtrack::kTPCpid,
                                      AliESDtrack::kTOFin,   AliESDtrack::kTOFout,     AliESDtrack::kTOFrefit,    AliESDtrack::kTOFpid,
                                      AliESDtrack::kITSupg,  AliESDtrack::kSkipFriend, AliESDtrack::kGlobalMerge, AliESDtrack::kMultInV0,
                                      AliESDtrack::kMultSec, AliESDtrack::kEmbedded,   AliESDtrack::kITSpureSA,   AliESDtrack::kESDpid};

    for (Int_t i = 0; i < 20; i++) {
        if ((track->GetStatus() & StatusCollection[i])) fHist_Tracks_Status->Fill(i);
    }
}
