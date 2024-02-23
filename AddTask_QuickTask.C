AliAnalysisQuickTask* AddTask_QuickTask(TString name = "name", Bool_t IsMC = kTRUE) {
    // get the manager via the static access member. since it's static, you don't need
    // to create an instance of the class here to call the function
    AliAnalysisManager* mgr = AliAnalysisManager::GetAnalysisManager();
    if (!mgr) {
        return 0x0;
    }

    // get the input event handler, again via a static method.
    // this handler is part of the managing system and feeds events to your task
    if (!mgr->GetInputEventHandler()) {
        return 0x0;
    }

    // by default, a file is open for writing. here, we get the filename
    TString fileName = AliAnalysisManager::GetCommonFileName();

    // now we create an instance of your task
    AliAnalysisQuickTask* task = new AliAnalysisQuickTask(name, IsMC);

    if (!task) {
        return 0x0;
    }

    // trigger configuration, commented on purpose
    // task->SelectCollisionCandidates(AliVEvent::kAnyINT);

    // add your task to the manager
    mgr->AddTask(task);
    // connect task's to the manager
    mgr->ConnectInput(task, 0, mgr->GetCommonInputContainer());
    // same for the output
    mgr->ConnectOutput(task, 1, mgr->CreateContainer("Hists", TList::Class(), AliAnalysisManager::kOutputContainer, fileName.Data()));

    // in the end, this macro returns a pointer to your task. this will be convenient later on
    // when you will run your analysis in an analysis train on grid
    return task;
}
