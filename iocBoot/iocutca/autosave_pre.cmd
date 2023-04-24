save_restoreSet_status_prefix("${P}${R}")
save_restoreSet_DatedBackupFiles(1)

save_restoreSet_NumSeqFiles(3)
save_restoreSet_SeqPeriodInSeconds(300)

set_savefile_path("$(AUTOSAVE_PATH)", "")

set_requestfile_path("$(TOP)", "utcaApp/Db")

dbLoadRecords("db/save_restoreStatus.db", "P=${P}${R}")

set_pass0_restoreFile("${P}${R}_settings.sav")
set_pass1_restoreFile("${P}${R}_settings.sav")
