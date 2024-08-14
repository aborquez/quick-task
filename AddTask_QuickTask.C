/* --- */

TH1D *GetPtWeights(TString filename, Float_t SexaquarkMass);
TH1F *GetRadiusWeights(TString filename);

/* --- */

AliAnalysisQuickTask *AddTask_QuickTask(Float_t SexaquarkMass = 1.8, Bool_t ReweightPt = kFALSE, Bool_t ReweightRadius = kFALSE) {

    AliAnalysisManager *mgr = AliAnalysisManager::GetAnalysisManager();
    if (!mgr) return nullptr;

    AliAnalysisQuickTask *task = new AliAnalysisQuickTask("AnalysisTask_QuickTask");
    if (!task) return nullptr;

    /* --- */

    task->ReweightPt(ReweightPt);
    task->ReweightRadius(ReweightRadius);

    TGrid *alien = nullptr;
    if ((ReweightPt || ReweightRadius) && !gGrid) {
        alien = TGrid::Connect("alien://");
        if (!alien) {
            std::cerr << "!! AddTask_QuickTask !! Couldn't connect to the Grid !!" << std::endl;
            return nullptr;
        }
    }

    TH1D *PtWeightsHist = nullptr;
    if (ReweightPt) {
        TString pt_weights_path = "alien:///alice/cern.ch/user/a/aborquez/work/weights/blastwave.root";
        PtWeightsHist = GetPtWeights(pt_weights_path, SexaquarkMass);
        task->AddPtWeights(PtWeightsHist);
    }

    TH1F *RadiusWeightsHist = nullptr;
    if (ReweightRadius) {
        TString radius_weights_path = "alien:///alice/cern.ch/user/a/aborquez/work/weights/radius-weights.root";
        RadiusWeightsHist = GetRadiusWeights(radius_weights_path);
        task->AddRadiusWeights(RadiusWeightsHist);
    }

    task->Initialize();

    /* --- */

    mgr->AddTask(task);

    mgr->ConnectInput(task, 0, mgr->GetCommonInputContainer());

    TString fileName = AliAnalysisManager::GetCommonFileName();
    mgr->ConnectOutput(task, 1, mgr->CreateContainer("Trees", TList::Class(), AliAnalysisManager::kOutputContainer, fileName.Data()));
    mgr->ConnectOutput(task, 2, mgr->CreateContainer("Hists", TList::Class(), AliAnalysisManager::kOutputContainer, fileName.Data()));

    return task;
}

/*
  Get a histogram with the pt weights for the blast wave.
*/
TH1D *GetPtWeights(TString filename, Float_t SexaquarkMass) {

    TFile *f = TFile::Open(filename, "READ");
    if (!f) {
        std::cerr << "!! GetPtWeights !! Unable to open file " << filename << " !!" << std::endl;
        return nullptr;
    }

    TString hist_name = Form("BlastWaveHist_%.2f", SexaquarkMass);
    TH1D *h = dynamic_cast<TH1D *>(f->Get(hist_name));
    if (!h) {
        std::cerr << "!! GetPtWeights !! Unable to find histogram " << hist_name << " in file " << filename << " !!" << std::endl;
        return nullptr;
    }

    return h;
}

/*
  Get a histogram with the radius weights of photon conversions.
*/
TH1F *GetRadiusWeights(TString filename) {

    TFile *f = TFile::Open(filename, "READ");
    if (!f) {
        std::cerr << "!! GetRadiusWeights !! Unable to open file " << filename << " !!" << std::endl;
        return nullptr;
    }

    TString hist_name = "MCGen_PhotonConversions_Radius";
    TH1F *h = dynamic_cast<TH1F *>(f->Get("Hists")->FindObject(hist_name));
    if (!h) {
        std::cerr << " !! GetRadiusWeights !! !! Unable to find histogram " << hist_name << " in file " << filename << " !!" << std::endl;
        return nullptr;
    }

    return h;
}
