; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Italian.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_ITALIAN} "Linguaggio del programma di installazione"
LangString SelectInstallerLanguage  ${LANG_ITALIAN} "Scegliere per favore il linguaggio del programma di installazione"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_ITALIAN} " Update"
LangString LicenseSubTitleSetup ${LANG_ITALIAN} " Setup"

; installation directory text
LangString DirectoryChooseTitle ${LANG_ITALIAN} "Directory di installazione" 
LangString DirectoryChooseUpdate ${LANG_ITALIAN} "Scegli la directory di ${PRODUCT_LONG} per l’update alla versione ${VERSION_RELEASE}.(XXX):"
LangString DirectoryChooseSetup ${LANG_ITALIAN} "Scegli la directory dove installare ${PRODUCT_LONG}:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_ITALIAN} "Non riesco a trovare il programma '$INSTPROG'. Silent Update fallito."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_ITALIAN} "Avvia ora ${PRODUCT_LONG}?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_ITALIAN} "Controllo delle precedenti versioni…"

; check windows version
LangString CheckWindowsVersionDP ${LANG_ITALIAN} "Controllo della versione di Windows…"
LangString CheckWindowsVersionMB ${LANG_ITALIAN} '${PRODUCT_LONG} supporta solo Windows XP, Windows 2000 e Mac OS X.$\n$\nTentare l’installazione su Windows $R0 può provocare blocchi di sistema e perdita di dati.$\n$\nInstallare comunque?'

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_ITALIAN} "Controllo del permesso di installazione…"
LangString CheckAdministratorInstMB ${LANG_ITALIAN} 'Stai utilizzando un account “limitato”.$\nSolo un “amministratore” può installare ${PRODUCT_LONG}.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_ITALIAN} "Controllo del permesso di installazione…"
LangString CheckAdministratorUnInstMB ${LANG_ITALIAN} 'Stai utilizzando un account “limitato”.$\nSolo un “amministratore” può installare ${PRODUCT_LONG}.'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_ITALIAN} "${PRODUCT_LONG} ${VERSION_RELEASE} è stato sia già installato.$\n$\nVuoi ripetere l’installazione?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_ITALIAN} "In attesa che ${PRODUCT_LONG} chiuda…"
LangString CloseSecondLifeInstMB ${LANG_ITALIAN} "Non è possibile installare ${PRODUCT_LONG} se è già in funzione..$\n$\nTermina le operazioni in corso e scegli OK per chiudere ${PRODUCT_LONG} e continuare.$\nScegli CANCELLA per annullare l’installazione."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_ITALIAN} "In attesa della chiusura di ${PRODUCT_LONG}…"
LangString CloseSecondLifeUnInstMB ${LANG_ITALIAN} "Non è possibile installare ${PRODUCT_LONG} se è già in funzione.$\n$\nTermina le operazioni in corso e scegli OK per chiudere ${PRODUCT_LONG} e continuare.$\nScegli CANCELLA per annullare."

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_ITALIAN} "Verifica connessione di rete in corso..."

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_ITALIAN} "Cancellazione dei file cache nella cartella Documents and Settings"

; delete program files
LangString DeleteProgramFilesMB ${LANG_ITALIAN} "Sono ancora presenti dei file nella directory programmi di ${PRODUCT_LONG}.$\n$\nPotrebbe trattarsi di file creati o trasferiti in:$\n$INSTDIR$\n$\nVuoi cancellarli?"

; uninstall text
LangString UninstallTextMsg ${LANG_ITALIAN} "Così facendo ${PRODUCT_LONG} verrà disinstallato ${VERSION_RELEASE} dal tuo sistema."
