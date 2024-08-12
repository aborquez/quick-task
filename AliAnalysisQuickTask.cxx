#include "AliAnalysisQuickTask.h"

ClassImp(AliAnalysisQuickTask);

/*
 Empty I/O constructor. Non-persistent members are initialized to their default values from here.
*/
AliAnalysisQuickTask::AliAnalysisQuickTask()
    : AliAnalysisTaskSE(),
      //   fIsMC(0),
      fPDG(),
      fOutputListOfTrees(0),
      fOutputListOfHists(0),
      fMC(0),
      fESD(0),
      fPIDResponse(0),
      kMin_Track_P(0.),
      kMax_Track_P(0.),
      kMax_Track_Eta(0.),
      kMin_Track_NTPCClusters(0.),
      kMax_Track_Chi2PerNTPCClusters(0.),
      kMin_V0_Mass(0.),
      kMax_V0_Mass(0.),
      kMin_V0_Pt(0.),
      kMax_V0_Eta(0.),
      kMin_V0_CPAwrtPV(0.),
      kMax_V0_CPAwrtPV(0.),
      kMax_V0_DCAwrtPV(0.),
      kMax_V0_DCAbtwDau(0.),
      kMax_V0_DCAnegV0(0.),
      kMax_V0_DCAposV0(0.),
      kMax_V0_ArmPtOverAlpha(0.),
      kMax_V0_Chi2ndf(0.) {
    //
}

/*
 Constructor, called locally.
*/
AliAnalysisQuickTask::AliAnalysisQuickTask(const char* name)
    : AliAnalysisTaskSE(name),
      //   fIsMC(0),
      fPDG(),
      fOutputListOfTrees(0),
      fOutputListOfHists(0),
      fMC(0),
      fESD(0),
      fPIDResponse(0),
      kMin_Track_P(0.),
      kMax_Track_P(0.),
      kMax_Track_Eta(0.),
      kMin_Track_NTPCClusters(0.),
      kMax_Track_Chi2PerNTPCClusters(0.),
      kMin_V0_Mass(0.),
      kMax_V0_Mass(0.),
      kMin_V0_Pt(0.),
      kMax_V0_Eta(0.),
      kMin_V0_CPAwrtPV(0.),
      kMax_V0_CPAwrtPV(0.),
      kMax_V0_DCAwrtPV(0.),
      kMax_V0_DCAbtwDau(0.),
      kMax_V0_DCAnegV0(0.),
      kMax_V0_DCAposV0(0.),
      kMax_V0_ArmPtOverAlpha(0.),
      kMax_V0_Chi2ndf(0.) {
    DefineInput(0, TChain::Class());
    DefineOutput(1, TList::Class());  // fOutputListOfTrees
    DefineOutput(2, TList::Class());  // fOutputListOfHists
}

/*
 Destructor.
*/
AliAnalysisQuickTask::~AliAnalysisQuickTask() {
    if (fOutputListOfTrees) delete fOutputListOfTrees;
    if (fOutputListOfHists) delete fOutputListOfHists;
}

/*
 Create output objects, called once at RUNTIME ~ execution on Grid
*/
void AliAnalysisQuickTask::UserCreateOutputObjects() {

    /** Add mandatory routines **/

    AliAnalysisManager* man = AliAnalysisManager::GetAnalysisManager();
    if (!man) AliFatal("ERROR: AliAnalysisManager couldn't be found.");

    AliESDInputHandler* inputHandler = (AliESDInputHandler*)(man->GetInputEventHandler());
    if (!inputHandler) AliFatal("ERROR: AliESDInputHandler couldn't be found.");

    fPIDResponse = inputHandler->GetPIDResponse();

    /** Prepare Output **/

    /* Trees */

    fOutputListOfTrees = new TList();
    fOutputListOfTrees->SetOwner(kTRUE);

    fLogTree = ReadLogs("/alice/sim/2023/LHC23l1a3/A1.8", 297595, 1);
    fLogTree->Print();
    fOutputListOfTrees->Add(fLogTree);

    /* Histograms */

    fOutputListOfHists = new TList();
    fOutputListOfHists->SetOwner(kTRUE);

    fHist_Tracks_NSigmaProton = new TH1F("NSigmaProton", "", 100, -5., 5.);
    fOutputListOfHists->Add(fHist_Tracks_NSigmaProton);

    fHist_Tracks_NSigmaPion = new TH1F("NSigmaPion", "", 100, -5., 5.);
    fOutputListOfHists->Add(fHist_Tracks_NSigmaPion);

    fHist_Tracks_Eta = new TH1F("Tracks_Eta", "", 100, -0.8, 0.8);
    fOutputListOfHists->Add(fHist_Tracks_Eta);

    fHist_Tracks_Status = new TH1F("Status", "", 20, 0., 20);
    fOutputListOfHists->Add(fHist_Tracks_Status);

    fHist_AntiLambda_Mass = new TH1F("AntiLambda_Mass", "", 100, 0.5, 1.5);
    fOutputListOfHists->Add(fHist_AntiLambda_Mass);

    PostData(1, fOutputListOfTrees);
    PostData(2, fOutputListOfHists);
}

