AliAnalysisQuickTask* AddTask_QuickTask() {

    AliAnalysisManager* mgr = AliAnalysisManager::GetAnalysisManager();

    AliAnalysisQuickTask* task = new AliAnalysisQuickTask("name");

    mgr->AddTask(task);

    mgr->ConnectInput(task, 0, mgr->GetCommonInputContainer());

    TString fileName = AliAnalysisManager::GetCommonFileName();
    mgr->ConnectOutput(task, 1, mgr->CreateContainer("Hists", TList::Class(), AliAnalysisManager::kOutputContainer, fileName.Data()));

    return task;
}