/*
 Main function, called per each event at RUNTIME ~ execution on Grid
 - Uses: `fIsMC`, `fMC_PrimaryVertex`, `fESD`, `fMagneticField`, `fPrimaryVertex`, `fSourceOfV0s`, `fReactionChannel`, `fOutputListOfTrees`,
 `fOutputListOfHists`
*/
void AliAnalysisQuickTask::UserExec(Option_t*) {

    Bool_t MB = (fInputHandler->IsEventSelected() & AliVEvent::kINT7);
    if (!MB) return;

    fMC = MCEvent();
    if (!fMC) return;

    fMC_PrimaryVertex = const_cast<AliVVertex*>(fMC->GetPrimaryVertex());

    fESD = dynamic_cast<AliESDEvent*>(InputEvent());
    if (!fESD) return;

    AliInfoF("!! Run Number %i !!", fESD->GetRunNumber());
    AliInfoF("!! Period Number %i !!", fESD->GetPeriodNumber());

    fMagneticField = fESD->GetMagneticField();

    fPrimaryVertex = const_cast<AliESDVertex*>(fESD->GetPrimaryVertex());

    DefineTracksCuts("");
    DefineV0Cuts("");

    ProcessMCGen();

    ProcessTracks();

    KalmanV0Finder();

    /* Clear Containers */

    getPdgCode_fromMcIdx.clear();
    esdIndicesOfAntiProtonTracks.clear();
    esdIndicesOfPiPlusTracks.clear();

    PostData(1, fOutputListOfTrees);
    PostData(2, fOutputListOfHists);
}

/*
 Define track selection cuts.
 - Input: `cuts_option`
*/
void AliAnalysisQuickTask::DefineTracksCuts(TString cuts_option) {
    kMin_Track_P = 0.3;
    kMax_Track_P = 5.;
    kMax_Track_Eta = 0.8;
    kMin_Track_NTPCClusters = 50;
    kMax_Track_Chi2PerNTPCClusters = 7.;
}

/*
 Loop over MC particles in a single event. Store the indices of the signal particles.
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

/*                   */
/**  Reconstructed  **/
/*** ============= ***/

/*
 Loop over the reconstructed tracks in a single event.
*/
void AliAnalysisQuickTask::ProcessTracks() {

    AliESDtrack* track;

    for (Int_t esdIdxTrack = 0; esdIdxTrack < fESD->GetNumberOfTracks(); esdIdxTrack++) {

        track = static_cast<AliESDtrack*>(fESD->GetTrack(esdIdxTrack));

        if (!PassesTrackSelection(track)) continue;

        /* Store tracks indices */

        if (track->Charge() < 0 && TMath::Abs(fPIDResponse->NumberOfSigmasTPC(track, AliPID::kProton)) < 3.) {
            esdIndicesOfAntiProtonTracks.push_back(esdIdxTrack);
        }

        if (track->Charge() > 0 && TMath::Abs(fPIDResponse->NumberOfSigmasTPC(track, AliPID::kPion)) < 3.) {
            esdIndicesOfPiPlusTracks.push_back(esdIdxTrack);
        }

        /* Fill histograms */

        fHist_Tracks_NSigmaProton->Fill(fPIDResponse->NumberOfSigmasTPC(track, AliPID::kProton));
        fHist_Tracks_NSigmaPion->Fill(fPIDResponse->NumberOfSigmasTPC(track, AliPID::kPion));
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

    // >> p
    if (!track->GetInnerParam()) return kFALSE;
    if (kMin_Track_P && track->GetInnerParam()->GetP() < kMin_Track_P) return kFALSE;
    if (kMax_Track_P && track->GetInnerParam()->GetP() > kMax_Track_P) return kFALSE;

    // >> eta
    if (kMax_Track_Eta && TMath::Abs(track->Eta()) > kMax_Track_Eta) return kFALSE;

    // >> TPC clusters
    if (kMin_Track_NTPCClusters && track->GetTPCNcls() < kMin_Track_NTPCClusters) return kFALSE;

    // >> chi2 per TPC cluster
    if (kMax_Track_Chi2PerNTPCClusters && track->GetTPCchi2() / (Double_t)track->GetTPCNcls() > kMax_Track_Chi2PerNTPCClusters) return kFALSE;

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

/*                          */
/**  V0s -- Kalman Filter  **/
/*** ==================== ***/

/*
 Define V0 selection cuts.
 - Uses: `fSourceOfV0s`
 - Input: `cuts_option`
*/
void AliAnalysisQuickTask::DefineV0Cuts(TString cuts_option) {

    kMin_V0_Mass = 1.08;
    kMax_V0_Mass = 1.16;
    kMin_V0_Pt = 1.0;
    kMax_V0_Eta = 0.9;

    kMin_V0_CPAwrtPV = 0.99;
    kMax_V0_CPAwrtPV = 1.;
    kMax_V0_DCAwrtPV = 1.;
    kMax_V0_DCAbtwDau = 2.;
    kMax_V0_DCAnegV0 = 2.;
    kMax_V0_DCAposV0 = 2.;
    kMax_V0_ArmPtOverAlpha = 0.2;
    kMax_V0_Chi2ndf = 10.;
}

/*
 Find all V0s via Kalman Filter.
*/
void AliAnalysisQuickTask::KalmanV0Finder() {

    AliESDtrack* esdTrackNeg;
    AliESDtrack* esdTrackPos;

    /* Define primary vertex as a KFVertex */

    KFVertex kfPrimaryVertex = CreateKFVertex(*fPrimaryVertex);

    /* Declare TLorentzVectors */

    TLorentzVector lvTrackNeg;
    TLorentzVector lvTrackPos;
    TLorentzVector lvV0;

    const Int_t pdgV0 = -3122;
    const Int_t pdgTrackNeg = -2212;
    const Int_t pdgTrackPos = 211;

    /* Loop over all possible pairs of tracks */

    for (Int_t& esdIdxNeg : esdIndicesOfAntiProtonTracks) {
        for (Int_t& esdIdxPos : esdIndicesOfPiPlusTracks) {

            /* Sanity check */

            if (esdIdxNeg == esdIdxPos) continue;

            /* Get tracks */

            esdTrackNeg = static_cast<AliESDtrack*>(fESD->GetTrack(esdIdxNeg));
            esdTrackPos = static_cast<AliESDtrack*>(fESD->GetTrack(esdIdxPos));

            /* Kalman Filter */

            KFParticle kfDaughterNeg = CreateKFParticle(*esdTrackNeg, fPDG.GetParticle(pdgTrackNeg)->Mass(), (Int_t)esdTrackNeg->Charge());
            KFParticle kfDaughterPos = CreateKFParticle(*esdTrackPos, fPDG.GetParticle(pdgTrackPos)->Mass(), (Int_t)esdTrackPos->Charge());

            KFParticleMother kfV0;
            kfV0.AddDaughter(kfDaughterNeg);
            kfV0.AddDaughter(kfDaughterPos);

            /* Transport V0 and daughters */

            kfV0.TransportToDecayVertex();

            KFParticle kfTransportedNeg = TransportKFParticle(kfDaughterNeg, kfDaughterPos, pdgTrackNeg, (Int_t)esdTrackNeg->Charge());
            KFParticle kfTransportedPos = TransportKFParticle(kfDaughterPos, kfDaughterNeg, pdgTrackPos, (Int_t)esdTrackPos->Charge());

            /* Reconstruct V0 */

            lvTrackNeg.SetXYZM(kfTransportedNeg.Px(), kfTransportedNeg.Py(), kfTransportedNeg.Pz(), fPDG.GetParticle(pdgTrackNeg)->Mass());
            lvTrackPos.SetXYZM(kfTransportedPos.Px(), kfTransportedPos.Py(), kfTransportedPos.Pz(), fPDG.GetParticle(pdgTrackPos)->Mass());
            lvV0 = lvTrackNeg + lvTrackPos;

            /* Apply cuts and fill hist */

            if (!PassesV0Cuts(kfV0, kfDaughterNeg, kfDaughterPos, lvV0, lvTrackNeg, lvTrackPos)) continue;

            fHist_AntiLambda_Mass->Fill(lvV0.M());
        }  // end of loop over pos. tracks
    }      // end of loop over neg. tracks
}

/*
 Apply cuts to a V0 candidate.
*/
Bool_t AliAnalysisQuickTask::PassesV0Cuts(KFParticleMother kfV0, KFParticle kfDaughterNeg, KFParticle kfDaughterPos, TLorentzVector lvV0,
                                          TLorentzVector lvTrackNeg, TLorentzVector lvTrackPos) {

    Double_t Mass = lvV0.M();
    if (kMin_V0_Mass && Mass < kMin_V0_Mass) return kFALSE;
    if (kMax_V0_Mass && Mass > kMax_V0_Mass) return kFALSE;

    Double_t Pt = lvV0.Pt();
    if (kMin_V0_Pt && Pt < kMin_V0_Pt) return kFALSE;

    Double_t Eta = TMath::Abs(lvV0.Eta());
    if (kMax_V0_Eta && Eta > kMax_V0_Eta) return kFALSE;

    Double_t CPAwrtPV =
        CosinePointingAngle(lvV0, kfV0.GetX(), kfV0.GetY(), kfV0.GetZ(), fPrimaryVertex->GetX(), fPrimaryVertex->GetY(), fPrimaryVertex->GetZ());
    if (kMin_V0_CPAwrtPV && CPAwrtPV < kMin_V0_CPAwrtPV) return kFALSE;
    if (kMax_V0_CPAwrtPV && CPAwrtPV > kMax_V0_CPAwrtPV) return kFALSE;

    Double_t DCAwrtPV = LinePointDCA(lvV0.Px(), lvV0.Py(), lvV0.Pz(), kfV0.GetX(), kfV0.GetY(), kfV0.GetZ(), fPrimaryVertex->GetX(),
                                     fPrimaryVertex->GetY(), fPrimaryVertex->GetZ());
    if (kMax_V0_DCAwrtPV && DCAwrtPV > kMax_V0_DCAwrtPV) return kFALSE;

    Double_t DCAbtwDau = TMath::Abs(kfDaughterNeg.GetDistanceFromParticle(kfDaughterPos));
    if (kMax_V0_DCAbtwDau && DCAbtwDau > kMax_V0_DCAbtwDau) return kFALSE;

    Double_t DCAnegV0 = TMath::Abs(kfDaughterNeg.GetDistanceFromVertex(kfV0));
    if (kMax_V0_DCAnegV0 && DCAnegV0 > kMax_V0_DCAnegV0) return kFALSE;

    Double_t DCAposV0 = TMath::Abs(kfDaughterPos.GetDistanceFromVertex(kfV0));
    if (kMax_V0_DCAposV0 && DCAposV0 > kMax_V0_DCAposV0) return kFALSE;

    Double_t ArmPt = ArmenterosQt(lvV0.Px(), lvV0.Py(), lvV0.Pz(), lvTrackNeg.Px(), lvTrackNeg.Py(), lvTrackNeg.Pz());
    Double_t ArmAlpha = ArmenterosAlpha(lvV0.Px(), lvV0.Py(), lvV0.Pz(), lvTrackNeg.Px(), lvTrackNeg.Py(), lvTrackNeg.Pz(), lvTrackPos.Px(),
                                        lvTrackPos.Py(), lvTrackPos.Pz());
    Double_t ArmPtOverAlpha = TMath::Abs(ArmPt / ArmAlpha);
    if (kMax_V0_ArmPtOverAlpha && ArmPtOverAlpha > kMax_V0_ArmPtOverAlpha) return kFALSE;

    Double_t Chi2ndf = (Double_t)kfV0.GetChi2() / (Double_t)kfV0.GetNDF();
    if (kMax_V0_Chi2ndf && Chi2ndf > kMax_V0_Chi2ndf) return kFALSE;

    return kTRUE;
}

/*                            */
/**  Mathematical Functions  **/
/*** ====================== ***/

Double_t AliAnalysisQuickTask::CosinePointingAngle(TLorentzVector lvParticle, Double_t X, Double_t Y, Double_t Z, Double_t refPointX,
                                                   Double_t refPointY, Double_t refPointZ) {
    TVector3 posRelativeToRef(X - refPointX, Y - refPointY, Z - refPointZ);
    return TMath::Cos(lvParticle.Angle(posRelativeToRef));
}

Double_t AliAnalysisQuickTask::ArmenterosAlpha(Double_t V0_Px, Double_t V0_Py, Double_t V0_Pz, Double_t Neg_Px, Double_t Neg_Py, Double_t Neg_Pz,
                                               Double_t Pos_Px, Double_t Pos_Py, Double_t Pos_Pz) {
    TVector3 momTot(V0_Px, V0_Py, V0_Pz);
    TVector3 momNeg(Neg_Px, Neg_Py, Neg_Pz);
    TVector3 momPos(Pos_Px, Pos_Py, Pos_Pz);

    Double_t lQlNeg = momNeg.Dot(momTot) / momTot.Mag();
    Double_t lQlPos = momPos.Dot(momTot) / momTot.Mag();

    // (protection)
    if (lQlPos + lQlNeg == 0.) {
        return 2;
    }  // closure
    return (lQlPos - lQlNeg) / (lQlPos + lQlNeg);
}

Double_t AliAnalysisQuickTask::ArmenterosQt(Double_t V0_Px, Double_t V0_Py, Double_t V0_Pz, Double_t Neg_Px, Double_t Neg_Py, Double_t Neg_Pz) {
    TVector3 momTot(V0_Px, V0_Py, V0_Pz);
    TVector3 momNeg(Neg_Px, Neg_Py, Neg_Pz);

    return momNeg.Perp(momTot);
}

Double_t AliAnalysisQuickTask::LinePointDCA(Double_t V0_Px, Double_t V0_Py, Double_t V0_Pz, Double_t V0_X, Double_t V0_Y, Double_t V0_Z,
                                            Double_t refPointX, Double_t refPointY, Double_t refPointZ) {

    TVector3 V0Momentum(V0_Px, V0_Py, V0_Pz);
    TVector3 V0Vertex(V0_X, V0_Y, V0_Z);
    TVector3 RefVertex(refPointX, refPointY, refPointZ);

    TVector3 CrossProduct = (RefVertex - V0Vertex).Cross(V0Momentum);

    return CrossProduct.Mag() / V0Momentum.Mag();
}

/*                             */
/**  Kalman Filter Functions  **/
/*** ======================= ***/

/*
 Correct initialization of a KFParticle.
 (Copied from `AliPhysics/PWGLF/.../AliAnalysisTaskDoubleHypNucTree.cxx`)
*/
KFParticle AliAnalysisQuickTask::CreateKFParticle(AliExternalTrackParam& track, Double_t mass, Int_t charge) {

    Double_t fP[6];
    track.GetXYZ(fP);
    track.PxPyPz(fP + 3);

    Int_t fQ = track.Charge() * TMath::Abs(charge);
    fP[3] *= TMath::Abs(charge);
    fP[4] *= TMath::Abs(charge);
    fP[5] *= TMath::Abs(charge);

    Double_t pt = 1. / TMath::Abs(track.GetParameter()[4]) * TMath::Abs(charge);
    Double_t cs = TMath::Cos(track.GetAlpha());
    Double_t sn = TMath::Sin(track.GetAlpha());
    Double_t r = TMath::Sqrt((1. - track.GetParameter()[2]) * (1. + track.GetParameter()[2]));

    Double_t m00 = -sn;
    Double_t m10 = cs;
    Double_t m23 = -pt * (sn + track.GetParameter()[2] * cs / r);
    Double_t m43 = -pt * pt * (r * cs - track.GetParameter()[2] * sn);
    Double_t m24 = pt * (cs - track.GetParameter()[2] * sn / r);
    Double_t m44 = -pt * pt * (r * sn + track.GetParameter()[2] * cs);
    Double_t m35 = pt;
    Double_t m45 = -pt * pt * track.GetParameter()[3];

    m43 *= track.GetSign();
    m44 *= track.GetSign();
    m45 *= track.GetSign();

    const Double_t* cTr = track.GetCovariance();
    Double_t fC[21];
    fC[0] = cTr[0] * m00 * m00;
    fC[1] = cTr[0] * m00 * m10;
    fC[2] = cTr[0] * m10 * m10;
    fC[3] = cTr[1] * m00;
    fC[4] = cTr[1] * m10;
    fC[5] = cTr[2];
    fC[6] = m00 * (cTr[3] * m23 + cTr[10] * m43);
    fC[7] = m10 * (cTr[3] * m23 + cTr[10] * m43);
    fC[8] = cTr[4] * m23 + cTr[11] * m43;
    fC[9] = m23 * (cTr[5] * m23 + cTr[12] * m43) + m43 * (cTr[12] * m23 + cTr[14] * m43);
    fC[10] = m00 * (cTr[3] * m24 + cTr[10] * m44);
    fC[11] = m10 * (cTr[3] * m24 + cTr[10] * m44);
    fC[12] = cTr[4] * m24 + cTr[11] * m44;
    fC[13] = m23 * (cTr[5] * m24 + cTr[12] * m44) + m43 * (cTr[12] * m24 + cTr[14] * m44);
    fC[14] = m24 * (cTr[5] * m24 + cTr[12] * m44) + m44 * (cTr[12] * m24 + cTr[14] * m44);
    fC[15] = m00 * (cTr[6] * m35 + cTr[10] * m45);
    fC[16] = m10 * (cTr[6] * m35 + cTr[10] * m45);
    fC[17] = cTr[7] * m35 + cTr[11] * m45;
    fC[18] = m23 * (cTr[8] * m35 + cTr[12] * m45) + m43 * (cTr[13] * m35 + cTr[14] * m45);
    fC[19] = m24 * (cTr[8] * m35 + cTr[12] * m45) + m44 * (cTr[13] * m35 + cTr[14] * m45);
    fC[20] = m35 * (cTr[9] * m35 + cTr[13] * m45) + m45 * (cTr[13] * m35 + cTr[14] * m45);

    KFParticle part;
    part.Create(fP, fC, fQ, mass);

    return part;
}

/*
 Correct initialization of a KFVertex.
 (Copied from `AliPhysics/PWGLF/.../AliAnalysisTaskDoubleHypNucTree.cxx`)
*/
KFVertex AliAnalysisQuickTask::CreateKFVertex(const AliVVertex& vertex) {

    Double_t param[6];
    vertex.GetXYZ(param);

    Double_t cov[6];
    vertex.GetCovarianceMatrix(cov);

    KFPVertex kfpVtx;
    Float_t paramF[3] = {(Float_t)param[0], (Float_t)param[1], (Float_t)param[2]};
    kfpVtx.SetXYZ(paramF);
    Float_t covF[6] = {(Float_t)cov[0], (Float_t)cov[1], (Float_t)cov[2], (Float_t)cov[3], (Float_t)cov[4], (Float_t)cov[5]};
    kfpVtx.SetCovarianceMatrix(covF);

    KFVertex KFVtx(kfpVtx);

    return KFVtx;
}

/*
 Transport a KFParticle to the point of closest approach w.r.t. another KFParticle.
 - Uses: `fPDG`
 - Input: `kfThis`, `kfOther`, `pdgThis`, `chargeThis`
 - Return: `kfTransported`
*/
KFParticle AliAnalysisQuickTask::TransportKFParticle(KFParticle kfThis, KFParticle kfOther, Int_t pdgThis, Int_t chargeThis) {

    float dS[2];
    float dsdr[4][6];
    kfThis.GetDStoParticle(kfOther, dS, dsdr);

    float mP[8], mC[36];
    kfThis.Transport(dS[0], dsdr[0], mP, mC);

    float mM = fPDG.GetParticle(pdgThis)->Mass();
    float mQ = chargeThis;  // only valid for charged particles with Q = +/- 1

    KFParticle kfTransported;
    kfTransported.Create(mP, mC, mQ, mM);

    return kfTransported;
}

/*                    */
/**  External Files  **/
/*** ============== ***/

/*
 Hola hola.
*/
TTree* AliAnalysisQuickTask::ReadLogs(TString input_dir, Int_t run_number, Int_t dir_number) {

    TTree* t = new TTree("Injected", "Injected");  // i.e., before the antisexaquark-nucleon interaction

    TGrid* alien = nullptr;
    if (!gGrid) {
        // alien = TGrid::Connect("alien://", 0, 0, "t");
        alien = TGrid::Connect("alien://");
        if (!alien) return t;
    }

    TString filename = "sim.log";
    gSystem->Exec(Form("alien.py cp alien://%s/%i/%03i/%s file://.", input_dir.Data(), run_number, dir_number, filename.Data()));
    TString new_path = Form("%s/%s", gSystem->pwd(), filename.Data());
    AliInfoF("!! Reading file = %s !!", new_path.Data());

    std::ifstream SimLog(new_path);
    if (!SimLog.is_open()) {
        AliInfo("!! Unable to open file !!");
        return t;
    }

    Char_t ReactionChannel;

    Int_t RunNumber = run_number;
    Int_t DirNumber = dir_number;

    Int_t EventID = -1;
    Int_t ReactionID;
    Int_t NPDGCode;
    Double_t SPx, SPy, SPz, SM;
    Double_t NPx, NPy, NPz;

    t->Branch("RunNumber", &RunNumber);
    t->Branch("DirNumber", &DirNumber);
    t->Branch("ReactionChannel", &ReactionChannel);
    t->Branch("EventID", &EventID);
    t->Branch("ReactionID", &ReactionID);
    t->Branch("Nucl_PDGCode", &NPDGCode);
    t->Branch("Sexa_Px_ini", &SPx);
    t->Branch("Sexa_Py_ini", &SPy);
    t->Branch("Sexa_Pz_ini", &SPz);
    t->Branch("Sexa_M_ini", &SM);
    t->Branch("Fermi_Px", &NPx);
    t->Branch("Fermi_Py", &NPy);
    t->Branch("Fermi_Pz", &NPz);

    // auxiliary variables
    std::string cstr_line;
    TString tstr_line, csv;
    TObjArray* csv_arr = nullptr;

    /* Read lines */

    while (std::getline(SimLog, cstr_line)) {

        tstr_line = cstr_line;

        if (!ReactionChannel) {
            if (tstr_line.Contains("I-AliGenSexaquarkReaction::PrintParameters:   Chosen Reaction Channel : ")) {
                ReactionChannel = tstr_line(71);
                std::cout << "!! ReadLogs !! reaction channel: " << ReactionChannel << " !!" << std::endl;
            }
        }

        if (!SM) {
            if (tstr_line.Contains("I-AliGenSexaquarkReaction::PrintParameters:   * M = ")) {
                SM = TString(tstr_line(51, 59)).Atof();  // assuming M with length 8
                std::cout << "!! ReadLogs !! sexaquark mass: " << SM << " !!" << std::endl;
            }
        }

        // a new event has appeared
        if (tstr_line.Contains("I-AliGenCocktail::Generate: Generator 1: AliGenHijing")) EventID++;

        if (!tstr_line.Contains("I-AliGenSexaquarkReaction::GenerateN: 6")) continue;

        csv = static_cast<TString>(tstr_line(38, tstr_line.Length() - 1));
        csv_arr = csv.Tokenize(",");

        ReactionID = dynamic_cast<TObjString*>(csv_arr->At(0))->String().Atoi();
        NPDGCode = ReactionChannel == 'A' ? 2112 : 2212;  // considering only ADEH
        SPx = dynamic_cast<TObjString*>(csv_arr->At(1))->String().Atof();
        SPy = dynamic_cast<TObjString*>(csv_arr->At(2))->String().Atof();
        SPz = dynamic_cast<TObjString*>(csv_arr->At(3))->String().Atof();
        NPx = dynamic_cast<TObjString*>(csv_arr->At(4))->String().Atof();
        NPy = dynamic_cast<TObjString*>(csv_arr->At(5))->String().Atof();
        NPz = dynamic_cast<TObjString*>(csv_arr->At(6))->String().Atof();

        t->Fill();
    }  // end of loop over lines

    SimLog.close();

    return t;
}
